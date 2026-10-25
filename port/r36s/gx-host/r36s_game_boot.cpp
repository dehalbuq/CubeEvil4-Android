// EXPERIMENT: original CubeEvil4 GX renderer receiving live GX chunks from
// the project's existing re4_coreproc shared-memory IPC (Linux AArch64).
// Real-game-data optional test. A separate 120-frame synthetic IPC test is preserved.
// The core process is separate: its 32-bit game address constraints cannot
// collide with SDL2/EGL/GL allocations in this process.
#include "gx_gl.h"
#include "re4_core_client.h"
#include "re4_gx_frame.h"
#include "r36s_boot_crash_trace.h"
#ifdef RE4_R36S_SHOWCASE
#include "r36s_showcase_clean_data.h"
#include "r36s_showcase_exit_controls.h"
#endif
#include <SDL2/SDL.h>
#include <GLES3/gl3.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <cstdint>
#ifdef RE4_R36S_SHOWCASE
#include <array>
#include <atomic>
#include <thread>
#endif

#ifdef RE4_R36S_SHOWCASE
// Hard-disable the heavyweight debug telemetry for the public-facing
// front-end. Crash and error messages still use fprintf(stderr,...).
#define printf(...) ((int)0)
#define puts(...) ((int)0)
#define TRACE_MARK(...) ((void)0)
#else
#define TRACE_MARK(...) r36s_crash::mark(__VA_ARGS__)
#endif

namespace {
using Clock=std::chrono::steady_clock;
constexpr int kW=640,kH=480;

int64_t nowNs() {
  const auto ns=std::chrono::duration_cast<std::chrono::nanoseconds>(
                    Clock::now().time_since_epoch()).count();
  return (int64_t)ns;
}
void coreMemory(int pid){
  if(pid<=0)return;
  char path[64]{};
  snprintf(path,sizeof(path),"/proc/%d/status",pid);
  FILE* f=fopen(path,"r");
  if(!f){printf("CORE_MEM_STATUS=not_available_pid_%d\n",pid);return;}
  char line[256];
  while(fgets(line,sizeof(line),f)){
    if(strncmp(line,"VmRSS:",6)==0||strncmp(line,"VmHWM:",6)==0||
       strncmp(line,"VmSize:",7)==0){
      printf("CORE_PID_%d_%s",pid,line);
    }
  }
  fclose(f);
}
#ifdef RE4_R36S_SCALABLE_EFB
void systemMemory(const char* stage) {
  FILE* f=fopen("/proc/meminfo","r");
  if(!f){puts("SYSTEM_MEMINFO=UNAVAILABLE");return;}
  unsigned long available=0,freeBytes=0,swap=0;
  char line[256];
  while(fgets(line,sizeof(line),f)){
    if(sscanf(line,"MemAvailable: %lu kB",&available)==1)continue;
    if(sscanf(line,"MemFree: %lu kB",&freeBytes)==1)continue;
    if(sscanf(line,"SwapFree: %lu kB",&swap)==1)continue;
  }
  fclose(f);
  printf("SYSTEM_MEMORY_SAMPLE STAGE=%s MEM_AVAILABLE_KB=%lu MEM_FREE_KB=%lu SWAP_FREE_KB=%lu\n",
       stage,available,freeBytes,swap);
  if(available && available<131072u)
    printf("SYSTEM_MEMORY_WARNING=LOW_AVAILABLE_RAM_KB_%lu\n",available);
}
#endif
void memory(const char* stage) {
  FILE* f=fopen("/proc/self/status","r");
  if(!f)return;
  char b[256];
  while(fgets(b,sizeof b,f)) {
    if(strncmp(b,"VmRSS:",6)==0 || strncmp(b,"VmHWM:",6)==0)
       printf("%s_%s",stage,b);
  }
  fclose(f);
}
bool exists(const std::string& p,bool dir){
  struct stat st{};
  return stat(p.c_str(),&st)==0 && (dir?S_ISDIR(st.st_mode):S_ISREG(st.st_mode));
}
bool checkGameData(const std::string& root){
  std::string disc;
  for(const char* item: {"disc0","disc1",""}){
    const std::string candidate=item[0]?root+"/"+item:root;
    if(exists(candidate+"/sys/boot.bin",false) &&
       exists(candidate+"/sys/fst.bin",false) && exists(candidate+"/files",true)){
      disc=candidate;break;
    }
  }
  if(disc.empty()){
    fprintf(stderr,"GAME_DATA_INVALID_LAYOUT: expect disc0/sys/{boot.bin,fst.bin} and disc0/files (or disc1 / root)\n");
    return false;
  }
  FILE* f=fopen((disc+"/sys/boot.bin").c_str(),"rb");
  char id[7]{};
  size_t got=f?fread(id,1,6,f):0;
  if(f)fclose(f);
  printf("GAME_DISC=%s GAME_ID=%.*s\n",disc.c_str(),6,id);
  if(got!=6 || memcmp(id,"G4BE08",6)!=0){
    fprintf(stderr,"GAME_DATA_INVALID_ID: expected GameCube G4BE08 (specific supported revision)\n");
    return false;
  }
  puts("GAME_DATA_PREFLIGHT=OK");
  return true;
}
static int8_t axis(Sint16 v,int limit){
  int n=int(v)*limit/32767;
  return (int8_t)std::max(-limit,std::min(limit,n));
}
void updatePad(SDL_GameController* joy, bool menu=false){
  Re4CorePad pad{};
  if(menu){re4ipc_pad(&pad);return;}
  if(joy){
    SDL_GameControllerUpdate();
    auto key=[joy](SDL_GameControllerButton b){return SDL_GameControllerGetButton(joy,b)!=0;};
    if(key(SDL_CONTROLLER_BUTTON_A))pad.buttons|=0x0100;
    if(key(SDL_CONTROLLER_BUTTON_B))pad.buttons|=0x0200;
    if(key(SDL_CONTROLLER_BUTTON_X))pad.buttons|=0x0400;
    if(key(SDL_CONTROLLER_BUTTON_Y))pad.buttons|=0x0800;
    if(key(SDL_CONTROLLER_BUTTON_START))pad.buttons|=0x1000;
    if(key(SDL_CONTROLLER_BUTTON_DPAD_LEFT))pad.buttons|=0x0001;
    if(key(SDL_CONTROLLER_BUTTON_DPAD_RIGHT))pad.buttons|=0x0002;
    if(key(SDL_CONTROLLER_BUTTON_DPAD_DOWN))pad.buttons|=0x0004;
    if(key(SDL_CONTROLLER_BUTTON_DPAD_UP))pad.buttons|=0x0008;
    if(key(SDL_CONTROLLER_BUTTON_LEFTSHOULDER))pad.buttons|=0x0010;
    pad.stickX=axis(SDL_GameControllerGetAxis(joy,SDL_CONTROLLER_AXIS_LEFTX),100);
    pad.stickY=axis(-SDL_GameControllerGetAxis(joy,SDL_CONTROLLER_AXIS_LEFTY),100);
    pad.substickX=axis(SDL_GameControllerGetAxis(joy,SDL_CONTROLLER_AXIS_RIGHTX),90);
    pad.substickY=axis(-SDL_GameControllerGetAxis(joy,SDL_CONTROLLER_AXIS_RIGHTY),90);
    int tl=SDL_GameControllerGetAxis(joy,SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    int tr=SDL_GameControllerGetAxis(joy,SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
    pad.triggerL=(uint8_t)std::max(0,tl*255/32767);
    pad.triggerR=(uint8_t)std::max(0,tr*255/32767);
    if(pad.triggerL>120)pad.buttons|=0x0040;
    if(pad.triggerR>120 || key(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))pad.buttons|=0x0020;
  }else{
    const Uint8* k=SDL_GetKeyboardState(nullptr);
    if(k[SDL_SCANCODE_RETURN]||k[SDL_SCANCODE_Z])pad.buttons|=0x0100;
    if(k[SDL_SCANCODE_X])pad.buttons|=0x0200;
    if(k[SDL_SCANCODE_SPACE])pad.buttons|=0x1000;
    if(k[SDL_SCANCODE_LEFT])pad.buttons|=0x0001;
    if(k[SDL_SCANCODE_RIGHT])pad.buttons|=0x0002;
    if(k[SDL_SCANCODE_DOWN])pad.buttons|=0x0004;
    if(k[SDL_SCANCODE_UP])pad.buttons|=0x0008;
  }
  re4ipc_pad(&pad);
}
void reportStats(const re4gx::GxRenderer& r){
  const auto& s=r.Stats();
  printf("GX_CHUNKS=%u DRAWS=%u TRIANGLES=%u SHADERS=%u SHADER_ERRORS=%u\n",
          s.chunks,s.draws,s.triangles,s.programs,s.shaderErrors);
  printf("GX_TEXTURES=%u COPIES=%u PRESENTS=%u RESETS=%u\n",
          s.textures,s.displayCopies,s.presents,s.resets);
#ifdef RE4_R36S_UPLOAD_PROFILE
  printf("GEOMETRY_UPLOAD_TOTAL CHUNKS=%u BYTES=%llu GL_CALL_TIME_US=%llu ALLOC_CALLS=%u SUBDATA_CALLS=%u OVERSIZE_FALLBACKS=%u\n",
         s.geometryUploadChunks,s.geometryUploadBytes,s.geometryUploadMicros,
         s.geometryBufferAllocations,s.geometryBufferSubDataCalls,s.geometryOversizeFallbacks);
#endif
#ifdef RE4_R36S_LOW_SPEC
  printf("LOW_SPEC_CULLED_DRAWS=%u CULLED_TRIANGLES=%u DOF_PASSES_SKIPPED=%u\n",
    s.culledFarDraws,s.culledFarTriangles,s.skippedAutofocus);
#endif
}

#ifdef RE4_R36S_SHOWCASE
// Full-screen fit on the R36S 4:3 display: use the ENTIRE source width,
// crop ONLY the black letterbox rows, and stretch the 16:9 active picture
// vertically to fill 640x480. Deliberately distorts proportions rather than
// losing gameplay objects or HUD at the left and right edges.
void showcaseAspect(re4gx::GxRenderer& renderer) {
  renderer.SetShowcaseDisplayCrop(0.0f,0.125f,1.0f,0.75f);
}

// Sound is PCM 32000 Hz, S16 stereo in the core IPC ring (core_ipc.cpp).
// Never read it on the graphics thread: GX rendering can take 150+ ms/frame.
// SDL_QueueAudio is fed independently to keep latency below ~125ms.
struct ShowcaseAudio {
  SDL_AudioDeviceID dev=0;
  std::atomic<bool> done{false};
  std::thread worker;
  void start(){
    if(SDL_InitSubSystem(SDL_INIT_AUDIO)!=0)
      fprintf(stderr,"SHOWCASE_AUDIO_INIT_FAILED=%s\n",SDL_GetError());
    else {
      SDL_AudioSpec wanted{},got{};
      wanted.freq=32000; wanted.format=AUDIO_S16SYS;
      wanted.channels=2; wanted.samples=512; wanted.callback=nullptr;
      dev=SDL_OpenAudioDevice(nullptr,0,&wanted,&got,0);
      if(!dev)fprintf(stderr,"SHOWCASE_AUDIO_DEVICE_FAILED=%s\n",SDL_GetError());
      else SDL_PauseAudioDevice(dev,0);
    }
    worker=std::thread([this] {
      std::array<int16_t,512> pcm{};  // 256 stereo frames = 8 ms
      constexpr Uint32 kQueueCap=32000u*2u*2u/8u; // 125 ms
      while(!done.load(std::memory_order_relaxed)){
        if(dev && SDL_GetQueuedAudioSize(dev)>kQueueCap){
          SDL_Delay(4);continue;
        }
        re4ipc_audio(pcm.data(),256);  // fills unavailable frames with silence
        if(dev && SDL_QueueAudio(dev,pcm.data(),static_cast<Uint32>(pcm.size()*sizeof(pcm[0])))!=0)
          SDL_Delay(10);
        if(!dev)SDL_Delay(8);  // even without ALSA, drain ring to avoid stopping game
      }
    });
  }
  void stopAndClose(){
    done.store(true,std::memory_order_relaxed);
    if(worker.joinable())worker.join();
    if(dev){SDL_ClearQueuedAudio(dev);SDL_CloseAudioDevice(dev);dev=0;}
  }
  ~ShowcaseAudio(){stopAndClose();}
};

// Embedded bitmap font for the exit menu. No external fonts or SDL_ttf
// dependencies, no proprietary artwork, and no code executes in normal play.
using ShowcaseGlyph=std::array<unsigned char,7>;
const ShowcaseGlyph& glyph(char ch){
  static const ShowcaseGlyph blank{};
  switch(ch){
    case 'A': {static const ShowcaseGlyph g{14,17,17,31,17,17,17};return g;}
    case 'B': {static const ShowcaseGlyph g{30,17,17,30,17,17,30};return g;}
    case 'C': {static const ShowcaseGlyph g{14,17,16,16,16,17,14};return g;}
    case 'D': {static const ShowcaseGlyph g{30,17,17,17,17,17,30};return g;}
    case 'E': {static const ShowcaseGlyph g{31,16,16,30,16,16,31};return g;}
    case 'F': {static const ShowcaseGlyph g{31,16,16,30,16,16,16};return g;}
    case 'G': {static const ShowcaseGlyph g{14,17,16,23,17,17,15};return g;}
    case 'I': {static const ShowcaseGlyph g{31,4,4,4,4,4,31};return g;}
    case 'J': {static const ShowcaseGlyph g{7,2,2,2,18,18,12};return g;}
    case 'L': {static const ShowcaseGlyph g{16,16,16,16,16,16,31};return g;}
    case 'M': {static const ShowcaseGlyph g{17,27,21,21,17,17,17};return g;}
    case 'N': {static const ShowcaseGlyph g{17,25,21,19,17,17,17};return g;}
    case 'O': {static const ShowcaseGlyph g{14,17,17,17,17,17,14};return g;}
    case 'P': {static const ShowcaseGlyph g{30,17,17,30,16,16,16};return g;}
    case 'R': {static const ShowcaseGlyph g{30,17,17,30,20,18,17};return g;}
    case 'S': {static const ShowcaseGlyph g{15,16,16,14,1,1,30};return g;}
    case 'T': {static const ShowcaseGlyph g{31,4,4,4,4,4,4};return g;}
    case 'U': {static const ShowcaseGlyph g{17,17,17,17,17,17,14};return g;}
    case 'V': {static const ShowcaseGlyph g{17,17,17,17,17,10,4};return g;}
    case 'X': {static const ShowcaseGlyph g{17,17,10,4,10,17,17};return g;}
    case 'Y': {static const ShowcaseGlyph g{17,17,10,4,4,4,4};return g;}
    case '?': {static const ShowcaseGlyph g{14,17,1,2,4,0,4};return g;}
    case ':': {static const ShowcaseGlyph g{0,4,4,0,4,4,0};return g;}
    case '+': {static const ShowcaseGlyph g{0,4,4,31,4,4,0};return g;}
    case '3': {static const ShowcaseGlyph g{30,1,1,14,1,1,30};return g;}
    case '4': {static const ShowcaseGlyph g{2,6,10,18,31,2,2};return g;}
    default:return blank;
  }
}
void showcaseRect(int x,int y,int width,int height,float r,float g,float b){
  if(x<0||y<0||width<=0||height<=0||x+width>kW||y+height>kH)return;
  glScissor(x,kH-y-height,width,height);
  glClearColor(r,g,b,1);
  glClear(GL_COLOR_BUFFER_BIT);
}
void showcaseText(int x,int y,int factor,const char* str,float r,float g,float b){
  for(const char* ch=str;*ch;++ch,x+=factor*6){
    const auto &gph=glyph(*ch);
    for(int row=0;row<7;++row)
      for(int col=0;col<5;++col)
        if(gph[row]&(16>>col))
          showcaseRect(x+col*factor,y+row*factor,factor,factor,r,g,b);
  }
}
void showcaseExitOverlay(){
  glBindFramebuffer(GL_FRAMEBUFFER,0);
  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
  glEnable(GL_SCISSOR_TEST);
  showcaseRect(80,142,480,194,.02f,.025f,.04f);
  showcaseRect(80,142,480,4,.83f,.63f,.18f);
  showcaseRect(80,332,480,4,.83f,.63f,.18f);
  showcaseText(166,162,4,"SAIR DO JOGO?",1.f,.85f,.4f);
  showcaseText(144,210,3,"A: SAIR",.5f,1.f,.5f);
  showcaseText(335,210,3,"B: VOLTAR",1.f,.55f,.55f);
  showcaseText(181,274,2,"CUBEEVIL4 R36S",.75f,.85f,1.f);
  glDisable(GL_SCISSOR_TEST);
}
#endif

} // namespace

int main(int argc,char**argv) {
  setvbuf(stdout,nullptr,_IONBF,0);
  setvbuf(stderr,nullptr,_IONBF,0);
  TRACE_MARK("installing_sigaction",0,0);
#ifndef RE4_R36S_SHOWCASE
  if(!r36s_crash::install()){
    fprintf(stderr,"CRASH_TRACE_INSTALL_WARNING=unavailable\n");
  }else{
    fprintf(stderr,"CRASH_TRACE_INSTALLED=YES (PC,LR,stage,maps)\n");
  }
#endif
  puts("CUBEEVIL4_R36S_FIRST_REAL_GAME_DATA_BOOT");
  if(argc<4){
    fprintf(stderr,"usage: r36s_game_boot COREFOLDER MODULESFOLDER DATADIR [SECONDS]\n");
    return 2;
  }
  const std::string coreDir=argv[1],modules=argv[2];
  const std::string originalDataDir=argv[3];
  if(!checkGameData(originalDataDir))return 20;
#ifdef RE4_R36S_SHOWCASE
  // Own-data, per-process, read-only symlink overlay: the original debug
  // config stays byte-for-byte intact. Only the copied PRINT_PAGE number
  // is set to zero, disabling the game's native debug text renderer.
  ShowcaseCleanData cleanGameData;
  const bool debugOverlayReady=cleanGameData.prepare(coreDir,originalDataDir);
  const std::string dataDir=debugOverlayReady?cleanGameData.path():originalDataDir;
  if(!debugOverlayReady)
    fprintf(stderr,"SHOWCASE_DEBUG_TEXT_NOTE=unable_to_prepare_clean_config_overlay; original debug text may remain\n");
#else
  const std::string dataDir=originalDataDir;
#endif
  int sec=argc>4?atoi(argv[4]):180;
#ifdef RE4_R36S_SHOWCASE
  // A practical demo is not cut off after 180 seconds; maximum 2h safeguards
  // forgotten unattended processes. The exit menu works at any time.
  sec=argc>4?atoi(argv[4]):7200;
  sec=std::max(60,std::min(sec,7200));
#else
  sec=std::max(5,std::min(sec,600));
#endif
  printf("RUN_MODE=GAME_BOOT MAX_SECONDS=%d\n",sec);
#ifdef RE4_R36S_UPLOAD_PROFILE
#ifdef RE4_R36S_BUFFER_REUSE
  puts("GEOMETRY_UPLOAD_MODE=REUSE_SUBDATA VBO_INITIAL=8MiB IBO_INITIAL=1MiB VBO_MAX=16MiB IBO_MAX=4MiB");
#else
  puts("GEOMETRY_UPLOAD_MODE=BASELINE_GLBUFFERDATA_EVERY_CHUNK");
#endif
  puts("GEOMETRY_UPLOAD_NOTE=GL_wall_clock_may_include_driver_waits_not_pure_DMA_GPU_time");
#endif
#ifdef RE4_R36S_SCALABLE_EFB
  printf("SCALABLE_EFB_REQUEST=%s\n",
         getenv("RE4_R36S_RENDER_SCALE")?getenv("RE4_R36S_RENDER_SCALE"):"(unset)");
  systemMemory("SESSION_START");
#endif
#ifdef RE4_R36S_A35_TARGET
  puts("A35_COMPILER_TARGET=cortex-a35 OPT_LEVEL=O3 LTO=enabled FAST_MATH=disabled");
  printf("A35_EFB_REQUEST=%s\n",
    getenv("RE4_R36S_HALF_EFB")?getenv("RE4_R36S_HALF_EFB"):"0");
#endif
#ifdef RE4_R36S_LOW_SPEC
  printf("LOW_SPEC_ENABLED=1\n");
  printf("LOW_SPEC_LIGHTS_MAX=%s\n",
    getenv("RE4_R36S_LIGHTS_MAX")?getenv("RE4_R36S_LIGHTS_MAX"):"8");
  printf("LOW_SPEC_SKIP_DOF=%s SKIP_FOG=%s FAR_Z=%s\n",
    getenv("RE4_R36S_SKIP_DOF")?getenv("RE4_R36S_SKIP_DOF"):"0",
    getenv("RE4_R36S_SKIP_FOG")?getenv("RE4_R36S_SKIP_FOG"):"0",
    getenv("RE4_R36S_FAR_Z")?getenv("RE4_R36S_FAR_Z"):"0");
#endif
  const char* envMode=getenv("RE4_R36S_SWAP_MODE");
  int swapMode=envMode?atoi(envMode):5;
  if(swapMode<0 || swapMode>5){
    fprintf(stderr,"SWAP_MODE_INVALID=%d valid 0..5\n",swapMode);
    return 21;
  }
  // Each mode changes only the GL swap / uniform-buffer strategy.
  // Mode 5: keep VSync interval 1, glFinish before SDL_GL_SwapWindow.
  // This is the closest to the real-mode-1 configuration shown to survive
  // 487 real-game frames on the R36S Mali-G31. Runtime stability unproven.
  if(swapMode==2)setenv("RE4_GX_PERSISTENT","0",1);
  if(swapMode==3)setenv("RE4_GX_UNIFORM_STREAM","0",1);
  printf("GPU_SWAP_DIAG_MODE=%d\n",swapMode);
  printf("GPU_MODE_DESC=%s\n",
    swapMode==0?"baseline_vsync1_persistent":
    swapMode==1?"vsync0_glFinish_before_swap":
    swapMode==2?"vsync1_no_persistent_uniform_mapping":
    swapMode==3?"vsync1_legacy_uniform_buffers":
    swapMode==4?"vsync0_glFinish_no_swap_offscreen":
                "vsync1_glFinish_before_swap_R36S_candidate");
  printf("RE4_GX_PERSISTENT=%s RE4_GX_UNIFORM_STREAM=%s\n",
    getenv("RE4_GX_PERSISTENT")?getenv("RE4_GX_PERSISTENT"):"(default)",
    getenv("RE4_GX_UNIFORM_STREAM")?getenv("RE4_GX_UNIFORM_STREAM"):"(default)");
  // Ensure real game boot; never accidentally run the synthetic probe here.
  unsetenv("RE4_CORE_TEST_ENTRY");
#ifdef RE4_R36S_SHOWCASE
  setenv("RE4_CORE_IPC_CHECK","0",1);
#else
  setenv("RE4_CORE_IPC_CHECK","1",1);
#endif
  re4ipc::Spawn spawn;
  spawn.program=coreDir+"/re4_coreproc";
  // core_ipc_client.cpp deliberately strips inherited LD_LIBRARY_PATH
  // before execve; explicitly supply the folder containing libre4_*.so.
  spawn.libraryDir=coreDir;
  spawn.helloSeconds=30;
  re4ipc::configure(spawn);
  memory("START");
  TRACE_MARK("ipc_reserve",0,0);
  if(re4ipc_reserve(dataDir.c_str(),modules.c_str())!=1){
    fprintf(stderr,"CORE_IPC_RESERVE_FAILED=%s\n",re4ipc_message());
    re4ipc::shutdown(1000);
    return 3;
  }
  Re4CorePad pad{};
  // Keep controls neutral until the first SDL input poll.
  re4ipc_pad(&pad);
  TRACE_MARK("core_start",0,0);
  if(re4ipc_start()!=1){
    fprintf(stderr,"CORE_IPC_START_FAILED=%s\n",re4ipc_message());
    re4ipc::shutdown(1000);
    return 4;
  }
  const int corePid=re4ipc::pid();
  printf("CORE_PID=%d\n",corePid);
  TRACE_MARK("sdl_init",0,0);
#ifdef RE4_R36S_SHOWCASE
  // Keep the R36S physical joystick visible even if KMSDRM window focus is
  // unexpectedly lost, and receive both SDL_JOYBUTTONDOWN and controller events.
  SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
#endif
  if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS|SDL_INIT_GAMECONTROLLER)!=0) {
    fprintf(stderr,"SDL_ERROR=%s\n",SDL_GetError());
    re4ipc::shutdown(3000);return 5;
  }
  printf("SDL_DRIVER=%s\n",SDL_GetCurrentVideoDriver());
  SDL_GameController* joy=nullptr;
  for(int n=0;n<SDL_NumJoysticks();n++){
    if(SDL_IsGameController(n)){joy=SDL_GameControllerOpen(n);if(joy)break;}
  }
  printf("GAME_CONTROLLER=%s\n",joy?SDL_GameControllerName(joy):"(none; keyboard fallback)");
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,0);
  SDL_Window* window=SDL_CreateWindow("CubeEvil4 IPC GX R36S",SDL_WINDOWPOS_CENTERED,
     SDL_WINDOWPOS_CENTERED,kW,kH,SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL);
  if(!window){fprintf(stderr,"SDL_WINDOW_ERROR=%s\n",SDL_GetError());SDL_Quit();re4ipc::shutdown(3000);return 6;}
  TRACE_MARK("egl_create",0,0);
  SDL_GLContext context=SDL_GL_CreateContext(window);
  if(!context){
    fprintf(stderr,"SDL_CONTEXT_ERROR=%s\n",SDL_GetError());
    SDL_DestroyWindow(window);SDL_Quit();re4ipc::shutdown(3000);return 7;
  }
  SDL_GL_MakeCurrent(window,context);
  const int interval=swapMode==1||swapMode==4?0:1;
  int intervalRc=SDL_GL_SetSwapInterval(interval);
  printf("SDL_SWAP_INTERVAL_REQUESTED=%d SET_RC=%d ACTUAL=%d ERR=%s\n",
    interval,intervalRc,SDL_GL_GetSwapInterval(),intervalRc?SDL_GetError():"none");
  printf("GL_RENDERER=%s\nGL_VERSION=%s\n",glGetString(GL_RENDERER),glGetString(GL_VERSION));
  int result=0;unsigned processed=0,resets=0,presented=0,glErrors=0;
#ifdef RE4_R36S_SHOWCASE
  // Keep a raw SDL_Joystick handle alongside SDL_GameController for R36S
  // firmware mappings where SELECT/BACK doesn't generate controller events.
  r36s_showcase_exit::Device exitDevice;
  exitDevice.attach(joy);
  SDL_JoystickEventState(SDL_ENABLE);
  SDL_GameControllerEventState(SDL_ENABLE);
  const bool diagnoseInput=std::getenv("RE4_SHOWCASE_INPUT_DIAG") &&
                           std::strcmp(std::getenv("RE4_SHOWCASE_INPUT_DIAG"),"1")==0;
  if(diagnoseInput) {
    fprintf(stderr,"R36S_INPUT_DEVICES=%d GAMECONTROLLER=%s\n",
            SDL_NumJoysticks(), joy?SDL_GameControllerName(joy):"(no SDL GC mapping)");
    for(int i=0;i<SDL_NumJoysticks();++i)
      fprintf(stderr,"R36S_INPUT_JOYSTICK_%d=%s\n",i,SDL_JoystickNameForIndex(i));
  }
#endif
  try {
    re4gx::GxRenderer renderer;
    re4gx::GxOptions opts;
    opts.samples=1;opts.tiledMsaa=false;
    renderer.SetOptions(opts);
    std::string error;
    TRACE_MARK("renderer_init",0,0);
    if(!renderer.Init(1,&error)){
      fprintf(stderr,"GX_INIT_ERROR=%s\n",error.c_str());
      result=8;
    } else {
#ifdef RE4_R36S_SHADER_CACHE
      const char* shaderDir=getenv("RE4_R36S_SHADER_CACHE_DIR");
      GLint binaryFormats=0;
      glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS,&binaryFormats);
      bool binaryCacheEnabled=(shaderDir && *shaderDir && renderer.SetProgramCache(shaderDir));
      printf("SHADER_CACHE_BINARY_FORMATS=%d\n",binaryFormats);
      printf("SHADER_CACHE_ENABLED=%d\n",binaryCacheEnabled?1:0);
      printf("SHADER_CACHE_DIR=%s\n",shaderDir && *shaderDir?shaderDir:"(not configured)");
      puts("SHADER_CACHE_NOTE=first_run_compiles_new_shaders_next_run_can_load_binary_cache");
#endif
#ifdef RE4_R36S_SHOWCASE
      showcaseAspect(renderer);
      ShowcaseAudio showcaseAudio;
      showcaseAudio.start();
      bool showcaseMenu=false;
      r36s_showcase_exit::Gestures exitGestures;
#endif
      const auto deadline=Clock::now()+std::chrono::seconds(sec);
      uint32_t last=0;
      auto nextTick=Clock::now();
      auto telemetryAt=Clock::now()+std::chrono::seconds(5);
      auto telemetryStart=Clock::now();
      auto quitHeldAt=Clock::time_point{};
      unsigned lastTelemetryFrames=0;
#ifdef RE4_R36S_UPLOAD_PROFILE
      unsigned long long lastGeometryUploadMicros=0;
      unsigned long long lastGeometryUploadBytes=0;
      unsigned lastGeometryChunks=0;
      unsigned lastGeometryAlloc=0;
      unsigned lastGeometrySubData=0;
#endif
#ifdef RE4_R36S_LOW_SPEC
      unsigned lastCulledFarDraws=0,lastDofSkipped=0;
#endif
      unsigned polls=0;
      bool userQuit=false;
      printf("SAFE_EXIT=ESC_OR_SDL_QUIT_OR_HOLD_SELECT_PLUS_START_2_SECONDS\n");
      while(Clock::now()<deadline && processed<36000 && !result && !userQuit){
        SDL_Event ev{};
#ifdef RE4_R36S_SHOWCASE
        bool toggleMenu=false,confirmExit=false,cancelExit=false,directExit=false;
        while(SDL_PollEvent(&ev)){
          if(ev.type==SDL_QUIT){directExit=true;break;}
          if(ev.type==SDL_KEYDOWN && ev.key.repeat==0){
            if(ev.key.keysym.sym==SDLK_ESCAPE||ev.key.keysym.sym==SDLK_F1)
              toggleMenu=true;
            else if(showcaseMenu &&
                    (ev.key.keysym.sym==SDLK_RETURN||ev.key.keysym.sym==SDLK_z))
              confirmExit=true;
            else if(showcaseMenu &&
                    (ev.key.keysym.sym==SDLK_BACKSPACE||ev.key.keysym.sym==SDLK_x))
              cancelExit=true;
          }
          // SDL_CONTROLLERBUTTONDOWN and SDL_JOYBUTTONDOWN are frequently
          // reported for the SAME R36S button, and the old raw HOME=0 guess
          // made ordinary A presses open the overlay. Never use individual
          // button-down events to open/close the menu. Poll long holds only.
          if(diagnoseInput && ev.type==SDL_JOYBUTTONDOWN)
            fprintf(stderr,"R36S_RAW_JOYBUTTON_DOWN DEVICE=%d BUTTON=%u\\n",
                    static_cast<int>(ev.jbutton.which),
                    static_cast<unsigned>(ev.jbutton.button));
        }
        SDL_GameControllerUpdate();
        SDL_JoystickUpdate();
        const auto actions=exitGestures.update(
            exitDevice.poll(),showcaseMenu,Clock::now());
        toggleMenu |= actions.toggleMenu;
        confirmExit |= actions.confirmExit;
        cancelExit |= actions.cancelMenu;
        directExit |= actions.emergencyExit;
        if(directExit) {
          if(diagnoseInput)
            fprintf(stderr,"R36S_USER_EXIT=DELIBERATE_LONG_HOLD\\n");
          userQuit=true;
          break;
        }
        if(diagnoseInput && toggleMenu)
          fprintf(stderr,"R36S_EXIT_MENU_TOGGLED=LONG_HOLD MENU_WAS_OPEN=%d\\n",
                  showcaseMenu?1:0);
        if(toggleMenu) {
          showcaseMenu=!showcaseMenu;
          re4ipc_pause(showcaseMenu?1:0,500);
        } else if(showcaseMenu && confirmExit) {
          userQuit=true;
        } else if(showcaseMenu && cancelExit) {
          showcaseMenu=false;
          re4ipc_pause(0,500);
        }
 #else
        while(SDL_PollEvent(&ev)){
          if(ev.type==SDL_QUIT || (ev.type==SDL_KEYDOWN && ev.key.keysym.sym==SDLK_ESCAPE)){
             puts("USER_EXIT_EVENT=QUIT_OR_ESCAPE");userQuit=true;break;
          }
        }
#endif
        if(result||userQuit)break;
#ifndef RE4_R36S_SHOWCASE
        if(joy){
          SDL_GameControllerUpdate();
          const bool held=SDL_GameControllerGetButton(joy,SDL_CONTROLLER_BUTTON_BACK) &&
                          SDL_GameControllerGetButton(joy,SDL_CONTROLLER_BUTTON_START);
          if(held){
            if(quitHeldAt==Clock::time_point{})quitHeldAt=Clock::now();
            if(Clock::now()-quitHeldAt>=std::chrono::seconds(2)){
              printf("USER_EXIT_EVENT=SELECT_PLUS_START_2S\n");
              userQuit=true;break;
            }
          }else quitHeldAt=Clock::time_point{};
        }
#endif
        TRACE_MARK("core_state",processed,last);
        int state=re4ipc_state();
        if(state==RE4_CORE_FAILED || state==RE4_CORE_ENDED){
          printf("CORE_STATE=%d MSG=%s\n",state,re4ipc_message());result=14;break;
        }
        TRACE_MARK("input_controller",processed,last);
#ifdef RE4_R36S_SHOWCASE
        updatePad(joy,showcaseMenu);
        if(!showcaseMenu)re4ipc_vsync(nowNs());
#else
        updatePad(joy);
        TRACE_MARK("ipc_vsync",processed,last);
        re4ipc_vsync(nowNs());
#endif
        // Receive the SAME decoded GX stream that the Android core mode receives.
        TRACE_MARK("ipc_gx_acquire",processed,last);
#ifdef RE4_R36S_SHOWCASE
        const Re4GxChunk* acquired=showcaseMenu?nullptr:re4ipc_gx_acquire();
#else
        const Re4GxChunk* acquired=re4ipc_gx_acquire();
#endif
        if(const Re4GxChunk* c=acquired){
          if(c->version!=RE4_GX_FRAME_VERSION || c->opWords>(1u<<22) ||
             c->vertexBytes>(32u<<20) || c->blobBytes>(32u<<20)){
            fprintf(stderr,"CORE_GX_INVALID_PACKET=%u,%u,%u\n",c->version,c->opWords,c->blobBytes);
            re4ipc_gx_release(c);result=9;break;
          }
          if((c->flags & RE4_GX_CHUNK_RESET)!=0)resets++;
          if(last && c->sequence<=last) {
            fprintf(stderr,"CORE_GX_SEQUENCE_NOT_INCREASING=%u\n",c->sequence);
            re4ipc_gx_release(c);result=10;break;
          }
          last=c->sequence;
          if(processed<40 || processed%10==0){
            printf("TRACE_GX_FRAME_BEGIN=%u GX_SEQUENCE=%u OPS=%u VERTEX_BYTES=%u INDEX_COUNT=%u BLOB_BYTES=%u DRAWS=%u TEXTURE_BYTES=%u FLAGS=0x%x\n",
              processed+1,last,c->opWords,c->vertexBytes,c->indexCount,c->blobBytes,c->draws,c->textureBytes,c->flags);
          }
          TRACE_MARK("renderer_Run_GX",processed+1,last);
          renderer.Run(*c);
          TRACE_MARK("renderer_Run_return",processed+1,last);
          if(processed<40 || processed%10==0)puts("TRACE_RENDER_RUN_OK=1");
          glBindFramebuffer(GL_FRAMEBUFFER,0);
          glViewport(0,0,kW,kH);
          glClearColor(0,0,0,1);
          glClear(GL_COLOR_BUFFER_BIT);
          TRACE_MARK("renderer_Present",processed+1,last);
          if(renderer.Present(0,0,kW,kH))presented++;
#ifdef RE4_R36S_SHOWCASE
          if(showcaseMenu)showcaseExitOverlay();
#endif
          TRACE_MARK("renderer_Present_return",processed+1,last);
#ifndef RE4_R36S_SHOWCASE
          GLenum e=glGetError();
          if(e!=GL_NO_ERROR) {
            if(glErrors<12)printf("CORE_GX_GL_ERROR_FRAME_%u=0x%x\n",processed,e);
            glErrors++;
          }
#endif
          if(swapMode==1 || swapMode==4 || swapMode==5){
            TRACE_MARK("pre_swap_glFinish",processed+1,last);
            if(processed<30)printf("TRACE_GL_FINISH_BEGIN=%u\n",processed+1);
            glFinish();
            if(processed<30)printf("TRACE_GL_FINISH_OK=%u GL_ERROR=0x%x\n",
                                   processed+1,glGetError());
          }
          if(swapMode!=4){
            TRACE_MARK("egl_swap",processed+1,last);
            if(processed<30)printf("TRACE_EGL_SWAP_BEGIN=%u\n",processed+1);
            SDL_GL_SwapWindow(window);
            if(processed<30)printf("TRACE_EGL_SWAP_OK=%u\n",processed+1);
          }else{
            if(processed==0)puts("WARNING_OFFSCREEN_TEST_NO_SCREEN_UPDATE=YES");
          }
          TRACE_MARK("ipc_gx_release",processed+1,last);
          re4ipc_gx_release(c);
          TRACE_MARK("frame_complete",processed+1,last);
          if(processed<40 || processed%10==0)puts("TRACE_FRAME_COMPLETE=1");
          processed++;
          if(processed==1 || processed%10==0){
            const auto& s=renderer.Stats();
            printf("LIVE_FRAME=%u GX_SEQ=%u DRAWS=%u SHADERS=%u SHADER_ERRORS=%u\n",
              processed,last,s.draws,s.programs,s.shaderErrors);
          }
        }
#ifdef RE4_R36S_SHOWCASE
        else if(showcaseMenu){
          glBindFramebuffer(GL_FRAMEBUFFER,0);
          glViewport(0,0,kW,kH);
          glClearColor(0,0,0,1);
          glClear(GL_COLOR_BUFFER_BIT);
          renderer.Present(0,0,kW,kH);
          showcaseExitOverlay();
          glFinish(); // proven Mali mode 5 remains intact in exit menu
          SDL_GL_SwapWindow(window);
        }
#else
        // Diagnostic launcher used to discard sound; demo has a dedicated SDL audio feed.
        int16_t audio[512*2]{};
        TRACE_MARK("ipc_audio_drain",processed,last);
        re4ipc_audio(audio,512);
        TRACE_MARK("ipc_audio_return",processed,last);
#endif
        nextTick+=std::chrono::milliseconds(16);
        const auto current=Clock::now();
        if(nextTick>current) SDL_Delay((Uint32)std::chrono::duration_cast<std::chrono::milliseconds>(nextTick-current).count());
        else nextTick=current;
        if(++polls%150==0)printf("GAME_BOOT_HEARTBEAT=%u CORE_STATE=%d CHUNKS=%u PRESENTED=%u\n",polls,re4ipc_state(),processed,presented);
#ifndef RE4_R36S_SHOWCASE
        if(Clock::now()>=telemetryAt){
          const auto timeNow=Clock::now();
          const double seconds=std::chrono::duration<double>(timeNow-telemetryStart).count();
          const unsigned delta=processed-lastTelemetryFrames;
          const auto& stat=renderer.Stats();
          printf("PERFORMANCE_WINDOW_SECONDS=%.2f FRAMES=%u FPS_PRESENTED=%.2f TOTAL_FRAMES=%u DRAWS=%u TRIANGLES=%u SHADERS=%u SHADER_CACHE_HITS=%u\n",
              seconds,delta,seconds>0?delta/seconds:0.0,processed,stat.draws,stat.triangles,stat.programs,stat.programsLoaded);
#ifdef RE4_R36S_LOW_SPEC
          printf("LOW_SPEC_WINDOW DRAWS_CULLED=%u CULLED_TOTAL=%u DOF_SKIPPED=%u DOF_TOTAL=%u\n",
            stat.culledFarDraws-lastCulledFarDraws,stat.culledFarDraws,
            stat.skippedAutofocus-lastDofSkipped,stat.skippedAutofocus);
          lastCulledFarDraws=stat.culledFarDraws;
          lastDofSkipped=stat.skippedAutofocus;
#endif
#ifdef RE4_R36S_UPLOAD_PROFILE
          const unsigned uploadChunks=stat.geometryUploadChunks-lastGeometryChunks;
          const unsigned long long uploadUs=stat.geometryUploadMicros-lastGeometryUploadMicros;
          const unsigned long long uploadBytes=stat.geometryUploadBytes-lastGeometryUploadBytes;
          printf("GEOMETRY_UPLOAD_WINDOW FRAMES=%u CHUNKS=%u BYTES=%llu WALL_US=%llu AVG_MS_PER_CHUNK=%.3f ALLOC_CALLS=%u SUBDATA_CALLS=%u OVERSIZE_FALLBACKS=%u\n",
             delta, uploadChunks,uploadBytes,uploadUs,uploadChunks?double(uploadUs)/uploadChunks/1000.0:0.0,
             stat.geometryBufferAllocations-lastGeometryAlloc,
             stat.geometryBufferSubDataCalls-lastGeometrySubData,
             stat.geometryOversizeFallbacks);
          lastGeometryUploadMicros=stat.geometryUploadMicros;
          lastGeometryUploadBytes=stat.geometryUploadBytes;
          lastGeometryChunks=stat.geometryUploadChunks;
          lastGeometryAlloc=stat.geometryBufferAllocations;
          lastGeometrySubData=stat.geometryBufferSubDataCalls;
#endif
          memory("FRONTEND");
          coreMemory(corePid);
#ifdef RE4_R36S_SCALABLE_EFB
          systemMemory("PERFORMANCE_WINDOW");
#endif
          lastTelemetryFrames=processed;
          telemetryStart=timeNow;
          telemetryAt=timeNow+std::chrono::seconds(5);
        }
#endif
      }
      printf("SESSION_STOP_REASON=%s\n",userQuit?"user_exit":result?"error":"time_or_frame_limit");
#ifdef RE4_R36S_SHOWCASE
      showcaseAudio.stopAndClose();
      if(showcaseMenu)re4ipc_pause(0,500);
#endif
      TRACE_MARK("report_stats",processed,last);
      reportStats(renderer);
#ifdef RE4_R36S_SHADER_CACHE
      printf("SHADER_CACHE_FINAL_PROGRAMS=%u\n",renderer.Stats().programs);
      printf("SHADER_CACHE_FINAL_HITS=%u\n",renderer.Stats().programsLoaded);
      printf("SHADER_CACHE_FINAL_MISSES_OR_UNSUPPORTED=%u\n",
        renderer.Stats().programs-renderer.Stats().programsLoaded);
#endif
      re4ipc_gx_detach();
      unsigned a=0,b=0,c=0,d=0;
      re4ipc::checkCounts(&a,&b,&c,&d);
      printf("CORE_IPC_CHECKS_GX=%u MISMATCH=%u SNAPSHOTS=%u SNAPSHOT_MISMATCH=%u\n",a,b,c,d);
      printf("CORE_GX_RECEIVED=%u RESETS=%u PRESENTED=%u GL_ERRORS=%u\n",processed,resets,presented,glErrors);
      if(!result && !userQuit && (processed<1 || presented<1 || resets<1 || b || glErrors || renderer.Stats().shaderErrors)){
        result=11;
      }
      if(!result && userQuit) puts("RESULT=USER_EXIT_CLEAN");
      else if(!result) puts("RESULT=GAME_BOOT_RENDERED_FRAMES_PLAYABILITY_UNVERIFIED");
      else puts("RESULT=GAME_BOOT_NEEDS_INVESTIGATION");
      printf("GPU_TEST_MODE=%d FRAMES=%u PRESENTED_CALLS=%u SWAP_CALLED=%s\n",
              swapMode,processed,presented,swapMode==4?"NO":"YES");
      memory("END");
      coreMemory(corePid);
#ifdef RE4_R36S_SCALABLE_EFB
      systemMemory("SESSION_END");
#endif
      TRACE_MARK("renderer_release",processed,last);
      renderer.Release();
    }
  }catch(const std::exception& e){
    fprintf(stderr,"EXCEPTION=%s\n",e.what());result=12;
  }catch(...){
    fprintf(stderr,"EXCEPTION=unknown\n");result=12;
  }
  // The probe exits when START is pressed, but shutdown also closes its socket.
  pad={};
  re4ipc_pad(&pad);
  TRACE_MARK("ipc_shutdown",0,0);
  re4ipc::shutdown(5000);
#ifdef RE4_R36S_SHOWCASE
  exitDevice.release();
#endif
  if(joy)SDL_GameControllerClose(joy);
  TRACE_MARK("egl_cleanup",0,0);
  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();
  printf("EXIT_CODE=%d\n",result);
  TRACE_MARK("exit",0,0);
  return result;
}
