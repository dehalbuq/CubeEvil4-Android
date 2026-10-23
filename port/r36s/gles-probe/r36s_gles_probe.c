/*
 * R36S DarkOSRE GLES smoke test; no ISO or proprietary game data required.
 * Uses dlopen so SDL2/GLES development headers are not needed.
 * License: MIT.
 */
#define _POSIX_C_SOURCE 200809L
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct SDL_Window SDL_Window;
typedef void* GLContext;
enum {
    SDL_INIT_VIDEO=0x20, SDL_WINDOW_OPENGL=2, SDL_WINDOW_SHOWN=4,
    SDL_GL_CONTEXT_MAJOR_VERSION=17, SDL_GL_CONTEXT_MINOR_VERSION=18,
    SDL_GL_CONTEXT_PROFILE_MASK=21, SDL_GL_CONTEXT_PROFILE_ES=4,
    GL_VENDOR=0x1f00, GL_RENDERER=0x1f01, GL_VERSION=0x1f02,
    GL_COLOR_BUFFER_BIT=0x4000
};
static int (*SDL_Init)(unsigned);
static void (*SDL_Quit)(void);
static const char* (*SDL_GetError)(void);
static int (*SDL_GetNumVideoDrivers)(void);
static const char* (*SDL_GetVideoDriver)(int);
static const char* (*SDL_GetCurrentVideoDriver)(void);
static int (*SDL_GL_SetAttribute)(int,int);
static SDL_Window* (*SDL_CreateWindow)(const char*,int,int,int,int,unsigned);
static void (*SDL_DestroyWindow)(SDL_Window*);
static GLContext (*SDL_GL_CreateContext)(SDL_Window*);
static void (*SDL_GL_DeleteContext)(GLContext);
static void* (*SDL_GL_GetProcAddress)(const char*);
static void (*SDL_GL_SwapWindow)(SDL_Window*);
static void (*SDL_Delay)(unsigned);

#define LOAD(name) do { \
  *(void **)(&name)=dlsym(lib,#name); \
  if (!(name)) { fprintf(stderr,"MISSING_SDL_SYMBOL=%s\n",#name);return 2; } \
} while(0)
#define GET_GL(name) do { *(void **)(&name)=SDL_GL_GetProcAddress(#name); \
  if (!(name)) {fprintf(stderr,"MISSING_GL_SYMBOL=%s\n",#name);err=5;goto done;} } while(0)

int main(int argc,char**argv) {
    setvbuf(stdout,NULL,_IONBF,0);
    puts("CUBEEVIL4_R36S_GPU_SMOKE_TEST");
    void *lib=dlopen("libSDL2-2.0.so.0",RTLD_NOW|RTLD_LOCAL);
    if(!lib) {fprintf(stderr,"SDL_DLOPEN_ERROR=%s\n",dlerror());return 2;}
    LOAD(SDL_Init); LOAD(SDL_Quit); LOAD(SDL_GetError);
    LOAD(SDL_GetNumVideoDrivers); LOAD(SDL_GetVideoDriver);
    LOAD(SDL_GetCurrentVideoDriver); LOAD(SDL_GL_SetAttribute);
    LOAD(SDL_CreateWindow); LOAD(SDL_DestroyWindow);
    LOAD(SDL_GL_CreateContext); LOAD(SDL_GL_DeleteContext);
    LOAD(SDL_GL_GetProcAddress); LOAD(SDL_GL_SwapWindow); LOAD(SDL_Delay);
    int drivers=SDL_GetNumVideoDrivers();
    for(int i=0;i<drivers;i++)printf("SDL_DRIVER_%d=%s\n",i,SDL_GetVideoDriver(i));
    if(argc>1 && strcmp(argv[1],"--list")==0) return 0;
    if(SDL_Init(SDL_INIT_VIDEO)!=0) {
        fprintf(stderr,"SDL_INIT_ERROR=%s\n",SDL_GetError());return 3;
    }
    printf("SDL_ACTIVE_DRIVER=%s\n",SDL_GetCurrentVideoDriver());
    SDL_Window* window=NULL; GLContext context=NULL; int requested=0,err=0;
    for(int major=3;major>=2;major--) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,major);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,0);
        window=SDL_CreateWindow("R36S OpenGL ES test",0,0,640,480,
                                 SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN);
        if(!window) {
            fprintf(stderr,"WINDOW_ES%d_ERROR=%s\n",major,SDL_GetError());
            continue;
        }
        context=SDL_GL_CreateContext(window);
        if(context) {requested=major;break;}
        fprintf(stderr,"CONTEXT_ES%d_ERROR=%s\n",major,SDL_GetError());
        SDL_DestroyWindow(window);window=NULL;
    }
    if(!context) {fprintf(stderr,"RESULT=NO_GL_CONTEXT\n");err=4;goto done;}
    const unsigned char* (*glGetString)(unsigned)=NULL;
    void (*glClearColor)(float,float,float,float)=NULL;
    void (*glClear)(unsigned)=NULL;
    unsigned (*glGetError)(void)=NULL;
    GET_GL(glGetString); GET_GL(glClearColor); GET_GL(glClear); GET_GL(glGetError);
    const char* vendor=(const char*)glGetString(GL_VENDOR);
    const char* renderer=(const char*)glGetString(GL_RENDERER);
    const char* version=(const char*)glGetString(GL_VERSION);
    printf("REQUESTED_ES=%d\nGL_VENDOR=%s\nGL_RENDERER=%s\nGL_VERSION=%s\n",
           requested,vendor?vendor:"(null)",renderer?renderer:"(null)",version?version:"(null)");
    if(!version||!renderer) {err=6;goto done;}
    const float rgb[3][3]={{0.03f,0.25f,0.6f},{0.02f,0.55f,0.18f},{0.6f,0.15f,0.12f}};
    for(int i=0;i<3;i++) {
        glClearColor(rgb[i][0],rgb[i][1],rgb[i][2],1);
        glClear(GL_COLOR_BUFFER_BIT);
        unsigned e=glGetError();
        printf("FRAME_%d_GL_ERROR=0x%x\n",i+1,e);
        if(e)err=7;
        SDL_GL_SwapWindow(window);
        SDL_Delay(900);
    }
    if(strstr(renderer,"llvmpipe")||strstr(renderer,"softpipe")) {
       printf("WARNING=SOFTWARE_RENDERER\n");if(!err)err=8;
    }
    if(requested<3) {printf("WARNING=GLES3_NOT_AVAILABLE\n");if(!err)err=9;}
    printf("RESULT=%s\n",err?"REVIEW_LOG":"GLES3_DRAW_OK");
done:
    if(context) SDL_GL_DeleteContext(context);
    if(window) SDL_DestroyWindow(window);
    SDL_Quit();
    return err;
}
