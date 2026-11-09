// RE4NX R36S Phase 4: subset of Horizon HIPC+CMIF wire framing.
// Based on publicly documented wire headers, not Nintendo code.
// DOES NOT launch commercial NSO/NRO, emulate NVN, or implement Switch SVC ABI.
// The fake sm: service and handles exist only to exercise the parser safely.
// Linux AArch64: freestanding static, syscalls for stdout/exit only.
#if defined(__aarch64__) && !defined(RE4NX_HOST_TEST)
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long usize;
#define ON_AARCH64 1
static long syscall3(long n,long a,long b,long c){
 register long x0 __asm__("x0")=a,x1 __asm__("x1")=b,x2 __asm__("x2")=c;
 register long x8 __asm__("x8")=n;
 __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1),"r"(x2),"r"(x8) : "memory","cc");
 return x0;
}
static void output(const char*buf,usize n){(void)syscall3(64,1,(long)buf,(long)n);}
__attribute__((noreturn)) static void shutdown(int rc){(void)syscall3(93,rc,0,0);__builtin_unreachable();}
void phase4_main(long *stack);
__asm__(".global _start\n.type _start,%function\n_start:\n mov x0, sp\n b phase4_main\n");
#else
#include <stdio.h>
#include <unistd.h>
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long usize;
#define ON_AARCH64 0
static void output(const char*buf,usize n){(void)fwrite(buf,1,n,stdout);}
#endif

#define MSG_SIZE 256u
#define HANDLE_COUNT 64u
#define HIPC_REQUEST_TYPE 6u /* Invoke2Method; accept type 4 for older firmware */
#define HIPC_ALT_REQUEST_TYPE 4u
#define CMIF_IN 0x49434653u  /* bytes: SFCI */
#define CMIF_OUT 0x4f434653u /* bytes: SFCO */
#define RE4_OK 0u
#define R_INVALID 0x101u
#define R_NOT_FOUND 0x102u
#define R_NO_HANDLES 0x103u
#define R_UNIMPLEMENTED 0x104u
#define R_NOT_SUPPORTED 0x105u

static usize length(const char*s){usize n=0;while(s[n])n++;return n;}
static void logstr(const char*s){output(s,length(s));}
static int eq(const char*a,const char*b){while(*a&&*b){if(*a!=*b)return 0;a++;b++;}return *a==*b;}
static int failure(const char*reason){logstr("PHASE4_FINAL=FAIL REASON=");logstr(reason);logstr("\n");return 1;}
static void clear(u8*b,usize n){for(usize i=0;i<n;i++)b[i]=0;}
static void put16(u8*b,usize off,u16 x){b[off]=(u8)x;b[off+1u]=(u8)(x>>8);}
static void put32(u8*b,usize off,u32 x){for(unsigned i=0;i<4;i++)b[off+i]=(u8)(x>>(8u*i));}
static u16 get16(const u8*b,usize off){return (u16)((u16)b[off]|((u16)b[off+1u]<<8));}
static u32 get32(const u8*b,usize off){return (u32)b[off]|((u32)b[off+1u]<<8)|((u32)b[off+2u]<<16)|((u32)b[off+3u]<<24);}

struct Parsed {u32 type,version,command,token;usize raw_start,raw_end;};
// Header0 bit allocation: type(0..15), pointer(16..19), send(20..23),
// receive(24..27), exchange(28..31). Header1: raw words(0..9),
// receive-list count(10..13), reserved(14..19), receive offset(20..30),
// special(31). Only a descriptor-free subset is implemented on purpose.
static u32 parse_request(const u8*src,usize size,struct Parsed *p){
 if(!src||!p||size<8u||size>MSG_SIZE)return R_INVALID;
 const u32 h0=get32(src,0),h1=get32(src,4);
 const u32 type=h0&65535u,words=h1&1023u;
 if(type!=HIPC_REQUEST_TYPE&&type!=HIPC_ALT_REQUEST_TYPE)return R_NOT_SUPPORTED;
 if(h0>>16u)return R_NOT_SUPPORTED; /* no buffer descriptors yet */
 if((h1&0xfffffc00u)!=0u)return R_NOT_SUPPORTED; /* no receive list/special */
 if(words<8u)return R_INVALID;
 const usize raw=16u; /* zero-descriptor raw data is aligned to 16 */
 if((usize)words>(size-raw)/4u)return R_INVALID;
 if(raw+16u>size)return R_INVALID;
 const usize end=raw+(usize)words*4u;
 if(get32(src,raw)!=CMIF_IN)return R_INVALID;
 const u16 version=get16(src,raw+4u),reserved=get16(src,raw+6u);
 if(version>1u||reserved!=0u)return R_NOT_SUPPORTED;
 p->type=type;p->version=version;p->command=get32(src,raw+8u);
 p->token=get32(src,raw+12u);p->raw_start=raw;p->raw_end=end;
 return RE4_OK;
}

// A generation-tagged handle-table model. Not Horizon kernel handles.
enum Service { SERVICE_EMPTY=0,SERVICE_SM,SERVICE_HID,SERVICE_TIME,SERVICE_VI,SERVICE_NVDRV,SERVICE_FS,SERVICE_APPLET};
struct Handle {u32 generation;enum Service service;};
static struct Handle handles[HANDLE_COUNT];
static u32 live_handles=0;
static u32 make_handle(unsigned idx){return (handles[idx].generation<<8u)|(idx+1u);}
static struct Handle*get_handle(u32 id){
 const u32 ix=id&255u;
 if(ix==0||ix>HANDLE_COUNT)return 0;
 struct Handle*h=&handles[ix-1u];
 return h->service!=SERVICE_EMPTY&&h->generation==(id>>8u)?h:0;
}
static u32 open_handle(enum Service service,u32*out){
 if(!out||service==SERVICE_EMPTY)return R_INVALID;
 for(unsigned i=0;i<HANDLE_COUNT;i++)if(handles[i].service==SERVICE_EMPTY){
  handles[i].generation=(handles[i].generation+1u)&0x00ffffffu;
  if(!handles[i].generation)handles[i].generation=1u;
  handles[i].service=service;
  *out=make_handle(i);live_handles++;return RE4_OK;
 }
 return R_NO_HANDLES;
}
static u32 close_handle(u32 id){
 struct Handle*h=get_handle(id);
 if(!h)return R_INVALID;
 h->service=SERVICE_EMPTY;live_handles--;return RE4_OK;
}
static u32 connect_port(const char*port,u32*out){
 if(!port||!eq(port,"sm:"))return R_NOT_FOUND;
 return open_handle(SERVICE_SM,out);
}
static int name_eq(const u8*raw,const char*expected){
 unsigned i=0;
 while(i<8u&&expected[i]){if(raw[i]!=(u8)expected[i])return 0;i++;}
 if(expected[i])return 0;
 for(;i<8u;i++)if(raw[i]!=0u)return 0;
 return 1;
}
static enum Service get_service_kind(const u8*name){
 if(name_eq(name,"hid"))return SERVICE_HID;
 if(name_eq(name,"time:u"))return SERVICE_TIME;
 if(name_eq(name,"vi:u"))return SERVICE_VI;
 if(name_eq(name,"nvdrv"))return SERVICE_NVDRV;
 if(name_eq(name,"fsp-srv"))return SERVICE_FS;
 if(name_eq(name,"appletOE"))return SERVICE_APPLET;
 return SERVICE_EMPTY;
}

// Response subset: HIPC header + optional special(move-handle) + padding
// + CMIF out header, with explicit zero padding. No pointers/domains.
static u32 response(u8*dst,usize capacity,u32 rc,u32 token,u32 moved,usize*out_size){
 if(!dst||capacity<48u||!out_size)return R_INVALID;
 clear(dst,capacity);
 put32(dst,0,0u); /* response type */
 put32(dst,4,8u|(moved?(1u<<31u):0u)); /* raw CMIF + padding */
 if(moved){put32(dst,8,1u<<5u);put32(dst,12,moved);} /* 1 move handle */
 put32(dst,16,CMIF_OUT);
 put16(dst,20,1u); /* version */
 put32(dst,24,rc);
 put32(dst,28,token); /* modeled token echo (not an InterfaceId) */
 *out_size=48u;
 return RE4_OK;
}
// SVC-like host adapter for a restricted synthetic fake sm: session.
// Real SVC handlers, buffers, domain conversion and NVN are NOT implemented.
static u32 send_sync_request(u32 handle,const u8*req,usize len,u8*dst,usize cap,usize*out_len){
 struct Handle*h=get_handle(handle);
 if(!h)return R_INVALID;
 struct Parsed p;
 const u32 parsed=parse_request(req,len,&p);
 if(parsed!=RE4_OK)return parsed;
 if(h->service!=SERVICE_SM){
  response(dst,cap,R_UNIMPLEMENTED,p.token,0u,out_len);
  return R_UNIMPLEMENTED;
 }
 u32 rc=R_UNIMPLEMENTED,moved=0u;
 if(p.command==0u){
  // Modeled Initialize; deliberately no PID forwarding or privilege bits.
  rc=RE4_OK;
 }else if(p.command==1u){
  const usize payload=p.raw_start+16u;
  if(payload+8u>p.raw_end)rc=R_INVALID;
  else {
   enum Service service=get_service_kind(req+payload);
   if(service==SERVICE_EMPTY)rc=R_NOT_FOUND;
   else rc=open_handle(service,&moved);
  }
 }
 if(response(dst,cap,rc,p.token,moved,out_len)!=RE4_OK){
  if(moved)close_handle(moved);
  return R_INVALID;
 }
 return rc;
}
static usize make_request(u8*dst,u32 command,u32 version,u32 token,const char*name){
 clear(dst,MSG_SIZE);
 put32(dst,0,HIPC_REQUEST_TYPE);
 put32(dst,4,name?10u:8u); /* payload 8 or 0, plus mandatory padding */
 put32(dst,16,CMIF_IN);
 put16(dst,20,(u16)version);
 put32(dst,24,command);
 put32(dst,28,token);
 if(name){
  unsigned i=0;
  for(;i<8u&&name[i];i++)dst[32u+i]=(u8)name[i];
 }
 return name?56u:48u;
}
static int basic_tests(void){
 u8 req[MSG_SIZE],out[MSG_SIZE];usize outlen=0;
 u32 sm=0,hid=0,vi=0;
 if(connect_port("sm:",&sm)!=RE4_OK)return failure("CONNECT_SM");
 if(connect_port("bad:",&hid)!=R_NOT_FOUND)return failure("BAD_PORT_ACCEPTED");
 usize n=make_request(req,0u,1u,0x77889900u,0);
 if(send_sync_request(sm,req,n,out,MSG_SIZE,&outlen)!=RE4_OK||outlen!=48u||
    get32(out,16)!=CMIF_OUT||get32(out,24)!=RE4_OK||get32(out,28)!=0x77889900u)
  return failure("INIT_CMIF_REPLY");
 n=make_request(req,1u,1u,42u,"hid");
 if(send_sync_request(sm,req,n,out,MSG_SIZE,&outlen)!=RE4_OK||
    (get32(out,4)&(1u<<31u))==0u||get32(out,8)!=(1u<<5u))
  return failure("HID_MOVE_HANDLE");
 hid=get32(out,12);
 if(!get_handle(hid)||get_handle(hid)->service!=SERVICE_HID)return failure("HID_HANDLE_VALUE");
 n=make_request(req,1u,0u,7u,"vi:u");
 put32(req,0,HIPC_ALT_REQUEST_TYPE);
 if(send_sync_request(sm,req,n,out,MSG_SIZE,&outlen)!=RE4_OK)return failure("LEGACY_MESSAGE_TYPE");
 vi=get32(out,12);
 n=make_request(req,1u,1u,0u,"noname");
 if(send_sync_request(sm,req,n,out,MSG_SIZE,&outlen)!=R_NOT_FOUND||get32(out,24)!=R_NOT_FOUND)
  return failure("UNKNOWN_SERVICE_ACCEPTED");
 n=make_request(req,42u,1u,0u,0);
 if(send_sync_request(hid,req,n,out,MSG_SIZE,&outlen)!=R_UNIMPLEMENTED||
    get32(out,24)!=R_UNIMPLEMENTED)return failure("HID_FALSE_IMPLEMENTATION");
 if(close_handle(hid)!=RE4_OK||close_handle(hid)!=R_INVALID)return failure("DOUBLE_CLOSE");
 if(send_sync_request(hid,req,n,out,MSG_SIZE,&outlen)!=R_INVALID)return failure("STALE_HANDLE");
 if(close_handle(vi)!=RE4_OK||close_handle(sm)!=RE4_OK||live_handles!=0u)
  return failure("HANDLE_LEAK");
 logstr("HIPC_CMIF_SYNTHETIC=PASS HEADER_LAYOUT=PASS SFCO=PASS MOVE_HANDLE=PASS\n");
 logstr("HORIZON_SERVICE_MODEL=PASS SM_ONLY=YES OTHER_SERVICES=STUBS\n");
 return 0;
}
static int negative_tests(void){
 u8 req[MSG_SIZE];struct Parsed p;
 usize n=make_request(req,1u,1u,1u,"hid");
 if(parse_request(req,7u,&p)!=R_INVALID)return failure("TRUNCATED_HEADER");
 if(parse_request(req,n-1u,&p)!=R_INVALID)return failure("TRUNCATED_PAYLOAD");
 put32(req,4,1023u);
 if(parse_request(req,n,&p)!=R_INVALID)return failure("OVERSIZED_RAW");
 n=make_request(req,1u,1u,1u,"hid");
 put32(req,0,6u|(1u<<16u));
 if(parse_request(req,n,&p)!=R_NOT_SUPPORTED)return failure("POINTER_DESCRIPTOR_ACCEPTED");
 n=make_request(req,1u,1u,1u,"hid");
 put32(req,4,10u|(1u<<31u));
 if(parse_request(req,n,&p)!=R_NOT_SUPPORTED)return failure("SPECIAL_HANDLE_ACCEPTED");
 n=make_request(req,1u,1u,1u,"hid");
 put32(req,4,10u|(1u<<10u));
 if(parse_request(req,n,&p)!=R_NOT_SUPPORTED)return failure("RECEIVE_LIST_ACCEPTED");
 n=make_request(req,1u,1u,1u,"hid");
 put32(req,16,0u);
 if(parse_request(req,n,&p)!=R_INVALID)return failure("CMIF_MAGIC_ACCEPTED");
 n=make_request(req,1u,3u,1u,"hid");
 if(parse_request(req,n,&p)!=R_NOT_SUPPORTED)return failure("UNKNOWN_VERSION_ACCEPTED");
 n=make_request(req,1u,1u,1u,"hid");
 put16(req,22,1u);
 if(parse_request(req,n,&p)!=R_NOT_SUPPORTED)return failure("RESERVED_CMIF_ACCEPTED");
 n=make_request(req,1u,1u,1u,"hid");
 put32(req,0,0xdeadbeefu);
 if(parse_request(req,n,&p)!=R_NOT_SUPPORTED)return failure("BAD_MSG_TYPE_ACCEPTED");
 logstr("HIPC_NEGATIVE=PASS CASES=10 INVALID_DESCRIPTORS=REJECTED\n");
 return 0;
}
static int handle_stress(void){
 u8 req[MSG_SIZE],out[MSG_SIZE];usize outlen=0;
 u32 sm=0,ids[HANDLE_COUNT],extra=0;
 if(connect_port("sm:",&sm)!=RE4_OK)return failure("STRESS_CONNECT");
 const usize n=make_request(req,1u,1u,0u,"hid");
 for(unsigned i=0;i<HANDLE_COUNT-1u;i++){
  if(send_sync_request(sm,req,n,out,MSG_SIZE,&outlen)!=RE4_OK)return failure("HANDLE_FILL");
  ids[i]=get32(out,12);
 }
 if(send_sync_request(sm,req,n,out,MSG_SIZE,&outlen)!=R_NO_HANDLES)return failure("HANDLE_EXHAUSTION");
 for(unsigned i=0;i<HANDLE_COUNT-1u;i++)if(close_handle(ids[i])!=RE4_OK)return failure("HANDLE_RELEASE");
 if(close_handle(sm)!=RE4_OK||live_handles!=0u)return failure("HANDLE_STRESS_LEAK");
 if(connect_port("sm:",&sm)!=RE4_OK)return failure("STRESS_RECONNECT");
 for(unsigned i=0;i<10000u;i++){
  if(send_sync_request(sm,req,n,out,MSG_SIZE,&outlen)!=RE4_OK)return failure("HANDLE_CYCLE_ALLOC");
  extra=get32(out,12);
  if(close_handle(extra)!=RE4_OK)return failure("HANDLE_CYCLE_FREE");
 }
 if(close_handle(sm)!=RE4_OK||live_handles!=0u)return failure("HANDLE_CYCLE_LEAK");
 logstr("HIPC_HANDLE_STRESS=PASS MAX=64 CYCLES=10000 STALE_REJECTED=YES\n");
 return 0;
}
static u32 prng=0x9e3779b9u;
static u32 rand32(void){prng^=prng<<13u;prng^=prng>>17u;prng^=prng<<5u;return prng;}
static int fuzz_test(void){
 u8 req[MSG_SIZE],out[MSG_SIZE];usize outlen=0;
 u32 sm=0;
 if(connect_port("sm:",&sm)!=RE4_OK)return failure("FUZZ_CONNECT");
 for(unsigned i=0;i<50000u;i++){
  const usize n=make_request(req,(i%3u)==0u?1u:0u,1u,i,"hid");
  u32 changes=1u+rand32()%7u;
  for(u32 j=0;j<changes;j++){
   const usize where=(usize)(rand32()%MSG_SIZE);
   req[where]^=(u8)(1u<<(rand32()%8u));
  }
  const usize length_to_read=(i%11u)==0u?(usize)(rand32()%MSG_SIZE):n;
  const u32 rc=send_sync_request(sm,req,length_to_read,out,MSG_SIZE,&outlen);
  if(rc==RE4_OK){
   if(get32(out,16)!=CMIF_OUT||get32(out,24)!=RE4_OK)return failure("FUZZ_BAD_REPLY");
   if(get32(out,4)&(1u<<31u)){
    const u32 h=get32(out,12);
    if(close_handle(h)!=RE4_OK)return failure("FUZZ_MOVED_HANDLE_LEAK");
   }
  }
  if(live_handles!=1u)return failure("FUZZ_LEAK");
 }
 if(close_handle(sm)!=RE4_OK||live_handles!=0u)return failure("FUZZ_CLOSE");
 logstr("HIPC_FUZZ=PASS ITERATIONS=50000 SEED=0x9e3779b9 BOUNDED=YES\n");
 return 0;
}
static int run(int argc,char**argv){
 logstr("RE4NX_PHASE4_HIPC=1 ARCH=");logstr(ON_AARCH64?"AARCH64":"HOST_VALIDATION");
 logstr(" HORIZON_IMPLEMENTATION=PARTIAL_WIRE_SUBSET GUEST_GAME_EXECUTION=DISABLED\n");
 if(argc>1&&eq(argv[1],"--fuzz")){
  if(fuzz_test())return 1;
  logstr("PHASE4_FINAL=PASS\n");return 0;
 }
 if(argc>1&&!eq(argv[1],"--selftest"))return failure("UNKNOWN_ARGUMENT");
 if(basic_tests()||negative_tests()||handle_stress())return 1;
 logstr("PHASE4_FINAL=PASS\n");return 0;
}
#if ON_AARCH64
void phase4_main(long*stack){shutdown(run((int)stack[0],(char**)(stack+1)));}
#else
int main(int argc,char**argv){return run(argc,argv);}
#endif
