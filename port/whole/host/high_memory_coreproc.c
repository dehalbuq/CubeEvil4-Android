// AArch64 core process must keep game-visible mappings below 4GB, as re4_host does.
// Extracted from the Linux re4_host 64-bit reservation logic.
#define _GNU_SOURCE
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include "low_memory.h"

#if __SIZEOF_POINTER__ == 8
#define HIGH_START 0x100000000UL

/* A free range, or as much of it as can be had: a range with a mapping the
 * process cannot see (qemu-user keeps its own memory out of the guest's
 * /proc/self/maps) is split until the pieces around it fit. Returns the number
 * of pages left out. */
static unsigned long gSplitBudget = 1UL << 16;

static unsigned long reserveSplit(unsigned long from, unsigned long to, int depth)
{
    if (to <= from || reserve(from, to)) {
        return 0;
    }
    unsigned long page = (unsigned long) sysconf(_SC_PAGESIZE);
    if (to - from <= page || depth > 40 || !gSplitBudget) {
        return (to - from) / page;
    }
    gSplitBudget--;
    unsigned long mid = from + (((to - from) / 2) & ~(page - 1));
    if (mid <= from) {
        mid = from + page;
    }
    return reserveSplit(from, mid, depth + 1) + reserveSplit(mid, to, depth + 1);
}

/* Every free range above 4 GB, up to the end of the user address space. mmap
 * without an address then has only the window left (on a native kernel it
 * hands out space below mmap_base, at least 128 MB under the stack; 64 MB
 * under the stack stay free for it to grow). Runs before the first malloc:
 * the brk heap would grow behind re4_host, above 4 GB; with that range taken,
 * malloc takes its memory with mmap, inside the window. Under qemu-user
 * (aarch64 in the cloud) the stack is the lowest mapping above 4 GB and the
 * C library, the program and a brk area lie above it: the ranges up to the
 * top count as well (malloc stays above 4 GB there, see main). */
/* The end of the user address space (TASK_SIZE: 47 bit less a page on
 * x86_64, 39 to 48 bit on arm64): the highest of these ends below which a
 * page can be mapped or is in use, and at least the end of the highest
 * mapping (qemu-user keeps the guest's program near the host's top). */
static unsigned long userTop(unsigned long used[][2], int n)
{
    static const int bits[] = {48, 47, 42, 39};
    unsigned long page = (unsigned long) sysconf(_SC_PAGESIZE);
    unsigned long top = 1UL << 39;
    for (unsigned t = 0; t < sizeof(bits) / sizeof(bits[0]); t++) {
        unsigned long end = (1UL << bits[t]) - page;
        void* want = (void*) (end - page);
        void* p = mmap(want, page, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED_NOREPLACE, -1, 0);
        if (p == want || (p == MAP_FAILED && errno == EEXIST)) {
            if (p == want) {
                munmap(p, page);
            }
            top = end;
            break;
        }
        if (p != MAP_FAILED) {
            munmap(p, page);
        }
    }
    for (int i = 0; i < n; i++) {
        if (used[i][1] > top && used[i][1] < (1UL << 56)) {
            top = used[i][1];
        }
    }
    return top;
}

void re4_reserve_high_memory(void)
{
    static unsigned long used[1024][2];
    unsigned long stack = 0;
    int n = readMaps(used, 1024, &stack);
    if (!stack) {
        fprintf(stderr, "re4_coreproc: no [stack] in /proc/self/maps\n");
        return;
    }
    unsigned long top = userTop(used, n);
    /* a native kernel keeps the stack near the top and lets it grow down;
       qemu-user maps a fixed one far below the top */
    unsigned long keep_from = stack - (64UL << 20), keep_to = stack;
    if (top - stack > (1UL << 32)) {
        keep_from = keep_to = 0;
    }
    unsigned long cursor = HIGH_START;
    unsigned long left = 0;
    for (int i = 0; i <= n && cursor < top; i++) {
        unsigned long next = top;
        if (i < n) {
            if (used[i][1] <= cursor) {
                continue;
            }
            if (used[i][0] < top) {
                next = used[i][0];
            }
        }
        if (next > cursor) {
            /* leave room for the stack to grow */
            if (cursor < keep_to && next > keep_from) {
                left += reserveSplit(cursor, keep_from > cursor ? keep_from : cursor, 0);
                if (next > keep_to) {
                    left += reserveSplit(keep_to, next, 0);
                }
            } else {
                left += reserveSplit(cursor, next, 0);
            }
        }
        if (i < n) {
            cursor = used[i][1];
        }
    }
    if (left) {
        fprintf(stderr, "re4_coreproc: %lu pages above 4 GB not reserved: memory there may reach the game\n", left);
    }
    /* one malloc arena: each further one takes 64 MB of the window */
    mallopt(M_ARENA_MAX, 1);
}

#else
void re4_reserve_high_memory(void) {}
#endif
