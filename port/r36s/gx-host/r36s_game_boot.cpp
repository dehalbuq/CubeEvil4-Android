// EXPERIMENT: original CubeEvil4 GX renderer receiving live GX chunks from
// the project's existing re4_coreproc shared-memory IPC (Linux AArch64).
// Default uses the upstream synthetic remote core test, with NO game data.
// The core process is separate: its 32-bit game address constraints cannot
// collide with SDL2/EGL/GL allocations in this process.
#include "gx_gl.h"
#include "re4_core_client.h"
#include "re4_gx_frame.h"
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

namespace {
using Clock=std::chrono::steady_clock;
constexpr int kW=640,kH=480;

int64_t nowNs() {
  const auto ns=std::chrono::duration_cast<std::chrono::nanoseconds>(
                    Clock::now().time_since_epoch()).count();
  return (int64_t)ns;
}
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
void updatePad(SDL_GameController* joy){
  Re4CorePad pad{};
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
}
} // namespace

int main(int argc,char**argv) {
  setvbuf(stdout,nullptr,_IONBF,0);
  puts("CUBEEVIL4_R36S_FIRST_REAL_GAME_DATA_BOOT");
  if(argc<4){
    fprintf(stderr,"usage: r36s_game_boot COREFOLDER MODULESFOLDER DATADIR [SECONDS]\n");
    return 2;
  }
  const std::string coreDir=argv[1],modules=argv[2],dataDir=argv[3];
  if(!checkGameData(dataDir))return 20;
  int sec=argc>4?atoi(argv[4]):45;
  sec=std::max(5,std::min(sec,90));
  printf("RUN_MODE=GAME_BOOT MAX_SECONDS=%d\n",sec);
  // This entry is synthetic and cannot execute original game data.
  unsetenv("RE4_CORE_TEST_ENTRY");
  setenv("RE4_CORE_IPC_CHECK","1",1);
  re4ipc::Spawn spawn;
  spawn.program=coreDir+"/re4_coreproc";
  // core_ipc_client.cpp deliberately strips inherited LD_LIBRARY_PATH
  // before execve; explicitly supply the folder containing libre4_*.so.
  spawn.libraryDir=coreDir;
  spawn.helloSeconds=30;
  re4ipc::configure(spawn);
  memory("START");
  if(re4ipc_reserve(dataDir.c_str(),modules.c_str())!=1){
    fprintf(stderr,"CORE_IPC_RESERVE_FAILED=%s\n",re4ipc_message());
    re4ipc::shutdown(1000);
    return 3;
  }
  Re4CorePad pad{};
  // Keep controls neutral until the first SDL input poll.
  re4ipc_pad(&pad);
  if(re4ipc_start()!=1){
    fprintf(stderr,"CORE_IPC_START_FAILED=%s\n",re4ipc_message());
    re4ipc::shutdown(1000);
    return 4;
  }
  printf("CORE_PID=%d\n",re4ipc::pid());
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
  SDL_GLContext context=SDL_GL_CreateContext(window);
  if(!context){
    fprintf(stderr,"SDL_CONTEXT_ERROR=%s\n",SDL_GetError());
    SDL_DestroyWindow(window);SDL_Quit();re4ipc::shutdown(3000);return 7;
  }
  SDL_GL_MakeCurrent(window,context);
  SDL_GL_SetSwapInterval(1);
  printf("GL_RENDERER=%s\nGL_VERSION=%s\n",glGetString(GL_RENDERER),glGetString(GL_VERSION));
  int result=0;unsigned processed=0,resets=0,presented=0,glErrors=0;
  try {
    re4gx::GxRenderer renderer;
    re4gx::GxOptions opts;
    opts.samples=1;opts.tiledMsaa=false;
    renderer.SetOptions(opts);
    std::string error;
    if(!renderer.Init(1,&error)){
      fprintf(stderr,"GX_INIT_ERROR=%s\n",error.c_str());
      result=8;
    } else {
      const auto deadline=Clock::now()+std::chrono::seconds(sec);
      uint32_t last=0;
      auto nextTick=Clock::now();
      unsigned polls=0;
      while(Clock::now()<deadline && processed<6000 && !result){
        SDL_Event ev{};
        while(SDL_PollEvent(&ev)){
          if(ev.type==SDL_QUIT || (ev.type==SDL_KEYDOWN && ev.key.keysym.sym==SDLK_ESCAPE)){
             printf("USER_CANCEL=1\n");result=13;break;
          }
        }
        if(result)break;
        int state=re4ipc_state();
        if(state==RE4_CORE_FAILED || state==RE4_CORE_ENDED){
          printf("CORE_STATE=%d MSG=%s\n",state,re4ipc_message());result=14;break;
        }
        updatePad(joy);
        re4ipc_vsync(nowNs());
        // Receive the SAME decoded GX stream that the Android core mode receives.
        if(const Re4GxChunk* c=re4ipc_gx_acquire()){
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
          renderer.Run(*c);
          glBindFramebuffer(GL_FRAMEBUFFER,0);
          glViewport(0,0,kW,kH);
          glClearColor(0,0,0,1);
          glClear(GL_COLOR_BUFFER_BIT);
          if(renderer.Present(0,0,kW,kH))presented++;
          GLenum e=glGetError();
          if(e!=GL_NO_ERROR) {
            if(glErrors<12)printf("CORE_GX_GL_ERROR_FRAME_%u=0x%x\n",processed,e);
            glErrors++;
          }
          SDL_GL_SwapWindow(window);
          re4ipc_gx_release(c);
          processed++;
          if(processed==1 || processed%10==0){
            const auto& s=renderer.Stats();
            printf("LIVE_FRAME=%u GX_SEQ=%u DRAWS=%u SHADERS=%u SHADER_ERRORS=%u\n",
              processed,last,s.draws,s.programs,s.shaderErrors);
          }
        }
        // Drain generated test audio to keep the core's IPC ring moving.
        int16_t audio[512*2]{};
        re4ipc_audio(audio,512);
        nextTick+=std::chrono::milliseconds(16);
        const auto current=Clock::now();
        if(nextTick>current) SDL_Delay((Uint32)std::chrono::duration_cast<std::chrono::milliseconds>(nextTick-current).count());
        else nextTick=current;
        if(++polls%150==0)printf("GAME_BOOT_HEARTBEAT=%u CORE_STATE=%d CHUNKS=%u PRESENTED=%u\n",polls,re4ipc_state(),processed,presented);
      }
      reportStats(renderer);
      re4ipc_gx_detach();
      unsigned a=0,b=0,c=0,d=0;
      re4ipc::checkCounts(&a,&b,&c,&d);
      printf("CORE_IPC_CHECKS_GX=%u MISMATCH=%u SNAPSHOTS=%u SNAPSHOT_MISMATCH=%u\n",a,b,c,d);
      printf("CORE_GX_RECEIVED=%u RESETS=%u PRESENTED=%u GL_ERRORS=%u\n",processed,resets,presented,glErrors);
      if(!result && (processed<1 || presented<1 || resets<1 || b || glErrors || renderer.Stats().shaderErrors)){
        result=11;
      }
      if(!result) puts("RESULT=GAME_BOOT_RENDERED_FRAMES_PLAYABILITY_UNVERIFIED");
      else puts("RESULT=GAME_BOOT_NEEDS_INVESTIGATION");
      memory("END");
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
  re4ipc::shutdown(5000);
  if(joy)SDL_GameControllerClose(joy);
  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();
  printf("EXIT_CODE=%d\n",result);
  return result;
}
