// RE4NX-R36S Phase 3: synthetic HLE IPC + native ARM64 call-through.
// This is NOT a Nintendo Switch emulator; commercial executables are never run.
// MIT license. Pure Linux ARM64 syscalls; no libc or guest assets required.
#if defined(__aarch64__)
typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef unsigned long usize;
#define NATIVE_ARM64 1
static long sys6(long n,long a,long b,long c,long d,long e,long f){
 register long x0 __asm__("x0")=a,x1 __asm__("x1")=b,x2 __asm__("x2")=c;
 register long x3 __asm__("x3")=d,x4 __asm__("x4")=e,x5 __asm__("x5")=f;
 register long x8 __asm__("x8")=n;
 __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1),"r"(x2),"r"(x3),"r"(x4),"r"(x5),"r"(x8) : "memory","cc");
 return x0;
}
#define NR_WRITE 64
#define NR_MMAP 222
#define NR_MPROTECT 226
#define NR_MUNMAP 215
#define NR_EXIT 93
static void *reserve_pages(usize len,int prot){return (void*)sys6(NR_MMAP,0,(long)len,prot,0x22,-1,0);}
static long protect_pages(void*p,usize len,int prot){return sys6(NR_MPROTECT,(long)p,(long)len,prot,0,0,0);}
static long release_pages(void*p,usize len){return sys6(NR_MUNMAP,(long)p,(long)len,0,0,0,0);}
static void emit(const char*p,usize n){sys6(NR_WRITE,1,(long)p,(long)n,0,0,0);}
__attribute__((noreturn)) static void halt(int rc){sys6(NR_EXIT,rc,0,0,0,0,0);__builtin_unreachable();}
void phase3_main(long*stack);
__asm__(".global _start\n.type _start,%function\n_start:\n mov x0, sp\n b phase3_main\n");
#else
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef unsigned long usize;
#define NATIVE_ARM64 0
static void *reserve_pages(usize len,int prot){return mmap(0,len,prot,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);}
static long protect_pages(void*p,usize len,int prot){return mprotect(p,len,prot);}
static long release_pages(void*p,usize len){return munmap(p,len);}
static void emit(const char*p,usize n){(void)write(1,p,n);}
#endif

#define MiB (1024ull*1024ull)
#define MAX_PROCESS_MIB 768u
#define MAX_HANDLES 64u
#define MAGIC_REQUEST 0x49434653u /* 'SFCI' only as a CMIF-inspired marker */
#define MAGIC_RESPONSE 0x4f434653u /* 'SFCO'; this is NOT a real HIPC encoding */
#define RC_OK 0u
#define R_INVALID 0x101u
#define R_NOT_FOUND 0x102u
#define R_NO_HANDLES 0x103u
#define R_UNIMPLEMENTED 0x104u
#define R_NO_MEMORY 0x105u

static usize length(const char*s){usize i=0;while(s[i])i++;return i;}
static void logstr(const char*s){emit(s,length(s));}
static int equal(const char*a,const char*b){while(*a&&*b){if(*a!=*b)return 0;a++;b++;}return *a==*b;}
static int matched8(const char*a,const char*b){for(unsigned i=0;i<8;i++){char c=b[i];if(!c)return a[i]==0; if(a[i]!=c)return 0;}return a[8]==0;}
static void bytes_zero(u8*p,usize n){for(usize i=0;i<n;i++)p[i]=0;}
static int bad_pointer(const void*p){return (usize)p>=(usize)-4095L;}
static int failed(const char*what){logstr("PHASE3_FINAL=FAIL REASON=");logstr(what);logstr("\n");return 1;}

// Memory rights for the synthetic process. Policy is W^X; this is not
// implemented using Switch SVCs or the real game's allocator.
static int allow_permissions(int rights){return (rights&6)!=6 && (rights&~7)==0;}
static u64 virtual_heap_mib=0;
static u32 set_virtual_heap_mib(u64 mib){
 // A fixed 64 MiB book-keeping allowance for loader, stacks, and the HLE table.
 if(mib>MAX_PROCESS_MIB-64u)return R_NO_MEMORY;
 virtual_heap_mib=mib;
 return RC_OK;
}

enum Service { S_FREE=0,S_SM=1,S_HID,S_TIME,S_VI,S_NVDRV,S_FS,S_APPLET };
static const struct {const char*name;enum Service kind;} services[]={
 {"hid",S_HID},{"time:u",S_TIME},{"vi:u",S_VI},
 {"nvdrv",S_NVDRV},{"fsp-srv",S_FS},{"appletOE",S_APPLET}
};
struct Slot {u32 generation;enum Service kind;};
static struct Slot slots[MAX_HANDLES];
static u32 slot_count=0;
static u32 handle_of(unsigned idx){return (slots[idx].generation<<8)|(idx+1u);}
static struct Slot*find_slot(u32 handle){
 unsigned idx=(handle&255u);
 if(idx==0||idx>MAX_HANDLES)return 0;
 idx--;
 if(slots[idx].kind==S_FREE||slots[idx].generation!=(handle>>8))return 0;
 return &slots[idx];
}
static u32 open_slot(enum Service kind,u32*out){
 if(kind==S_FREE||!out)return R_INVALID;
 for(unsigned i=0;i<MAX_HANDLES;i++)if(slots[i].kind==S_FREE){
  slots[i].generation=(slots[i].generation+1u)&0xffffffu;
  if(!slots[i].generation)slots[i].generation=1;
  slots[i].kind=kind;
  *out=handle_of(i);
  slot_count++;
  return RC_OK;
 }
 return R_NO_HANDLES;
}
static u32 svc_connect_to_named_port(const char*name,u32*out){
 if(!name||!equal(name,"sm:"))return R_NOT_FOUND;
 return open_slot(S_SM,out);
}
static u32 svc_close_handle(u32 handle){
 struct Slot*s=find_slot(handle);
 if(!s)return R_INVALID;
 s->kind=S_FREE;
 slot_count--;
 return RC_OK;
}
// Synthetic fixed request envelope: command 0 initializes sm: session;
// command 1 asks sm: to issue a service handle. Other services are STUBS
// and must explicitly return unimplemented (no false compatibility).
struct Request {u32 magic;u32 command;u32 count;u32 args[4];};
struct Response {u32 magic;u32 result;u32 count;u32 values[4];};
static void write_service_name(struct Request*r,const char*name){
 int ended=0;
 for(unsigned i=0;i<8;i++){
  char c=0;
  if(!ended){c=name[i];if(!c)ended=1;}
  ((u8*)r->args)[i]=(u8)c;
 }
}
static u32 svc_send_sync_request(u32 handle,const struct Request*r,struct Response*out){
 if(!r||!out)return R_INVALID;
 bytes_zero((u8*)out,sizeof(*out));out->magic=MAGIC_RESPONSE;
 struct Slot*s=find_slot(handle);
 if(!s){out->result=R_INVALID;return R_INVALID;}
 if(r->magic!=MAGIC_REQUEST||r->count>4){out->result=R_INVALID;return R_INVALID;}
 if(s->kind!=S_SM){out->result=R_UNIMPLEMENTED;return R_UNIMPLEMENTED;}
 if(r->command==0&&r->count==0){out->result=RC_OK;return RC_OK;}
 if(r->command!=1||r->count!=2){out->result=R_UNIMPLEMENTED;return R_UNIMPLEMENTED;}
 char name[9];for(unsigned i=0;i<8;i++)name[i]=((const char*)r->args)[i];name[8]=0;
 for(unsigned i=0;i<sizeof(services)/sizeof(services[0]);i++)
  if(matched8(name,services[i].name)){
   u32 h=0;u32 result=open_slot(services[i].kind,&h);
   out->result=result;
   if(result==RC_OK){out->count=1;out->values[0]=h;}
   return result;
  }
 out->result=R_NOT_FOUND;
 return R_NOT_FOUND;
}
static struct Request request(u32 cmd,u32 count){
 struct Request r;bytes_zero((u8*)&r,sizeof(r));r.magic=MAGIC_REQUEST;r.command=cmd;r.count=count;return r;
}
static u32 get_service(u32 sm,const char*name,u32*out){
 struct Request req=request(1,2);struct Response reply;
 write_service_name(&req,name);
 u32 rc=svc_send_sync_request(sm,&req,&reply);
 if(rc==RC_OK&&out)*out=reply.values[0];
 return rc;
}
static int ipc_selftest(void){
 u32 sm=0,hid=0,vi=0,nvdrv=0,service=0;
 if(svc_connect_to_named_port("sm:",&sm)!=RC_OK)return failed("SM_CONNECT");
 if(svc_connect_to_named_port("wrong:",&service)!=R_NOT_FOUND)return failed("UNKNOWN_PORT_ACCEPTED");
 struct Request init=request(0,0);struct Response reply;
 if(svc_send_sync_request(sm,&init,&reply)!=RC_OK)return failed("SM_INITIALIZE");
 if(get_service(sm,"hid",&hid)!=RC_OK||get_service(sm,"vi:u",&vi)!=RC_OK||
    get_service(sm,"nvdrv",&nvdrv)!=RC_OK)return failed("KNOWN_SERVICES");
 if(get_service(sm,"fake123",&service)!=R_NOT_FOUND)return failed("UNKNOWN_SERVICE_ACCEPTED");
 struct Request unknown=request(42,0);
 if(svc_send_sync_request(hid,&unknown,&reply)!=R_UNIMPLEMENTED)return failed("FALSE_HID_IMPLEMENTATION");
 struct Request corrupted=request(0,0);corrupted.magic=0;
 if(svc_send_sync_request(sm,&corrupted,&reply)!=R_INVALID)return failed("BAD_CMIF_ACCEPTED");
 u32 stale=hid;
 if(svc_close_handle(hid)!=RC_OK)return failed("HID_CLOSE");
 if(svc_close_handle(hid)!=R_INVALID)return failed("DOUBLE_CLOSE");
 if(get_service(sm,"time:u",&service)!=RC_OK)return failed("TIME_OPEN");
 if(svc_send_sync_request(stale,&unknown,&reply)!=R_INVALID)return failed("STALE_HANDLE_ACCEPTED");
 if(svc_close_handle(service)!=RC_OK||svc_close_handle(nvdrv)!=RC_OK||
    svc_close_handle(vi)!=RC_OK||svc_close_handle(sm)!=RC_OK||slot_count!=0)
  return failed("HANDLE_LEAK");
 // Exhaust the handle table and verify the next request is rejected.
 u32 ids[MAX_HANDLES];
 for(unsigned i=0;i<MAX_HANDLES;i++)
  if(open_slot(S_HID,&ids[i])!=RC_OK)return failed("HANDLE_LIMIT_FILL");
 u32 unexpected=0;
 if(open_slot(S_HID,&unexpected)!=R_NO_HANDLES)return failed("HANDLE_LIMIT_NOT_ENFORCED");
 for(unsigned i=0;i<MAX_HANDLES;i++)
  if(svc_close_handle(ids[i])!=RC_OK)return failed("HANDLE_LIMIT_RELEASE");
 if(slot_count!=0)return failed("HANDLE_LIMIT_LEAK");
 logstr("HLE_IPC_SYNTHETIC=PASS SERVICE_LOOKUP=PASS STUB_UNIMPLEMENTED=PASS\n");
 logstr("HANDLE_TABLE=PASS MAX=64 OVERFLOW=REJECTED INVALID_HANDLE=REJECTED STALE_HANDLE=REJECTED\n");
 return 0;
}
static int budget_test(void){
 if(set_virtual_heap_mib(704)!=RC_OK||virtual_heap_mib!=704)return failed("BUDGET_VALID");
 if(set_virtual_heap_mib(705)!=R_NO_MEMORY||virtual_heap_mib!=704)return failed("BUDGET_OVERFLOW");
 if(allow_permissions(6)||!allow_permissions(5)||!allow_permissions(3)||allow_permissions(8))
  return failed("WX_POLICY");
 const usize n=8u*MiB;
 u8*p=(u8*)reserve_pages(n,3);
 if(bad_pointer(p))return failed("RAM_MMAP");
 for(usize i=0;i<n;i+=4096u)p[i]=(u8)(i>>12);
 if(protect_pages(p,n,1)!=0){release_pages(p,n);return failed("RAM_READ_ONLY_MPROTECT");}
 for(usize i=0;i<n;i+=4096u)if(p[i]!=(u8)(i>>12)){
  release_pages(p,n);return failed("RAM_VERIFY");
 }
 if(release_pages(p,n)!=0)return failed("RAM_UNMAP");
 logstr("VIRTUAL_MEMORY_BUDGET=PASS CAP_MIB=768 HEAP_ALLOWED_MIB=704 HEAP_DENIED_MIB=705\n");
 logstr("MEMORY_POLICY_WX=PASS RWX_DENIED=YES\n");
 logstr("RAM_COMMIT_READBACK=PASS MIB=8 PAGES=2048\n");
 return 0;
}
#if NATIVE_ARM64
static void clear_instruction_cache(u8*p,usize size){
 u64 ctr;__asm__ volatile("mrs %0,ctr_el0" : "=r"(ctr));
 const usize dl=(usize)4u<<((ctr>>16)&15u),il=(usize)4u<<(ctr&15u);
 usize start=(usize)p,end=start+size;
 for(usize addr=start&~(dl-1u);addr<end;addr+=dl)
  __asm__ volatile("dc cvau,%0" :: "r"(addr) : "memory");
 __asm__ volatile("dsb ish" ::: "memory");
 for(usize addr=start&~(il-1u);addr<end;addr+=il)
  __asm__ volatile("ic ivau,%0" :: "r"(addr) : "memory");
 __asm__ volatile("dsb ish\n isb" ::: "memory");
}
static u64 callback_id(u64 command){return command==0x42u?0x7265346eull:0;}
static int jit_bridge_test(void){
 // Public ARM64 fixture: save LR; call function pointer x1, restore LR;
 // return result. This proves ABI and RX transition, NOT guest SVC handling.
 const u32 code[]={0xa9bf7bfdu,0xd63f0020u,0xa8c17bfdu,0xd65f03c0u};
 u8*page=(u8*)reserve_pages(4096u,3);
 if(bad_pointer(page))return failed("BRIDGE_MMAP");
 for(usize i=0;i<sizeof(code);i++)page[i]=((const u8*)code)[i];
 clear_instruction_cache(page,sizeof(code));
 if(protect_pages(page,4096u,5)!=0){release_pages(page,4096u);return failed("BRIDGE_MPROTECT_RX");}
 u64(*entry)(u64,u64(*)(u64))=(u64(*)(u64,u64(*)(u64)))(void*)page;
 const u64 answer=entry(0x42u,callback_id);
 if(release_pages(page,4096u)!=0||answer!=0x7265346eull)return failed("BRIDGE_CALL_RESULT");
 logstr("ARM64_SYNTHETIC_HOST_CALL=PASS AAPCS64=PASS RX_ONLY=PASS\n");
 return 0;
}
#else
static int jit_bridge_test(void){
 logstr("ARM64_SYNTHETIC_HOST_CALL=SKIP HOST_VALIDATION_ONLY=YES\n");return 0;
}
#endif
static int stress_test(void){
 u32 sm=0;
 if(svc_connect_to_named_port("sm:",&sm)!=RC_OK)return failed("STRESS_SM");
 for(unsigned i=0;i<10000;i++){
  u32 handle=0;
  if(get_service(sm,"hid",&handle)!=RC_OK||svc_close_handle(handle)!=RC_OK){
   svc_close_handle(sm);return failed("STRESS_HANDLE");
  }
 }
 if(svc_close_handle(sm)!=RC_OK||slot_count!=0)return failed("STRESS_HANDLE_LEAK");
 logstr("HANDLE_STRESS=PASS CYCLES=10000 LIVE_HANDLES=0\n");return 0;
}
static int run(int argc,char**argv){
 logstr("RE4NX_PHASE3_NATIVE=1 ARCH=");
 logstr(NATIVE_ARM64?"AARCH64":"HOST_VALIDATION");
 logstr(" ABI=LINUX_SYSV\nGUEST_GAME_EXECUTION=DISABLED\n");
 logstr("HORIZON_HLE=SIMULATOR_ONLY NVN_TO_GLES3=NOT_IMPLEMENTED\n");
 if(argc>1&&equal(argv[1],"--stress")){
  int r=stress_test();if(!r)logstr("PHASE3_STRESS_FINAL=PASS\n");return r;
 }
 if(argc>1&&!equal(argv[1],"--selftest")){
  logstr("USAGE: re4nx-hle-aarch64 --selftest | --stress\n");return failed("UNKNOWN_OPTION");
 }
 if(ipc_selftest()||budget_test()||jit_bridge_test())return 1;
 logstr("PHASE3_FINAL=PASS\n");return 0;
}
#if NATIVE_ARM64
void phase3_main(long*stack){halt(run((int)stack[0],(char**)(stack+1)));}
#else
int main(int argc,char**argv){return run(argc,argv);}
#endif
