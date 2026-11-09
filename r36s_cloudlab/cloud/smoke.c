#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>
#include <pthread.h>

static void *worker(void *x) { return (void *)(uintptr_t)((uintptr_t)x + 1); }
static int failure(const char* s) { fprintf(stderr, "CLOUD_SMOKE_FAIL=%s errno=%d\n", s, errno); return 1; }
int main(int argc,char**argv) {
  unsigned long mib=64;
  if(argc>1){
    char *tail=NULL;
    errno=0;
    mib=strtoul(argv[1],&tail,10);
    if(errno||!*argv[1]||!tail||*tail||mib<1||mib>640) return failure("invalid_memory_mib_1_to_640");
  }
#ifdef __aarch64__
 puts("CLOUD_ARM64_NATIVE=PASS");
#else
 puts("CLOUD_ARM64_NATIVE=SKIPPED_NON_ARM64_HOST");
#endif
  long p = sysconf(_SC_PAGESIZE);
  if (p != 4096) return failure("page_size_not_4096_profile_mismatch");
  printf("CLOUD_PAGE_SIZE=%ld\n", p);
  pthread_t th;
  if (pthread_create(&th, NULL, worker, (void*)(uintptr_t)41) != 0) return failure("pthread_create");
  void *value=NULL;
  if (pthread_join(th, &value) != 0 || (uintptr_t)value != 42) return failure("pthread_join");
  puts("CLOUD_THREADS=PASS");
  const size_t mem = (size_t)mib * 1024u * 1024u;
  unsigned char *buf = mmap(NULL, mem, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (buf==MAP_FAILED) return failure("mmap_ram");
  uint64_t sum=0;
  for (size_t i=0;i<mem;i+=(size_t)p) buf[i]=(unsigned char)(((i/(size_t)p)%251u)+1u);
  for (size_t i=0;i<mem;i+=(size_t)p) {
    unsigned char expected=(unsigned char)(((i/(size_t)p)%251u)+1u);
    if(buf[i]!=expected) return failure("ram_readback_bad_page");
    sum+=buf[i];
  }
  if(sum==0) return failure("ram_readback_zero");
  struct rusage before={0};
  if (getrusage(RUSAGE_SELF, &before) == 0)
    printf("CLOUD_PEAK_RSS_KB=%ld\n",before.ru_maxrss);
  munmap(buf,mem);
  printf("CLOUD_RAM_MIB=%lu COMMITTED_PAGES=%zu READBACK=PASS CHECKSUM=%llu\n",mib,mem/(size_t)p,(unsigned long long)sum);
  void *code=mmap(NULL,(size_t)p,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
  if(code==MAP_FAILED) return failure("mmap_code");
  memset(code,0,(size_t)p);
  if(mprotect(code,(size_t)p,PROT_READ|PROT_EXEC)!=0) return failure("mprotect_rx");
  if(mprotect(code,(size_t)p,PROT_READ|PROT_WRITE)!=0) return failure("mprotect_rw");
  munmap(code,(size_t)p);
  puts("CLOUD_MPROTECT_RW_TO_RX=PASS");
  puts("CLOUD_TESTS=PASS SYNTHETIC_ONLY=YES PERFORMANCE_EQUIVALENCE=NO");
  return 0;
}
