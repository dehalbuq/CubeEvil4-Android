// RE4NX Phase 5: restricted synthetic guest-SVC -> HIPC/CMIF bridge.
// Freestanding Linux AArch64 implementation (host tests use libc).
// SVC numbers match public Horizon docs; return codes/guest ABI are modeled.
// This is NOT a guest SVC trap handler, Horizon kernel, or commercial game loader.
#if defined(__aarch64__) && !defined(RE4NX_HOST_TEST)
typedef unsigned long usize;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
#define TARGET_ARCH 1
static long linux_syscall(long no,long a,long b,long c){
 register long x0 __asm__("x0")=a,x1 __asm__("x1")=b,x2 __asm__("x2")=c,x8 __asm__("x8")=no;
 __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1),"r"(x2),"r"(x8) : "memory","cc");
 return x0;
}
static void emit(const char*s,usize n){(void)linux_syscall(64,1,(long)s,(long)n);}
__attribute__((noreturn)) static void terminate(int rc){(void)linux_syscall(93,rc,0,0);__builtin_unreachable();}
void phase5_main(long*stack);
__asm__(".global _start\n.type _start,%function\n_start:\n mov x0, sp\n b phase5_main\n");
#else
#include <stdio.h>
typedef unsigned long usize;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
#define TARGET_ARCH 0
static void emit(const char*s,usize n){(void)fwrite(s,1,n,stdout);}
#endif

#define TLS_BYTES 256u
#define GUEST_BYTES 4096u
#define MAX_HANDLES 64u
#define CMIF_SFCI 0x49434653u
#define CMIF_SFCO 0x4f434653u
#define TEST_OK 0u
#define TEST_BAD_POINTER 0xf101u
#define TEST_BAD_MESSAGE 0xf102u
#define TEST_BAD_HANDLE 0xf103u
#define TEST_UNKNOWN_SERVICE 0xf104u
#define TEST_UNSUPPORTED 0xf105u
#define TEST_NO_HANDLES 0xf106u

static usize strsize(const char*s){usize n=0;while(s[n])n++;return n;}
static void say(const char*s){emit(s,strsize(s));}
static int equal(const char*a,const char*b){while(*a&&*b){if(*a!=*b)return 0;a++;b++;}return *a==*b;}
static void zero(u8*b,usize n){for(usize i=0;i<n;i++)b[i]=0;}
static u32 read32(const u8*b,usize i){return (u32)b[i]|((u32)b[i+1]<<8)|((u32)b[i+2]<<16)|((u32)b[i+3]<<24);}
static void write32(u8*b,usize i,u32 value){for(unsigned j=0;j<4;j++)b[i+j]=(u8)(value>>(8u*j));}
static void write16(u8*b,usize i,u16 value){b[i]=(u8)value;b[i+1]=(u8)(value>>8);}
static int fail(const char*msg){say("PHASE5_FINAL=FAIL REASON=");say(msg);say("\n");return 1;}

enum Kind {H_NONE=0,H_SM=1,H_HID=2,H_TIME=3,H_VI=4,H_NVDRV=5};
struct Hentry {u32 generation;enum Kind kind;};
struct Thread {u32 before;u8 tls[TLS_BYTES];u32 after;};
struct Runtime {struct Hentry handles[MAX_HANDLES];u8 guest[GUEST_BYTES];struct Thread thread;u32 active;};
struct Registers {u64 x[8];};
static struct Runtime rt;
static void reset(void){zero((u8*)&rt,sizeof(rt));rt.thread.before=0xc0febabeu;rt.thread.after=0xdecafbadU;}
static int canaries(void){return rt.thread.before==0xc0febabeu&&rt.thread.after==0xdecafbadU;}
static u32 alloc_handle(enum Kind kind,u32*out){
 if(!out||kind==H_NONE)return TEST_BAD_MESSAGE;
 for(unsigned i=0;i<MAX_HANDLES;i++)if(rt.handles[i].kind==H_NONE){
  struct Hentry*e=&rt.handles[i];
  e->generation=(e->generation+1u)&0xffffffu;
  if(!e->generation)e->generation=1u;
  e->kind=kind;
  *out=(e->generation<<8)|(i+1u);rt.active++;
  return TEST_OK;
 }
 return TEST_NO_HANDLES;
}
static struct Hentry*lookup(u32 h){
 u32 index=h&255u;
 if(index==0||index>MAX_HANDLES)return 0;
 struct Hentry*e=&rt.handles[index-1u];
 return e->kind!=H_NONE&&e->generation==(h>>8)?e:0;
}
static u32 release(u32 h){struct Hentry*e=lookup(h);if(!e)return TEST_BAD_HANDLE;e->kind=H_NONE;rt.active--;return TEST_OK;}
static u32 guest_port(u64 addr,u32*out){
 // Never turn untrusted guest addresses into host pointers.
 if(addr==0||addr>=GUEST_BYTES||GUEST_BYTES-(usize)addr<4u)return TEST_BAD_POINTER;
 char port[9];usize n=0;
 for(;n<8u;n++){
  if((usize)addr+n>=GUEST_BYTES)return TEST_BAD_POINTER;
  u8 c=rt.guest[(usize)addr+n];
  if(!c)break;
  if(c<32u||c>126u)return TEST_BAD_POINTER;
  port[n]=(char)c;
 }
 if(n==8u)return TEST_BAD_POINTER; // no NUL terminator in allowed eight bytes
 port[n]=0;
 if(!equal(port,"sm:"))return TEST_UNKNOWN_SERVICE;
 return alloc_handle(H_SM,out);
}
static int service_name(const u8*b,const char*s){
 usize i=0;for(;i<8u&&s[i];i++)if(b[i]!=(u8)s[i])return 0;
 if(s[i])return 0;
 for(;i<8u;i++)if(b[i]!=0u)return 0;
 return 1;
}
static enum Kind kind_for(const u8*b){
 if(service_name(b,"hid"))return H_HID;
 if(service_name(b,"time:u"))return H_TIME;
 if(service_name(b,"vi:u"))return H_VI;
 if(service_name(b,"nvdrv"))return H_NVDRV;
 return H_NONE;
}
static u32 parse(const u8*msg,usize max,u32*cmd,u32*token,usize*end){
 if(!msg||max<TLS_BYTES||!cmd||!token||!end)return TEST_BAD_MESSAGE;
 u32 h0=read32(msg,0),h1=read32(msg,4),raw_words=h1&1023u;
 if((h0&65535u)!=4u&&(h0&65535u)!=6u)return TEST_UNSUPPORTED;
 if((h0&0xffff0000u)||(h1&0xfffffc00u))return TEST_UNSUPPORTED;
 if(raw_words<8u||raw_words>(TLS_BYTES-16u)/4u)return TEST_BAD_MESSAGE;
 *end=16u+(usize)raw_words*4u;
 if(read32(msg,16)!=CMIF_SFCI||msg[20]>1u||msg[21]!=0||msg[22]!=0||msg[23]!=0)
  return TEST_BAD_MESSAGE;
 *cmd=read32(msg,24);*token=read32(msg,28);
 return TEST_OK;
}
static void reply(u32 result,u32 token,u32 moved){
 u8*b=rt.thread.tls;
 zero(b,TLS_BYTES);
 write32(b,0,0u); // HIPC response
 write32(b,4,8u|(moved?0x80000000u:0u));
 if(moved){write32(b,8,1u<<5);write32(b,12,moved);}
 write32(b,16,CMIF_SFCO);write16(b,20,1u);
 write32(b,24,result);write32(b,28,token);
}
static u32 svc_send(u32 handle){
 struct Hentry*h=lookup(handle);
 if(!h)return TEST_BAD_HANDLE;
 u32 cmd=0,token=0;usize end=0;
 u32 pr=parse(rt.thread.tls,TLS_BYTES,&cmd,&token,&end);
 if(pr!=TEST_OK)return pr;
 u32 result=TEST_UNSUPPORTED,moved=0;
 if(h->kind==H_SM){
  if(cmd==0)result=TEST_OK; // fake Initialize
  else if(cmd==1){
   if(end<40u)result=TEST_BAD_MESSAGE;
   else {
    enum Kind service=kind_for(rt.thread.tls+32u);
    result=service==H_NONE?TEST_UNKNOWN_SERVICE:alloc_handle(service,&moved);
   }
  }
 }
 // Kernel-level SendSyncRequest can succeed even if the service rejects the
 // command. Service error lives in the CMIF reply, not in guest register X0.
 reply(result,token,moved);
 return TEST_OK;
}
static void dispatch(u32 svc,struct Registers*r){
 u32 rc=TEST_UNSUPPORTED,h=0;
 switch(svc){
  case 0x1fu:rc=guest_port(r->x[1],&h);r->x[1]=rc==TEST_OK?h:0;break;
  case 0x21u:rc=svc_send((u32)r->x[0]);break;
  case 0x16u:rc=release((u32)r->x[0]);break;
  default:break;
 }
 r->x[0]=rc;
}
static void request(u32 command,u32 token,const char*service){
 u8*b=rt.thread.tls;zero(b,TLS_BYTES);
 write32(b,0,4u);write32(b,4,service?10u:8u);
 write32(b,16,CMIF_SFCI);write16(b,20,1u);
 write32(b,24,command);write32(b,28,token);
 if(service)for(unsigned i=0;i<8u&&service[i];i++)b[32u+i]=(u8)service[i];
}
static u32 call_send(u32 handle){struct Registers r={{0}};r.x[0]=handle;dispatch(0x21,&r);return (u32)r.x[0];}
static u32 call_close(u32 handle){struct Registers r={{0}};r.x[0]=handle;dispatch(0x16,&r);return (u32)r.x[0];}
static u32 connect(const char*name,u32*out){
 // Fake guest-side memory at offset 0x100, no host pointers crossing boundary.
 zero(rt.guest+256u,16u);
 for(unsigned i=0;i<8u&&name[i];i++)rt.guest[256u+i]=(u8)name[i];
 struct Registers r={{0}};r.x[1]=256u;dispatch(0x1f,&r);*out=(u32)r.x[1];return (u32)r.x[0];
}
static int selftest(void){
 reset();struct Registers r={{0}};
 r.x[1]=GUEST_BYTES-1u;dispatch(0x1f,&r);
 if(r.x[0]!=TEST_BAD_POINTER)return fail("GUEST_POINTER_BOUNDS");
 u32 sm=0,hid=0;
 if(connect("nope",&sm)!=TEST_UNKNOWN_SERVICE||sm)return fail("UNKNOWN_PORT");
 if(connect("sm:",&sm)!=TEST_OK||!lookup(sm))return fail("SVC_CONNECT_NAMED_PORT");
 request(0,0x10203040u,0);
 if(call_send(sm)!=TEST_OK||read32(rt.thread.tls,16)!=CMIF_SFCO||
    read32(rt.thread.tls,24)!=TEST_OK||read32(rt.thread.tls,28)!=0x10203040u)
  return fail("SVC_SEND_TLS_INIT");
 request(1,42u,"hid");
 if(call_send(sm)!=TEST_OK||read32(rt.thread.tls,16)!=CMIF_SFCO||
    (read32(rt.thread.tls,4)&0x80000000u)==0||read32(rt.thread.tls,8)!=(1u<<5))
  return fail("SERVICE_HANDLE_MARSHAL");
 hid=read32(rt.thread.tls,12);
 if(!lookup(hid)||lookup(hid)->kind!=H_HID)return fail("HID_HANDLE");
 request(42,11u,0);
 if(call_send(hid)!=TEST_OK||read32(rt.thread.tls,24)!=TEST_UNSUPPORTED)
  return fail("KERNEL_VS_SERVICE_RESULT");
 request(1,77u,"unknown");
 if(call_send(sm)!=TEST_OK||read32(rt.thread.tls,24)!=TEST_UNKNOWN_SERVICE)
  return fail("SERVICE_NOT_FOUND_REPLY");
 request(1,88u,"hid");
 write32(rt.thread.tls,0,4u|(1u<<16));
 if(call_send(sm)!=TEST_UNSUPPORTED)return fail("UNSUPPORTED_DESCRIPTOR");
 request(0,1u,0);
 write32(rt.thread.tls,4,1023u);
 if(call_send(sm)!=TEST_BAD_MESSAGE)return fail("RAW_WORDS_BOUNDS");
 if(call_close(hid)!=TEST_OK||call_close(hid)!=TEST_BAD_HANDLE)return fail("STALE_HANDLE_CLOSE");
 if(call_send(hid)!=TEST_BAD_HANDLE)return fail("STALE_HANDLE_SEND");
 if(call_close(sm)!=TEST_OK||rt.active!=0)return fail("FINAL_HANDLE_LEAK");
 dispatch(0x99u,&r);if(r.x[0]!=TEST_UNSUPPORTED)return fail("UNKNOWN_SVC_ALLOWED");
 if(!canaries())return fail("TLS_CANARY");
 say("SVC_BRIDGE_SYNTHETIC=PASS SVC_CONNECT_1F=PASS SVC_SEND_21=PASS SVC_CLOSE_16=PASS\n");
 say("HIPC_TLS_256=PASS KERNEL_SERVICE_RESULT_SEPARATED=PASS GUEST_POINTERS_BOUNDED=PASS\n");
 say("HANDLE_LIFECYCLE=PASS STUB_SERVICES=YES REAL_GUEST_SVC_TRAP=NOT_IMPLEMENTED\n");
 return 0;
}
static u32 random_state=0x264a2bb6u;
static u32 rng(void){random_state^=random_state<<13;random_state^=random_state>>17;random_state^=random_state<<5;return random_state;}
static int fuzz(void){
 reset();u32 sm=0;
 if(connect("sm:",&sm)!=TEST_OK)return fail("FUZZ_CONNECT");
 for(unsigned n=0;n<50000u;n++){
  request((n&1u)?1u:0u,n,(n&1u)?"hid":0);
  unsigned flips=1u+rng()%6u;
  for(unsigned j=0;j<flips;j++){
   unsigned at=rng()%TLS_BYTES;rt.thread.tls[at]^=(u8)(1u<<(rng()%8u));
  }
  if(call_send(sm)==TEST_OK&&read32(rt.thread.tls,16)==CMIF_SFCO&&
     (read32(rt.thread.tls,4)&0x80000000u)){
   u32 h=read32(rt.thread.tls,12);
   if(call_close(h)!=TEST_OK)return fail("FUZZ_MOVE_HANDLE");
  }
  if(rt.active!=1u||!canaries())return fail("FUZZ_LEAK_OR_TLS_CORRUPTION");
 }
 if(call_close(sm)!=TEST_OK||rt.active)return fail("FUZZ_FINAL_CLOSE");
 say("SVC_BRIDGE_FUZZ=PASS ITERATIONS=50000 CANARY=PASS ACTIVE_HANDLES=0\n");
 return 0;
}
static int run(int argc,char**argv){
 say("RE4NX_PHASE5_SVC=1 ARCH=");say(TARGET_ARCH?"AARCH64":"HOST");
 say(" GUEST_EXECUTION=DISABLED HORIZON_ABI=SIMULATED\n");
 if(argc>1&&equal(argv[1],"--fuzz")){
  if(fuzz())return 1;
 }else if(argc==1||equal(argv[1],"--selftest")){
  if(selftest())return 1;
 }else return fail("UNKNOWN_ARGUMENT");
 say("PHASE5_FINAL=PASS\n");return 0;
}
#if TARGET_ARCH
void phase5_main(long*stack){terminate(run((int)stack[0],(char**)(stack+1)));}
#else
int main(int argc,char**argv){return run(argc,argv);}
#endif
