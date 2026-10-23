// Native Linux ARM64 (DarkOSRE): initialise CubeEvil4's REAL GX renderer
// through SDL2/KMSDRM and run a synthetic GameCube EFB -> XFB copy.
// Optional: pass a RE4GXCH1 frame dump to replay the same renderer.
// No game ISO, no network, and no root permissions required.
#include "gx_gl.h"
#include "re4_gx_frame.h"
#include "re4_gx_frame_io.h"

#include <SDL2/SDL.h>
#include <GLES3/gl3.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <sys/stat.h>

namespace {
constexpr int kWidth=640, kHeight=480;

void memory(const char* stage) {
  FILE* f=std::fopen("/proc/self/status","r");
  if(!f)return;
  char line[256];
  while(std::fgets(line,sizeof(line),f)){
    if(std::strncmp(line,"VmRSS:",6)==0 || std::strncmp(line,"VmHWM:",6)==0)
      std::printf("%s_%.*s",stage,(int)std::strlen(line),line);
  }
  std::fclose(f);
}

bool load(const char* path,re4gxio::Loaded& loaded) {
  struct stat st{};
  if(stat(path,&st)!=0 || st.st_size < 64 || st.st_size > 48*1024*1024) {
    std::fprintf(stderr,"ERROR=FRAME_DUMP_SIZE_INVALID (max 48MiB): %s\n",path);
    return false;
  }
  FILE* f=std::fopen(path,"rb");
  if(!f){std::perror("FRAME_OPEN");return false;}
  bool ok=re4gxio::Read(f,loaded);
  std::fclose(f);
  if(!ok || loaded.chunk.version != RE4_GX_FRAME_VERSION) {
    std::fprintf(stderr,"ERROR=FRAME_FORMAT_INVALID\n");
    return false;
  }
  std::printf("REPLAY_SEQUENCE=%u FRAME=%u OPS=%u VERTEX_BYTES=%u\n",
       loaded.chunk.sequence,loaded.frame,loaded.chunk.opWords,loaded.chunk.vertexBytes);
  return true;
}

struct Demo {
  uint32_t ops[8] = {
      RE4_GX_OP_BP | (1u<<8), (0x49u<<24),
      RE4_GX_OP_BP | (1u<<8), (0x4Au<<24) | (479u<<10) | 639u,
      RE4_GX_OP_BP | (1u<<8), (0x4Bu<<24),
      RE4_GX_OP_BP | (1u<<8), (0x52u<<24) | (1u<<14)
  };
  Re4GxChunk chunk{};
  Demo(){
    chunk.version=RE4_GX_FRAME_VERSION;
    chunk.sequence=1;
    chunk.flags=RE4_GX_CHUNK_RESET | RE4_GX_CHUNK_XFB;
    chunk.ops=ops;chunk.opWords=8;
    chunk.xfbWidth=kWidth;chunk.xfbHeight=kHeight;
  }
};

int go(int argc,char** argv) {
  const char* forced=std::getenv("SDL_VIDEODRIVER");
  std::printf("SDL_VIDEODRIVER=%s\n",forced?forced:"(auto)");
  std::printf("SDL_EGL=%s\n",std::getenv("SDL_VIDEO_EGL_DRIVER")?
              std::getenv("SDL_VIDEO_EGL_DRIVER"):"(default)");
  memory("BEFORE");
  if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)!=0) {
    std::fprintf(stderr,"SDL_INIT_ERROR=%s\n",SDL_GetError());return 2;
  }
  std::printf("SDL_DRIVER=%s\n",SDL_GetCurrentVideoDriver());
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,0);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
  SDL_Window* window=SDL_CreateWindow("CubeEvil4 GX - R36S",SDL_WINDOWPOS_CENTERED,
             SDL_WINDOWPOS_CENTERED,kWidth,kHeight,SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN);
  if(!window) {
    std::fprintf(stderr,"WINDOW_ERROR=%s\n",SDL_GetError());SDL_Quit();return 3;
  }
  SDL_GLContext ctx=SDL_GL_CreateContext(window);
  if(!ctx) {
    std::fprintf(stderr,"CONTEXT_ERROR=%s\n",SDL_GetError());
    SDL_DestroyWindow(window);SDL_Quit();return 4;
  }
  SDL_GL_MakeCurrent(window,ctx);
  SDL_GL_SetSwapInterval(1);
  std::printf("GL_VENDOR=%s\nGL_RENDERER=%s\nGL_VERSION=%s\n",
    glGetString(GL_VENDOR),glGetString(GL_RENDERER),glGetString(GL_VERSION));
  GLint maxTex=0, maxRender=0, maxUbo=0;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maxTex);
  glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE,&maxRender);
  glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE,&maxUbo);
  std::printf("GL_LIMITS_TEXTURE=%d RENDERBUFFER=%d UNIFORM_BLOCK=%d\n",maxTex,maxRender,maxUbo);
  int result=0;
  try {
    re4gx::GxRenderer renderer;
    re4gx::GxOptions options;
    options.samples=1; options.halfDof=true; options.tiledMsaa=false;
    renderer.SetOptions(options);
    std::string error;
    bool ok=renderer.Init(1,&error);
    std::printf("GX_INIT=%s\n",ok?"OK":"FAILED");
    if(!ok){
      std::fprintf(stderr,"GX_INIT_ERROR=%s\n",error.c_str());
      result=5;
    } else {
      // The upstream renderer checks optional extensions via glGetString in ES3;
      // clear any unrelated legacy GL error before judging frame operations.
      for(int i=0;i<16 && glGetError()!=GL_NO_ERROR;i++){}
      re4gxio::Loaded loaded;
      Demo demo;
      const Re4GxChunk* chunk=&demo.chunk;
      if(argc>1){
        if(!load(argv[1],loaded))result=6;
        else chunk=&loaded.chunk;
      }
      if(!result){
        renderer.Run(*chunk);
        const auto stats=renderer.Stats();
        std::printf("GX_RUN_CHUNKS=%u DRAWS=%u SHADERS=%u SHADER_ERRORS=%u DISPLAY_COPIES=%u\n",
          stats.chunks,stats.draws,stats.programs,stats.shaderErrors,stats.displayCopies);
        unsigned w=0,h=0;
        GLuint xfb=renderer.Xfb(&w,&h);
        std::printf("GX_XFB=%u WIDTH=%u HEIGHT=%u\n",xfb,w,h);
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        glViewport(0,0,kWidth,kHeight);
        glClearColor(0.1f,0.1f,0.15f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        bool presented=renderer.Present(0,0,kWidth,kHeight);
        GLenum glError=glGetError();
        std::printf("GX_PRESENT=%s GL_ERROR=0x%x\n",presented?"OK":"NO_XFB",glError);
        if(!presented || !xfb || glError!=GL_NO_ERROR || stats.shaderErrors)
          result=7;
        if(!result){
          for(int i=0;i<3;i++){
            SDL_GL_SwapWindow(window);
            SDL_Delay(350);
          }
          std::printf("RESULT=GX_RENDERER_INITIALIZED_AND_DISPLAY_COPY_OK\n");
        }
      }
      memory("AFTER");
      renderer.Release();
    }
  } catch(const std::exception& e){
    std::fprintf(stderr,"CXX_EXCEPTION=%s\n",e.what());result=8;
  } catch(...){
    std::fprintf(stderr,"CXX_EXCEPTION=unknown\n");result=8;
  }
  SDL_GL_DeleteContext(ctx);
  SDL_DestroyWindow(window);
  SDL_Quit();
  if(result)std::printf("RESULT=GX_RENDERER_FAILED CODE=%d\n",result);
  return result;
}
}

int main(int argc,char** argv){
  std::setvbuf(stdout,nullptr,_IONBF,0);
  std::puts("CUBEEVIL4_ORIGINAL_GX_RENDERER_LINUX_AARCH64");
  return go(argc,argv);
}
