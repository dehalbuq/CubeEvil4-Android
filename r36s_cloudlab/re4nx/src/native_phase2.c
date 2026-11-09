// RE4NX-R36S Phase 2: freestanding Linux AArch64 NSO validation + synthetic execution.
// MIT license; no firmware keys, copyrighted game modules or Horizon HLE.
// Static AArch64 ELF, syscalls only, no libc, dynamic loader or C++ runtime.
// Real NSO files are INSPECTED, NEVER EXECUTED.
#if defined(__aarch64__)
typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef unsigned long usize;
#define RE4NX_NATIVE_ARM64 1
static long sys6(long n,long a,long b,long c,long d,long e,long f){
  register long x0 __asm__("x0")=a, x1 __asm__("x1")=b, x2 __asm__("x2")=c;
  register long x3 __asm__("x3")=d, x4 __asm__("x4")=e, x5 __asm__("x5")=f;
  register long x8 __asm__("x8")=n;
  __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1),"r"(x2),"r"(x3),"r"(x4),"r"(x5),"r"(x8) : "memory","cc");
  return x0;
}
#define N_OPENAT 56
#define N_CLOSE 57
#define N_LSEEK 62
#define N_WRITE 64
#define N_MMAP 222
#define N_MPROTECT 226
#define N_MUNMAP 215
#define N_EXIT 93
__attribute__((noreturn)) static void process_exit(long code){sys6(N_EXIT,code,0,0,0,0,0);__builtin_unreachable();}
void program_main(long *stack);
__asm__(".global _start\n.type _start,%function\n_start:\n mov x0, sp\n b program_main\n");
#else
// Host validation uses equivalent libc syscalls, not a guest execution test.
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef unsigned long usize;
#define RE4NX_NATIVE_ARM64 0
#endif

#define KB 1024UL
#define MB (KB*KB)
#define LIMIT_INPUT (192UL*MB)
#define LIMIT_SEGMENT (160UL*MB)
#define LIMIT_IMAGE (192UL*MB)
#define FLAGS_COMPRESSED 7u
#define FLAGS_HASHED 0x38u
#define PROT_R 1
#define PROT_W 2
#define PROT_X 4
#define MAP_PRIVATE_FLAG 2
#define MAP_ANON_FLAG 0x20
#define O_RDONLY_FLAG 0
#define AT_FDCWD_FLAG (-100L)

static long io_open(const char*s){
#if RE4NX_NATIVE_ARM64
  return sys6(N_OPENAT,AT_FDCWD_FLAG,(long)s,O_RDONLY_FLAG,0,0,0);
#else
  return open(s,O_RDONLY);
#endif
}
static long io_close(long fd){
#if RE4NX_NATIVE_ARM64
  return sys6(N_CLOSE,fd,0,0,0,0,0);
#else
  return close((int)fd);
#endif
}
static long io_seek(long fd,long off,long whence){
#if RE4NX_NATIVE_ARM64
  return sys6(N_LSEEK,fd,off,whence,0,0,0);
#else
  return lseek((int)fd,off,(int)whence);
#endif
}
static long io_write(long fd,const void*b,usize len){
#if RE4NX_NATIVE_ARM64
  return sys6(N_WRITE,fd,(long)b,(long)len,0,0,0);
#else
  return write((int)fd,b,(size_t)len);
#endif
}
static void *io_map(long fd,usize n,long prot,long flags){
#if RE4NX_NATIVE_ARM64
  return (void*)sys6(N_MMAP,0,(long)n,prot,flags,fd,0);
#else
  return mmap(0,(size_t)n,(int)prot,(int)flags,(int)fd,0);
#endif
}
static long io_protect(void*p,usize n,long prot){
#if RE4NX_NATIVE_ARM64
  return sys6(N_MPROTECT,(long)p,(long)n,prot,0,0,0);
#else
  return mprotect(p,(size_t)n,(int)prot);
#endif
}
static long io_unmap(void*p,usize n){
#if RE4NX_NATIVE_ARM64
  return sys6(N_MUNMAP,(long)p,(long)n,0,0,0,0);
#else
  return munmap(p,(size_t)n);
#endif
}
static usize cstrlen(const char*s){usize n=0;while(s[n])n++;return n;}
static int ceq(const char*a,const char*b){for(;*a&&*b;a++,b++)if(*a!=*b)return 0;return *a==*b;}
static void put(const char*s){io_write(1,s,cstrlen(s));}
static void num(u64 v){char b[32];usize i=0,j;do{b[i++]=(char)('0'+(v%10));v/=10;}while(v);for(j=0;j<i/2;j++){char t=b[j];b[j]=b[i-j-1];b[i-j-1]=t;}io_write(1,b,i);}
static void hex32(u32 v){static const char*chars="0123456789abcdef";char b[10]={'0','x'};for(int i=0;i<8;i++)b[2+i]=chars[(v>>(28-4*i))&15u];io_write(1,b,10);}
static u32 le(const u8*p){return (u32)p[0]|((u32)p[1]<<8)|((u32)p[2]<<16)|((u32)p[3]<<24);}
static void copybytes(u8*dst,const u8*src,usize n){for(usize i=0;i<n;i++)dst[i]=src[i];}
static void zerobytes(u8*dst,usize n){for(usize i=0;i<n;i++)dst[i]=0;}
static int same(const u8*a,const u8*b,usize n){for(usize i=0;i<n;i++)if(a[i]!=b[i])return 0;return 1;}
static usize a4096(usize v){return (v+4095UL)&~4095UL;}
static int map_failed(const void*p){return (unsigned long)p>=(unsigned long)-4095L;}
static int fail(const char*reason){put("PHASE2_RESULT=FAIL REASON=");put(reason);put("\nCOMMERCIAL_EXECUTION=BLOCKED\n");return 1;}

// SHA256, FIPS 180-4; validates the DECOMPRESSED segments, not the LZ4 data.
static u32 ror32(u32 x,unsigned b){return (x>>b)|(x<<(32-b));}
static void sha256(const u8*input,usize length,u8 result[32]){
 static const u32 K[64]={
 0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
 0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
 0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
 0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
 0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
 0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
 0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
 0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u};
 u32 h[8]={0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u};
 u64 bits=(u64)length*8u;
 usize blocks=(length/64)+1+(((length%64)>=56)?1:0);
 for(usize block=0;block<blocks;block++){
  u8 bytes[64];zerobytes(bytes,64);
  for(usize j=0;j<64;j++){
   usize pos=block*64+j;
   if(pos<length)bytes[j]=input[pos];else if(pos==length)bytes[j]=0x80;
  }
  if(block+1==blocks)for(unsigned j=0;j<8;j++)bytes[56+j]=(u8)(bits>>(56u-8u*j));
  u32 w[64];
  for(unsigned j=0;j<16;j++){
   const u8*q=bytes+j*4;
   w[j]=((u32)q[0]<<24)|((u32)q[1]<<16)|((u32)q[2]<<8)|(u32)q[3];
  }
  for(unsigned j=16;j<64;j++){
   u32 x=w[j-15],y=w[j-2];
   u32 s0=ror32(x,7)^ror32(x,18)^(x>>3);
   u32 s1=ror32(y,17)^ror32(y,19)^(y>>10);
   w[j]=w[j-16]+s0+w[j-7]+s1;
  }
  u32 a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],z=h[7];
  for(unsigned j=0;j<64;j++){
   u32 S1=ror32(e,6)^ror32(e,11)^ror32(e,25);
   u32 t1=z+S1+((e&f)^(~e&g))+K[j]+w[j];
   u32 S0=ror32(a,2)^ror32(a,13)^ror32(a,22);
   u32 t2=S0+((a&b)^(a&c)^(b&c));
   z=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
  }
  h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=z;
 }
 for(unsigned i=0;i<8;i++)for(unsigned j=0;j<4;j++)result[4*i+j]=(u8)(h[i]>>(24u-8u*j));
}
static int raw_lz4(const u8*src,usize size,u8*dst,usize expected){
 usize in=0,out=0;
 while(in<size){
  u8 token=src[in++];usize literal=(usize)(token>>4);
  if(literal==15){u8 n;do{if(in>=size)return 0;n=src[in++];if(literal>expected||n>expected-literal)return 0;literal+=n;}while(n==255);}
  if(literal>size-in||literal>expected-out)return 0;
  copybytes(dst+out,src+in,literal);out+=literal;in+=literal;
  if(in==size)break;
  if(size-in<2)return 0;
  usize offset=(usize)src[in]|((usize)src[in+1]<<8);in+=2;
  if(offset==0||offset>out)return 0;
  usize match=(usize)(token&15);
  if(match==15){u8 n;do{if(in>=size)return 0;n=src[in++];if(match>expected||n>expected-match)return 0;match+=n;}while(n==255);}
  if(match>expected-out||expected-out-match<4)return 0;
  match+=4;
  for(usize i=0;i<match;i++)dst[out+i]=dst[out+i-offset];
  out+=match;
 }
 return out==expected;
}

typedef struct {
 u32 source,offset,size,stored,compressed,hash;
} Segment;
#if RE4NX_NATIVE_ARM64
static void flush_native_cache(u8*begin,usize len){
  unsigned long ctr;
  __asm__ volatile("mrs %0, ctr_el0" : "=r"(ctr));
  const usize dline=(usize)4u<<((ctr>>16)&15u);
  const usize iline=(usize)4u<<(ctr&15u);
  const usize start=(usize)begin,end=start+len;
  for(usize p=start&~(dline-1u);p<end;p+=dline)
    __asm__ volatile("dc cvau, %0" :: "r"(p) : "memory");
  __asm__ volatile("dsb ish" ::: "memory");
  for(usize p=start&~(iline-1u);p<end;p+=iline)
    __asm__ volatile("ic ivau, %0" :: "r"(p) : "memory");
  __asm__ volatile("dsb ish\n isb" ::: "memory");
}
#endif
static int test_nso(const char*path,int synthetic){
 long fd=io_open(path);
 if(fd<0)return fail("OPEN_FAILED");
 long end=io_seek(fd,0,2);
 if(end<0||end<0x100||end>(long)LIMIT_INPUT){io_close(fd);return fail("FILE_SIZE_INVALID");}
 usize fileSize=(usize)end;
 const u8*blob=(const u8*)io_map(fd,fileSize,PROT_R,MAP_PRIVATE_FLAG);
 if(map_failed(blob)){io_close(fd);return fail("INPUT_MMAP_FAILED");}
 if(!same(blob,(const u8*)"NSO0",4)){io_unmap((void*)blob,fileSize);io_close(fd);return fail("NOT_UNENCRYPTED_NSO0");}
 u32 flags=le(blob+0xc),bss=le(blob+0x3c);
 if(flags&0x80u){io_unmap((void*)blob,fileSize);io_close(fd);return fail("ZBIC_UNSUPPORTED");}
 Segment sec[3];
 usize maxEnd=0;
 for(unsigned i=0;i<3;i++){
  const u8*h=blob+0x10+i*0x10;
  sec[i].source=le(h);sec[i].offset=le(h+4);sec[i].size=le(h+8);
  sec[i].compressed=(flags>>i)&1u;sec[i].hash=(flags>>(i+3))&1u;
  sec[i].stored=sec[i].compressed?le(blob+0x60+4*i):sec[i].size;
  u64 memEnd=(u64)sec[i].offset+(u64)sec[i].size;
  u64 fileEnd=(u64)sec[i].source+(u64)sec[i].stored;
  if(sec[i].size>LIMIT_SEGMENT||memEnd>LIMIT_IMAGE||fileEnd>fileSize||
     (sec[i].size&&sec[i].source<0x100)||
     (sec[i].size==0&&sec[i].stored!=0)||
     (sec[i].size!=0&&sec[i].stored==0)){
   io_unmap((void*)blob,fileSize);io_close(fd);return fail("SECTION_BOUNDS_INVALID");
  }
  if(memEnd>maxEnd)maxEnd=(usize)memEnd;
 }
 // Reject both memory overlaps and stored-file overlaps; avoid ambiguous maps.
 for(unsigned i=0;i<3;i++)for(unsigned j=i+1;j<3;j++){
  u64 a=sec[i].offset,b=sec[j].offset;
  u64 fa=sec[i].source,fb=sec[j].source;
  if((sec[i].size&&sec[j].size&&a<(b+sec[j].size)&&b<(a+sec[i].size))||
     (sec[i].stored&&sec[j].stored&&fa<(fb+sec[j].stored)&&fb<(fa+sec[i].stored))){
   io_unmap((void*)blob,fileSize);io_close(fd);return fail("SECTION_OVERLAP");
  }
 }
 u64 bssBegin=(u64)sec[2].offset+sec[2].size;
 u64 bssEnd=bssBegin+bss;
 if(bssEnd>LIMIT_IMAGE){io_unmap((void*)blob,fileSize);io_close(fd);return fail("BSS_TOO_LARGE");}
 for(unsigned i=0;i<2;i++){
  u64 a=sec[i].offset,e=a+sec[i].size;
  if(bss&&sec[i].size&&a<bssEnd&&e>bssBegin){io_unmap((void*)blob,fileSize);io_close(fd);return fail("BSS_OVERLAP");}
 }
 if(bssEnd>maxEnd)maxEnd=(usize)bssEnd;
 if(maxEnd==0){io_unmap((void*)blob,fileSize);io_close(fd);return fail("EMPTY_IMAGE");}
 usize imageLen=a4096(maxEnd);
 u8*image=(u8*)io_map(-1,imageLen,PROT_R|PROT_W,MAP_PRIVATE_FLAG|MAP_ANON_FLAG);
 if(map_failed(image)){io_unmap((void*)blob,fileSize);io_close(fd);return fail("IMAGE_MMAP_FAILED");}
 zerobytes(image,imageLen);
 for(unsigned i=0;i<3;i++){
  const Segment*s=&sec[i];u8*target=image+s->offset;
  int good=s->compressed?raw_lz4(blob+s->source,s->stored,target,s->size):1;
  if(!s->compressed)copybytes(target,blob+s->source,s->size);
  if(!good){io_unmap(image,imageLen);io_unmap((void*)blob,fileSize);io_close(fd);return fail("LZ4_DECODE_FAILED");}
  if(s->hash){u8 hash[32];sha256(target,s->size,hash);
    if(!same(hash,blob+0xa0+i*32,32)){
      io_unmap(image,imageLen);io_unmap((void*)blob,fileSize);io_close(fd);return fail("SHA256_MISMATCH");
    }
  }
 }
 put("RE4NX_PHASE2=1\nNSO_INSPECT=PASS LZ4=PASS SHA256=PASS BOUNDS=PASS\n");
 put("NSO_IMAGE_BYTES=");num(imageLen);put("\nFILE_BYTES=");num(fileSize);put("\nFLAGS=");hex32(flags);put("\n");
 for(unsigned i=0;i<3;i++){
  put("SEGMENT=");put(i==0?"TEXT":i==1?"RODATA":"DATA");
  put(" OFFSET=");hex32(sec[i].offset);put(" BYTES=");num(sec[i].size);
  put(" HASH=");put(sec[i].hash?"VERIFIED":"NOT_PRESENT");put("\n");
 }
 put("BSS_BYTES=");num(bss);put("\n");
 int ok=1;
 if(synthetic){
  static const u8 code[8]={0x40,0x05,0x80,0x52,0xc0,0x03,0x5f,0xd6};
  static const u8 ro[4]={'R','3','6','S'};
  static const u8 da[4]={'T','E','S','T'};
  if(flags!=0x3fu||sec[0].size!=8||sec[1].size!=4||sec[2].size!=4||
     sec[0].offset!=0||sec[1].offset!=4096||sec[2].offset!=8192||bss!=256||
     !same(image,code,8)||!same(image+4096,ro,4)||!same(image+8192,da,4))ok=0;
  if(!ok){io_unmap(image,imageLen);io_unmap((void*)blob,fileSize);io_close(fd);return fail("NOT_APPROVED_SYNTHETIC_FIXTURE");}
  // Demonstrate strict W^X page permissions, no commercial code can reach here.
  if(io_protect(image,4096,PROT_R|PROT_X)!=0||
     io_protect(image+4096,4096,PROT_R)!=0){
    io_unmap(image,imageLen);io_unmap((void*)blob,fileSize);io_close(fd);return fail("MPROTECT_WX_FAILED");
  }
#if RE4NX_NATIVE_ARM64
  flush_native_cache(image,8);
  int (*entry)(void)=(int(*)(void))(void*)image;
  int result=entry();
  if(result!=42){io_unmap(image,imageLen);io_unmap((void*)blob,fileSize);io_close(fd);return fail("SYNTHETIC_RETURN_MISMATCH");}
  put("ARM64_SYNTHETIC_EXECUTION=PASS RETURN=42 W_X=PASS\n");
#else
  put("ARM64_SYNTHETIC_EXECUTION=SKIP HOST_X86_64 VALIDATION_ONLY=PASS W_X=PASS\n");
#endif
 }else put("COMMERCIAL_EXECUTION=BLOCKED HORIZON_HLE=NOT_IMPLEMENTED\n");
 io_unmap(image,imageLen);io_unmap((void*)blob,fileSize);io_close(fd);
 put("PHASE2_RESULT=PASS\n");return 0;
}
static int memory_probe(void){
 const usize target=64UL*MB;
 u8*data=(u8*)io_map(-1,target,PROT_R|PROT_W,MAP_PRIVATE_FLAG|MAP_ANON_FLAG);
 if(map_failed(data))return fail("RAM_MMAP_FAILED");
 usize count=0;
 for(usize i=0;i<target;i+=4096){data[i]=(u8)((i>>12)&0xffu);count++;}
 usize errors=0;
 for(usize i=0;i<target;i+=4096)if(data[i]!=(u8)((i>>12)&0xffu))errors++;
 io_unmap(data,target);
 put("RAM_PROBE_MIB=64 TOUCHED_4K_PAGES=");num(count);put(" ERRORS=");num(errors);put("\n");
 if(errors)return fail("RAM_PATTERN_MISMATCH");
 put("RAM_PROBE=PASS\n");
 return 0;
}
static int run(int argc,char**argv){
 put("RE4NX_PHASE2_NATIVE=1\nARCH=");put(RE4NX_NATIVE_ARM64?"AARCH64":"HOST_X86_64");
 put(" ABI=LINUX_SYSV\nTARGET_PROCESS_MIB=768 TOTAL_PHYSICAL_GOAL_MIB=1024\n");
 if(argc==2&&ceq(argv[1],"--memory-probe"))return memory_probe();
 if(argc==3&&ceq(argv[1],"--synthetic"))return test_nso(argv[2],1);
 if(argc==3&&ceq(argv[1],"--inspect"))return test_nso(argv[2],0);
 put("USAGE: re4nx-native-aarch64 --synthetic synthetic-aarch64.nso | --inspect owned.nso | --memory-probe\n");
 put("NINTENDO_HORIZON=NOT_IMPLEMENTED NVN_GLES3=NOT_IMPLEMENTED\n");return 0;
}
#if RE4NX_NATIVE_ARM64
void program_main(long*stack){
 long argc=stack[0];char**argv=(char**)(stack+1);
 process_exit(run((int)argc,argv));
}
#else
int main(int argc,char**argv){return run(argc,argv);}
#endif
