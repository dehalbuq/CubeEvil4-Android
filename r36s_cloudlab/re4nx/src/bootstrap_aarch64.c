// Linux AArch64 freestanding static diagnostic: no libc, SDL, keys, or game data.
// This is NOT a Switch runtime and does NOT execute games.
// SPDX-License-Identifier: MIT
#if !defined(__aarch64__)
#error This small bootstrap must be compiled with --target=aarch64-linux-gnu
#endif
typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long usize;
#define AT_FDCWD (-100L)
#define SYS_OPENAT 56
#define SYS_CLOSE 57
#define SYS_READ 63
#define SYS_WRITE 64
#define SYS_EXIT 93
static long call1(long no,long a){
 register long x0 __asm__("x0")=a;
 register long x8 __asm__("x8")=no;
 __asm__ volatile("svc #0" : "+r"(x0) : "r"(x8) : "memory","cc");
 return x0;
}
static long call3(long no,long a,long b,long c){
 register long x0 __asm__("x0")=a;
 register long x1 __asm__("x1")=b;
 register long x2 __asm__("x2")=c;
 register long x8 __asm__("x8")=no;
 __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1),"r"(x2),"r"(x8) : "memory","cc");
 return x0;
}
static long call4(long no,long a,long b,long c,long d){
 register long x0 __asm__("x0")=a;
 register long x1 __asm__("x1")=b;
 register long x2 __asm__("x2")=c;
 register long x3 __asm__("x3")=d;
 register long x8 __asm__("x8")=no;
 __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1),"r"(x2),"r"(x3),"r"(x8) : "memory","cc");
 return x0;
}
static usize slen(const char*s){usize i=0;while(s[i])i++;return i;}
static void out(const char*s){call3(SYS_WRITE,1,(long)s,(long)slen(s));}
static void hexd(u32 v){
 static const char tbl[]="0123456789abcdef";
 char b[11]={'0','x',0,0,0,0,0,0,0,0,'\n'};
 for(int i=0;i<8;i++)b[2+i]=tbl[(v>>(28-4*i))&15u];
 call3(SYS_WRITE,1,(long)b,11);
}
static u32 le32(const u8*p){return (u32)p[0]|(u32)p[1]<<8|(u32)p[2]<<16|(u32)p[3]<<24;}
__attribute__((noreturn))static void done(long code){call1(SYS_EXIT,code);__builtin_unreachable();}
void bootstrap_main(long *stack);
// Kernel launches the process here; no ELF dynamic linker is needed.
__asm__(".global _start\n.type _start,%function\n_start:\n mov x0, sp\n b bootstrap_main\n");
void bootstrap_main(long *stack){
 long argc=stack[0]; char **argv=(char**)(stack+1);
 out("RE4NX_R36S_BOOTSTRAP=1\nARCH=AARCH64 ABI=LINUX_SYSV\n");
 if(argc<2){
  out("SYNTHETIC_NATIVE_BOOTSTRAP=PASS\nGUEST_GAME_EXECUTION=DISABLED\nUSAGE: bootstrap-aarch64 /path/to/decrypted/main.nso\n");
  done(0);
 }
 long fd=call4(SYS_OPENAT,AT_FDCWD,(long)argv[1],0,0);
 if(fd<0){out("NSO_INSPECT=FAIL OPENAT\n");done(2);}
 u8 h[0x100];usize got=0;
 while(got<sizeof h){
  long n=call3(SYS_READ,fd,(long)(h+got),(long)(sizeof h-got));
  if(n<=0)break;got+=(usize)n;
 }
 call1(SYS_CLOSE,fd);
 if(got<sizeof h){out("NSO_INSPECT=FAIL HEADER_TOO_SHORT\n");done(3);}
 if(h[0]!='N'||h[1]!='S'||h[2]!='O'||h[3]!='0'){
  out("NSO_INSPECT=FAIL EXPECTED_UNENCRYPTED_NSO0\n");done(4);
 }
 u32 flags=le32(h+0xc);
 if(flags&0x80){out("NSO_INSPECT=UNSUPPORTED_ZBIC\n");done(5);}
 out("NSO_INSPECT=HEADER_OK\nFLAGS=");hexd(flags);
 out("TEXT_UNCOMPRESSED_BYTES=");hexd(le32(h+0x18));
 out("RODATA_UNCOMPRESSED_BYTES=");hexd(le32(h+0x28));
 out("DATA_UNCOMPRESSED_BYTES=");hexd(le32(h+0x38));
 out("BSS_BYTES=");hexd(le32(h+0x3c));
 out("NOTE=HEADER_ONLY_USE_CPP_RUNNER_FOR_LZ4_SHA256_CHECK\nCOMMERCIAL_EXECUTION=BLOCKED\n");
 done(0);
}
