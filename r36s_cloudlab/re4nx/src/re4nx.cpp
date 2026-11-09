// RE4NX-R36S Fase 1: strict NSO validator, RAM budget, synthetic AArch64 test,
// and runtime SDL2 / OpenGL ES 3 capability probe. No copyrighted code.
// Copyright (c) 2026 RE4NX contributors. SPDX-License-Identifier: MIT
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include <chrono>
#include <cctype>
#if defined(__linux__)
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace {
constexpr std::size_t KiB=1024, MiB=KiB*KiB;
constexpr std::size_t kPhysical=1024*MiB;
constexpr std::size_t kHostReserve=256*MiB;
constexpr std::size_t kGuestBudget=kPhysical-kHostReserve; // 768 MiB total process budget
constexpr std::size_t kMaxImage=192*MiB;
constexpr std::size_t kMaxInput=192*MiB;
constexpr std::size_t kMaxSegment=160*MiB;
constexpr uint32_t kNsoHeader=0x100;
struct Error : std::runtime_error { using std::runtime_error::runtime_error; };
void require(bool good, const std::string&why) {if(!good)throw Error(why);}
uint32_t u32(const uint8_t*p){return uint32_t(p[0]) | uint32_t(p[1])<<8 | uint32_t(p[2])<<16 | uint32_t(p[3])<<24;}
void put32(std::vector<uint8_t>&b,std::size_t off,uint32_t v){for(unsigned i=0;i<4;i++)b.at(off+i)=uint8_t(v>>(i*8));}
uint64_t align4096(uint64_t n){return (n+4095u)&~uint64_t(4095u);}

// Small self-contained SHA256 implementation, used only for NSO integrity.
// Standard SHA-256 as defined in FIPS 180-4; no external dependencies.
uint32_t ror(uint32_t x,unsigned n){return (x>>n)|(x<<(32-n));}
std::array<uint8_t,32> sha256(const uint8_t*input,std::size_t len){
 static constexpr uint32_t k[64]={
 0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
 0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
 0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
 0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
 0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
 0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
 0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
 0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
 uint32_t h[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
 uint64_t bitlen=uint64_t(len)*8;
 // Per-block processing; pad in the final two blocks, no large temporary input copy.
 std::size_t blocks=(len/64)+1+(((len%64)>=56)?1:0);
 for(std::size_t blk=0;blk<blocks;blk++){
  uint8_t bytes[64]{};
  for(std::size_t j=0;j<64;j++){
   const std::size_t at=blk*64+j;
   if(at<len)bytes[j]=input[at];
   else if(at==len)bytes[j]=0x80;
  }
  if(blk+1==blocks)for(unsigned j=0;j<8;j++)bytes[56+j]=uint8_t(bitlen>>(56-8*j));
  uint32_t w[64];
  for(unsigned j=0;j<16;j++){
   const uint8_t*p=bytes+j*4;
   w[j]=(uint32_t(p[0])<<24)|(uint32_t(p[1])<<16)|(uint32_t(p[2])<<8)|p[3];
  }
  for(unsigned j=16;j<64;j++){
   uint32_t a=w[j-15],b=w[j-2];
   uint32_t s0=ror(a,7)^ror(a,18)^(a>>3),s1=ror(b,17)^ror(b,19)^(b>>10);
   w[j]=w[j-16]+s0+w[j-7]+s1;
  }
  uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],z=h[7];
  for(unsigned j=0;j<64;j++){
   uint32_t S1=ror(e,6)^ror(e,11)^ror(e,25),ch=(e&f)^(~e&g);
   uint32_t t1=z+S1+ch+k[j]+w[j];
   uint32_t S0=ror(a,2)^ror(a,13)^ror(a,22),maj=(a&b)^(a&c)^(b&c);
   uint32_t t2=S0+maj;
   z=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
  }
  h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=z;
 }
 std::array<uint8_t,32> out{};
 for(unsigned i=0;i<8;i++)for(unsigned j=0;j<4;j++)out[4*i+j]=uint8_t(h[i]>>(24-8*j));
 return out;
}

// Strict LZ4 *raw block* decoder; rejects truncated and out-of-bounds matches.
std::vector<uint8_t> lz4(const uint8_t*src,std::size_t size,std::size_t expected){
 require(expected<=kMaxSegment,"NSO: segmento descomprimido ultrapassa limite");
 std::vector<uint8_t> out(expected);
 std::size_t in=0,pos=0;
 auto length=[&](std::size_t first)->std::size_t{
   std::size_t n=first;
   if(n==15){uint8_t v;do{require(in<size,"LZ4: comprimento truncado");v=src[in++];require(n<=expected,"LZ4: comprimento excessivo");n+=v;}while(v==255);}
   return n;
 };
 while(in<size){
   uint8_t tok=src[in++];
   std::size_t literals=length(tok>>4);
   require(literals<=size-in && literals<=expected-pos,"LZ4: literais fora de limite");
   std::memcpy(out.data()+pos,src+in,literals);in+=literals;pos+=literals;
   if(in==size)break;
   require(size-in>=2,"LZ4: offset truncado");
   std::size_t offset=std::size_t(src[in])|(std::size_t(src[in+1])<<8);in+=2;
   require(offset>0 && offset<=pos,"LZ4: offset invalido");
   std::size_t match=length(tok&15);
   require(match<=expected-pos && match+4<=expected-pos,"LZ4: match excede segmento");
   match+=4;
   for(std::size_t n=0;n<match;n++)out[pos+n]=out[pos+n-offset];
   pos+=match;
 }
 require(pos==expected,"LZ4: tamanho descomprimido incorreto");
 return out;
}
struct Section {
 const char*name;
 uint32_t file,memory,size,stored;
 bool compressed,hash;
 std::vector<uint8_t> bytes;
};
struct Nso {
 std::array<Section,3> sections{};
 std::size_t imageSize=0;
 uint32_t flags=0,bss=0;
 std::string moduleId;
 bool mod0=false;
 std::size_t mod0Offset=0;
};
std::string hex(const uint8_t*p,std::size_t count){
 static constexpr char digits[]="0123456789abcdef";std::string ret;
 for(std::size_t i=0;i<count;i++){ret+=digits[p[i]>>4];ret+=digits[p[i]&15];}return ret;
}
Nso parseNso(const std::vector<uint8_t>&file){
 require(file.size()>=kNsoHeader,"NSO: cabecalho truncado (minimo 0x100)");
 require(std::memcmp(file.data(),"NSO0",4)==0,"NSO: assinatura NSO0 ausente (NCA/XCI/NSP nao suportados)");
 Nso out;const uint8_t*p=file.data();out.flags=u32(p+0x0c);
 require((out.flags & 0x80u)==0,"NSO: compressao ZBIC nao suportada nesta fase");
 out.moduleId=hex(p+0x40,16);
 out.bss=u32(p+0x3c);
 const char*names[]={".text",".rodata",".data"};
 uint64_t maxEnd=0;
 struct Interval {uint64_t a,b;};std::vector<Interval> ranges;
 for(unsigned i=0;i<3;i++){
  const unsigned base=0x10+i*0x10;
  Section sec{};sec.name=names[i];sec.file=u32(p+base);sec.memory=u32(p+base+4);sec.size=u32(p+base+8);
  sec.compressed=(out.flags&(1u<<i))!=0;sec.hash=(out.flags&(1u<<(i+3)))!=0;
  sec.stored=sec.compressed?u32(p+0x60+i*4):sec.size;
  require(sec.size<=kMaxSegment,"NSO: segmento excede limite de 160 MiB");
  require(uint64_t(sec.file)+sec.stored<=file.size(),"NSO: segmento ultrapassa arquivo");
  require(sec.size==0 || sec.file>=kNsoHeader,"NSO: segmento sobrepoe cabecalho");
  const uint64_t end=uint64_t(sec.memory)+sec.size;
  require(end<=kMaxImage,"NSO: enderecamento excede o limite de 192 MiB");
  if(sec.size){ranges.push_back({sec.memory,end});maxEnd=std::max(maxEnd,end);}
  if(sec.size==0){require(sec.stored==0,"NSO: segmento vazio com dados armazenados");}
  else if(sec.compressed)sec.bytes=lz4(p+sec.file,sec.stored,sec.size);
  else sec.bytes.assign(p+sec.file,p+sec.file+sec.size);
  if(sec.hash){
    auto h=sha256(sec.bytes.data(),sec.bytes.size());
    require(std::memcmp(h.data(),p+0xa0+i*32,32)==0,"NSO: SHA-256 incorreto em "+std::string(sec.name));
  }
  out.sections[i]=std::move(sec);
 }
 // The .bss immediately follows .data, not an independently placed section.
 uint64_t bssStart=uint64_t(out.sections[2].memory)+out.sections[2].size;
 require(bssStart+out.bss<=kMaxImage,"NSO: BSS ultrapassa limite de imagem");
 if(out.bss)ranges.push_back({bssStart,bssStart+out.bss});
 maxEnd=std::max(maxEnd,bssStart+out.bss);
 std::sort(ranges.begin(),ranges.end(),[](Interval x,Interval y){return x.a<y.a;});
 for(std::size_t i=1;i<ranges.size();i++)require(ranges[i-1].b<=ranges[i].a,"NSO: segmentos de memoria sobrepostos");
 out.imageSize=std::size_t(align4096(maxEnd));
 // libnx-built NSOs may contain a MOD0 module header; not all official
 // binaries share this exact startup layout. We only detect, never assume.
 const auto&txt=out.sections[0].bytes;
 if(txt.size()>=8){
  std::size_t at=u32(txt.data()+4);
  if(at<=txt.size() && txt.size()-at>=4 && std::memcmp(txt.data()+at,"MOD0",4)==0){out.mod0=true;out.mod0Offset=at;}
 }
 return out;
}
std::vector<uint8_t> readFile(const std::string&name){
 std::ifstream f(name,std::ios::binary|std::ios::ate);require(bool(f),"Nao foi possivel abrir arquivo: "+name);
 std::streamoff n=f.tellg();require(n>=0 && uint64_t(n)<=kMaxInput,"NSO excede 192 MiB de entrada (fase 1)");
 f.seekg(0);std::vector<uint8_t> b(static_cast<std::size_t>(n));
 if(n)f.read(reinterpret_cast<char*>(b.data()),n);
 require(bool(f),"Leitura incompleta de arquivo NSO");return b;
}
void printBudget(){
 std::cout<<"PHASE=1 SWITCH_RE4_TARGET=1 ARCH_EXECUTION=synthetic_only\n"
          <<"PHYSICAL_TARGET_MIB=1024 HOST_OS_RESERVE_MIB=256 EMULATOR_BUDGET_MIB=768\n"
          <<"GUEST_HEAP_TARGET_MIB=352 TEXTURE_BUDGET_MIB=160\n"
          <<"MODULE_LIMIT_MIB=192 RENDER_BUDGET_MIB=64 OTHER_MIB=0\n"
          <<"NOTE=These_are_planned_caps_not_proof_the_commercial_game_fits\n";
}
std::vector<uint8_t> fixture(){
 // Minimal synthetic NSO with one uncompressed .rodata and LZ4-compressed
 // AArch64 .text.  The code is only `mov w0,#42; ret` and never calls HOS.
 std::vector<uint8_t> f(0x200,0);std::memcpy(f.data(),"NSO0",4);
 put32(f,0x0c,0x3f); // 3 compressed segments, 3 verified hashes
 const std::vector<uint8_t> code={0x40,0x05,0x80,0x52,0xc0,0x03,0x5f,0xd6};
 const std::vector<uint8_t> ro={'R','3','6','S'};
 const std::vector<uint8_t> dat={'T','E','S','T'};
 std::vector<std::vector<uint8_t>> parts={code,ro,dat};
 unsigned fileOff=0x100;
 for(unsigned i=0;i<3;i++){
  const unsigned at=0x10+i*16;
  // For the first segment, reserve 8 bytes for a branch/ret and sample data.
  auto&part=parts[i];
  put32(f,at,fileOff);put32(f,at+4,i*0x1000);put32(f,at+8,uint32_t(part.size()));
  // Raw LZ4 block with literal-only sequence: token (length<<4), bytes.
  require(part.size()<15,"fixture text must be small");
  f[fileOff]=uint8_t(part.size()<<4);
  for(std::size_t j=0;j<part.size();j++)f[fileOff+1+j]=part[j];
  put32(f,0x60+i*4,uint32_t(part.size()+1));
  auto h=sha256(part.data(),part.size());std::copy(h.begin(),h.end(),f.begin()+0xa0+i*32);
  fileOff+=uint32_t(part.size()+1);
 }
 put32(f,0x3c,0x100); // BSS
 return f;
}
void printNso(const Nso&n){
 std::cout<<"NSO_RESULT=PASS BUILD_ID="<<n.moduleId<<" FLAGS=0x"<<std::hex<<n.flags<<std::dec<<"\n";
 for(const auto&s:n.sections)
  std::cout<<"NSO_SECTION="<<s.name<<" MEM_OFFSET=0x"<<std::hex<<s.memory<<std::dec
           <<" UNCOMPRESSED="<<s.size<<" STORED="<<s.stored
           <<" LZ4="<<(s.compressed?1:0)<<" SHA256="<<(s.hash?"PASS":"NOT_REQUESTED")<<"\n";
 std::cout<<"NSO_BSS="<<n.bss<<" NSO_IMAGE_MIB="<<std::fixed<<std::setprecision(2)
          <<double(n.imageSize)/MiB<<" MOD0="<<(n.mod0?"PRESENT":"NOT_DETECTED")<<"\n";
}

#if defined(__aarch64__) && defined(__linux__)
void testNativeAarch64(const Nso&n){
 // Execute ONLY the exact known 8 bytes in our synthetic fixture.
 const uint8_t allowed[8]={0x40,0x05,0x80,0x52,0xc0,0x03,0x5f,0xd6};
 require(n.sections[0].bytes.size()==sizeof allowed &&
         std::memcmp(n.sections[0].bytes.data(),allowed,sizeof allowed)==0,
         "TEST EXECUTION DENIED: non-synthetic module");
 void*mem=mmap(nullptr,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
 require(mem!=MAP_FAILED,"mmap synthetic failed");
 std::memcpy(mem,allowed,8);
 __builtin___clear_cache(reinterpret_cast<char*>(mem),reinterpret_cast<char*>(mem)+8);
 require(mprotect(mem,4096,PROT_READ|PROT_EXEC)==0,"mprotect RX failed");
 using Entry=int(*)();auto entry=reinterpret_cast<Entry>(mem);int result=entry();
 munmap(mem,4096);
 require(result==42,"AArch64 native synthetic returned wrong result");
 std::cout<<"AARCH64_NATIVE_SYNTHETIC=PASS RETURN=42 W_X=PASS\n";
}
#else
void testNativeAarch64(const Nso&){std::cout<<"AARCH64_NATIVE_SYNTHETIC=SKIP HOST_NOT_AARCH64\n";}
#endif

struct Library {
 void*h=nullptr;
 explicit Library(std::initializer_list<const char*> names){for(auto name:names)if((h=dlopen(name,RTLD_NOW|RTLD_LOCAL)))break;}
 ~Library(){if(h)dlclose(h);}
 template<typename T>T get(const char*name)const {return reinterpret_cast<T>(h?dlsym(h,name):nullptr);}
};
int graphicsProbe(int secs,bool fullscreen){
 // Only query via runtime symbols: no SDL2/GLES build headers needed.
 Library sdl({"libSDL2-2.0.so.0","libSDL2.so"});
 require(sdl.h,"GLES3_PROBE=FAIL SDL2_RUNTIME_MISSING");
 auto init=sdl.get<int(*)(uint32_t)>("SDL_Init");
 auto quit=sdl.get<void(*)()>("SDL_Quit");
 auto attr=sdl.get<int(*)(int,int)>("SDL_GL_SetAttribute");
 auto win=sdl.get<void*(*)(const char*,int,int,int,int,uint32_t)>("SDL_CreateWindow");
 auto destroy=sdl.get<void(*)(void*)>("SDL_DestroyWindow");
 auto create=sdl.get<void*(*)(void*)>("SDL_GL_CreateContext");
 auto del=sdl.get<void(*)(void*)>("SDL_GL_DeleteContext");
 auto proc=sdl.get<void*(*)(const char*)>("SDL_GL_GetProcAddress");
 auto swap=sdl.get<void(*)(void*)>("SDL_GL_SwapWindow");
 auto swapi=sdl.get<int(*)(int)>("SDL_GL_SetSwapInterval");
 auto error=sdl.get<const char*(*)()>("SDL_GetError");
 require(init&&quit&&attr&&win&&destroy&&create&&del&&proc&&swap,
         "GLES3_PROBE=FAIL SDL2_SYMBOL_MISSING");
 require(init(0x20u)==0,std::string("GLES3_PROBE=FAIL SDL_INIT ")+(error?error():""));
 // SDL_GL_CONTEXT_PROFILE_ES = 4; SDL_GL_* attributes: major=17, minor=18, profile=21.
 attr(21,4);attr(17,3);attr(18,0);attr(5,1);
 const uint32_t flags=0x2u|0x4u|(fullscreen?0x1001u:0u);
 void*w=win("RE4NX R36S GLES3 Phase1",0x2fff0000,0x2fff0000,640,480,flags);
 if(!w){std::string e=error?error():"unknown";quit();throw Error("GLES3_PROBE=FAIL SDL_WINDOW "+e);}
 void*ctx=create(w);
 if(!ctx){std::string e=error?error():"unknown";destroy(w);quit();throw Error("GLES3_PROBE=FAIL SDL_CONTEXT "+e);}
 auto glString=reinterpret_cast<const unsigned char*(*)(uint32_t)>(proc("glGetString"));
 auto glClearColor=reinterpret_cast<void(*)(float,float,float,float)>(proc("glClearColor"));
 auto glClear=reinterpret_cast<void(*)(uint32_t)>(proc("glClear"));
 auto glError=reinterpret_cast<uint32_t(*)()>(proc("glGetError"));
 require(glString&&glClearColor&&glClear&&glError,"GLES3_PROBE=FAIL MISSING_GL_SYMBOL");
 const char*version=reinterpret_cast<const char*>(glString(0x1F02));
 const char*renderer=reinterpret_cast<const char*>(glString(0x1F01));
 require(version,"GLES3_PROBE=FAIL NO_GL_VERSION");
 int major=0,minor=0;require(std::sscanf(version,"OpenGL ES %d.%d",&major,&minor)==2&&major>=3,
                              "GLES3_PROBE=FAIL VERSION_BELOW_ES3");
 std::cout<<"GLES3_PROBE=PASS VERSION="<<version<<" RENDERER="<<(renderer?renderer:"unknown")<<"\n";
 if(swapi)swapi(1);
 auto start=std::chrono::steady_clock::now();int frames=0;uint32_t err=0;
 // Only a color clear; DOES NOT emulate Switch GPU and does not benchmark RE4.
 while(std::chrono::steady_clock::now()-start<std::chrono::seconds(secs)){
  float t=float(frames%160)/160.f;
  glClearColor(0.05f,0.08f+t*0.12f,0.15f,1.f);glClear(0x00004000);swap(w);frames++;
  if((err=glError())!=0)break;
 }
 auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
 del(ctx);destroy(w);quit();
 std::cout<<"GLES3_CLEAR_FRAMES="<<frames<<" SECONDS="<<std::fixed<<std::setprecision(2)<<elapsed
          <<" CLEAR_FPS="<<(elapsed>0?frames/elapsed:0)<<" GL_ERROR="<<err<<"\n";
 require(err==0,"GLES3_PROBE=FAIL GL_ERROR");return 0;
}
void selftest(){
 const auto f=fixture();Nso n=parseNso(f);printNso(n);
 require(n.sections[0].bytes.size()==8,"test: LZ4 decode size");
 require(n.sections[1].bytes.size()==4,"test: rodata size");
 require(hex(sha256(reinterpret_cast<const uint8_t*>("abc"),3).data(),32)==
         "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
         "test: SHA256 abc");
 bool denied=false;
 try {auto bad=f;bad[0xa0]^=1;parseNso(bad);} catch(const Error&){denied=true;}
 require(denied,"test: bad SHA was not rejected");
 denied=false;
 try {auto bad=f;put32(bad,0x14,0xfffffff0u);parseNso(bad);}catch(const Error&){denied=true;}
 require(denied,"test: huge memory offsets not rejected");
 denied=false;
 try {auto bad=f;put32(bad,0x60,1);parseNso(bad);}catch(const Error&){denied=true;}
 require(denied,"test: broken LZ4 was not rejected");
 testNativeAarch64(n);
 std::cout<<"SELFTEST=PASS NSO_LZ4=PASS SHA256=PASS RANGE_CHECKS=PASS\n";
}
void usage(){
 std::cout<<"RE4NX-R36S / Switch RE4 specialized compatibility research (PHASE 1)\n"
          <<"Usage:\n  re4nx --selftest\n  re4nx --budget\n"
          <<"  re4nx --inspect /path/to/decrypted/main.nso [--expect-module-id 32_hex_chars]\n"
          <<"  re4nx --probe-gles3 [--fullscreen] [--seconds 3]\n"
          <<"No ROM decoding, firmware, decryption, commercial gameplay, HOS or Switch NVN emulation yet.\n";
}
} // anonymous namespace
int main(int argc,char**argv){
 try{
  if(argc<=1){usage();return 0;}
  std::string command=argv[1];
  if(command=="--selftest"){selftest();return 0;}
  if(command=="--budget"){printBudget();return 0;}
  if(command=="--inspect"){
   require(argc>=3,"--inspect precisa do caminho para um NSO descriptografado");
   auto nso=parseNso(readFile(argv[2]));
   // Optional version-specific allowlist: no guessed RE4 build ID is embedded.
   // The owner can inspect and explicitly pin one legally acquired build.
   if(argc>3){
    require(argc==5 && std::strcmp(argv[3],"--expect-module-id")==0,
            "Use: --inspect main.nso [--expect-module-id 32_hex_chars]");
    std::string expected=argv[4];
    require(expected.size()==32 && expected.find_first_not_of("0123456789abcdefABCDEF")==std::string::npos,
            "Module ID must contain exactly 32 hex digits");
    std::transform(expected.begin(),expected.end(),expected.begin(),[](unsigned char c){return char(std::tolower(c));});
    require(nso.moduleId==expected,"TITLE_LOCK=REJECTED MODULE_ID_MISMATCH");
    std::cout<<"TITLE_LOCK=PASS (user-specified build ID)\n";
   }else std::cout<<"TITLE_LOCK=NOT_CONFIGURED (inspection only)\n";
   printNso(nso);
   std::cout<<"GAME_EXECUTION=BLOCKED HORIZON_HLE=NOT_IMPLEMENTED NVN_TO_GLES3=NOT_IMPLEMENTED\n";
   return 0;
  }
  if(command=="--probe-gles3"){
   int seconds=3;bool fullscreen=false;
   for(int i=2;i<argc;i++){
    if(std::strcmp(argv[i],"--fullscreen")==0)fullscreen=true;
    else if(std::strcmp(argv[i],"--seconds")==0){require(i+1<argc,"--seconds sem valor");seconds=std::atoi(argv[++i]);}
    else throw Error(std::string("Opcao de grafico desconhecida: ")+argv[i]);
   }
   require(seconds>=1 && seconds<=20,"--seconds deve estar entre 1 e 20");
   return graphicsProbe(seconds,fullscreen);
  }
  usage();return 2;
 }catch(const std::exception&e){std::cerr<<"RESULT=FAIL REASON="<<e.what()<<"\n";return 1;}
}
