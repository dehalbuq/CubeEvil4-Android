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
  puts("CUBEEVIL4_R36S_CORE_PROCESS_TO_REAL_GX_LIVE_TEST");
  if(argc<3){
    fprintf(stderr,"usage: r36s_live_ipc COREFOLDER MODULESFOLDER [SECONDS]\n");
    return 2;
  }
  const std::string coreDir=argv[1],modules=argv[2];
  int sec=argc>3?atoi(argv[3]):4;
  sec=std::max(1,std::min(sec,8));
  // This entry is synthetic and cannot execute original game data.
  setenv("RE4_CORE_TEST_ENTRY","remote",1);
  setenv("RE4_CORE_IPC_CHECK","1",1);
  re4ipc::Spawn spawn;
  spawn.program=coreDir+"/re4_coreproc";
  // core_ipc_client.cpp deliberately strips inherited LD_LIBRARY_PATH
  // before execve; explicitly supply the folder containing libre4_*.so.
  spawn.libraryDir=coreDir;
  spawn.helloSeconds=30;
  re4ipc::configure(spawn);
  memory("START");
  if(re4ipc_reserve("/nonexistent-re4-data",modules.c_str())!=1){
    fprintf(stderr,"CORE_IPC_RESERVE_FAILED=%s\n",re4ipc_message());
    re4ipc::shutdown(1000);
    return 3;
  }
  Re4CorePad pad{};
  pad.buttons=0x0100;pad.stickX=50;pad.stickY=-20;pad.triggerR=200;
  re4ipc_pad(&pad);
  if(re4ipc_start()!=1){
    fprintf(stderr,"CORE_IPC_START_FAILED=%s\n",re4ipc_message());
    re4ipc::shutdown(1000);
    return 4;
  }
  printf("CORE_PID=%d\n",re4ipc::pid());
  if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)!=0) {
    fprintf(stderr,"SDL_ERROR=%s\n",SDL_GetError());
    re4ipc::shutdown(3000);return 5;
  }
  printf("SDL_DRIVER=%s\n",SDL_GetCurrentVideoDriver());
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
      while(Clock::now()<deadline && processed<120 && !result){
        SDL_Event ev{};
        while(SDL_PollEvent(&ev)){
          if(ev.type==SDL_QUIT || (ev.type==SDL_KEYDOWN && ev.key.keysym.sym==SDLK_ESCAPE)){
             printf("USER_CANCEL=1\n");result=13;break;
          }
        }
        if(result)break;
        int state=re4ipc_state();
        if(state==RE4_CORE_FAILED || state==RE4_CORE_ENDED){
          printf("CORE_STATE=%d MSG=%s\n",state,re4ipc_message());break;
        }
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
      }
      reportStats(renderer);
      re4ipc_gx_detach();
      unsigned a=0,b=0,c=0,d=0;
      re4ipc::checkCounts(&a,&b,&c,&d);
      printf("CORE_IPC_CHECKS_GX=%u MISMATCH=%u SNAPSHOTS=%u SNAPSHOT_MISMATCH=%u\n",a,b,c,d);
      printf("CORE_GX_RECEIVED=%u RESETS=%u PRESENTED=%u GL_ERRORS=%u\n",processed,resets,presented,glErrors);
      if(!result && (processed<3 || presented<2 || resets<1 || b || glErrors || renderer.Stats().shaderErrors)){
        result=11;
      }
      if(!result) puts("RESULT=LIVE_CORE_IPC_GX_RENDER_OK");
      else puts("RESULT=LIVE_CORE_IPC_GX_NEEDS_INVESTIGATION");
      memory("END");
      renderer.Release();
    }
  }catch(const std::exception& e){
    fprintf(stderr,"EXCEPTION=%s\n",e.what());result=12;
  }catch(...){
    fprintf(stderr,"EXCEPTION=unknown\n");result=12;
  }
  // The probe exits when START is pressed, but shutdown also closes its socket.
  pad={};pad.buttons=0x1000;
  re4ipc_pad(&pad);
  re4ipc::shutdown(5000);
  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();
  printf("EXIT_CODE=%d\n",result);
  return result;
}
