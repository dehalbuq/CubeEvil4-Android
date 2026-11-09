/* Mesa software EGL/GLES3 CI smoke test. No R36S Mali performance claim. */
#include <stdio.h>
#include <string.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

#ifndef EGL_OPENGL_ES3_BIT
#define EGL_OPENGL_ES3_BIT 0x0040
#endif
#ifndef EGL_PLATFORM_SURFACELESS_MESA
#define EGL_PLATFORM_SURFACELESS_MESA 0x31DD
#endif
static GLuint shader(GLenum type,const char*src){
 GLuint s=glCreateShader(type);
 glShaderSource(s,1,&src,NULL); glCompileShader(s);
 GLint pass=0;glGetShaderiv(s,GL_COMPILE_STATUS,&pass);
 if(!pass){char log[2048]={0};glGetShaderInfoLog(s,sizeof(log),NULL,log);fprintf(stderr,"GLSL_FAIL=%s\n",log);}
 if(!pass){glDeleteShader(s);return 0;}return s;
}
int main(void){
 EGLDisplay display=EGL_NO_DISPLAY;
 PFNEGLGETPLATFORMDISPLAYEXTPROC getPlatform = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
 if(getPlatform)display=getPlatform(EGL_PLATFORM_SURFACELESS_MESA,EGL_DEFAULT_DISPLAY,NULL);
 if(display==EGL_NO_DISPLAY)display=eglGetDisplay(EGL_DEFAULT_DISPLAY);
 EGLint major=0,minor=0;
 if(display==EGL_NO_DISPLAY||!eglInitialize(display,&major,&minor)){
  fprintf(stderr,"EGL_INIT=FAIL ERROR=0x%x\n",eglGetError());return 2;
 }
 if(!eglBindAPI(EGL_OPENGL_ES_API)){fprintf(stderr,"EGL_BIND=FAIL\n");return 3;}
 const EGLint configAttrib[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,
                               EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_NONE};
 EGLConfig config;EGLint count=0;
 if(!eglChooseConfig(display,configAttrib,&config,1,&count)||count<1){fprintf(stderr,"EGL_CONFIG_ES3=FAIL\n");return 4;}
 const EGLint pbAttrib[]={EGL_WIDTH,32,EGL_HEIGHT,32,EGL_NONE};
 EGLSurface surface=eglCreatePbufferSurface(display,config,pbAttrib);
 const EGLint ctxAttrib[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
 EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,ctxAttrib);
 if(surface==EGL_NO_SURFACE||context==EGL_NO_CONTEXT||!eglMakeCurrent(display,surface,surface,context)){
  fprintf(stderr,"EGL_CONTEXT=FAIL ERROR=0x%x\n",eglGetError());return 5;
 }
 printf("GLES_VENDOR=%s\nGLES_RENDERER=%s\nGLES_VERSION=%s\n",
        (const char*)glGetString(GL_VENDOR),(const char*)glGetString(GL_RENDERER),(const char*)glGetString(GL_VERSION));
 const char* v="#version 300 es\nlayout(location=0) in vec2 p; void main(){gl_Position=vec4(p,0.0,1.0);}";
 const char* f="#version 300 es\nprecision mediump float; out vec4 c; void main(){c=vec4(0.0,1.0,0.0,1.0);}";
 GLuint vs=shader(GL_VERTEX_SHADER,v),fs=shader(GL_FRAGMENT_SHADER,f);
 if(!vs||!fs){fprintf(stderr,"GLES3_SHADER=FAIL\n");return 6;}
 GLuint prog=glCreateProgram();glAttachShader(prog,vs);glAttachShader(prog,fs);glLinkProgram(prog);
 GLint linked=0;glGetProgramiv(prog,GL_LINK_STATUS,&linked);
 if(!linked){fprintf(stderr,"GLES3_PROGRAM=FAIL\n");return 7;}
 glViewport(0,0,32,32);glClearColor(0.0f,1.0f,0.0f,1.0f);glClear(GL_COLOR_BUFFER_BIT);
 unsigned char pixel[4]={0};glReadPixels(16,16,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
 if(glGetError()!=GL_NO_ERROR || pixel[1]<200){fprintf(stderr,"GLES3_PIXEL=FAIL (%u,%u,%u,%u)\n",pixel[0],pixel[1],pixel[2],pixel[3]);return 8;}
 printf("GLES3_SOFTWARE_SMOKE=PASS PIXEL=%u,%u,%u,%u\n",pixel[0],pixel[1],pixel[2],pixel[3]);
 glDeleteProgram(prog);glDeleteShader(vs);glDeleteShader(fs);
 eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
 eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);
 return 0;
}
