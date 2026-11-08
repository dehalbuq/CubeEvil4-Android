// R36S Phase 2: SDL2 + GLES3 hardware compatibility probe.
// The actual game frontend is r36s_game_showcase_evdev_exit, using the
// separate re4_coreproc IPC process (port/r36s/gx-host).
#include "gx_gl.h"
#include <SDL2/SDL.h>
#include <GLES3/gl3.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace {
volatile std::sig_atomic_t stopRequested = 0;
void onSignal(int) { stopRequested = 1; }
}

int main(int argc, char** argv) {
  bool windowed = false;
  for (int i = 1; i < argc; ++i) {
    const std::string arg(argv[i]);
    if (arg == "--windowed") windowed = true;
    else if (arg != "--probe") {
      std::fprintf(stderr, "usage: %s [--probe] [--windowed]\n"
        "This diagnostic never starts the commercial game; use the IPC launcher.\n", argv[0]);
      return 2;
    }
  }
  std::signal(SIGINT, onSignal);
  std::signal(SIGTERM, onSignal);
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    std::fprintf(stderr, "R36S_SDL_INIT_FAILED=%s\n", SDL_GetError());
    return 1;
  }
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_Window* window = SDL_CreateWindow("CubeEvil4 R36S GLES3 probe",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480,
    SDL_WINDOW_OPENGL | (windowed ? SDL_WINDOW_SHOWN : SDL_WINDOW_FULLSCREEN_DESKTOP));
  if (!window) {
    std::fprintf(stderr, "R36S_SDL_WINDOW_FAILED=%s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }
  SDL_GLContext context = SDL_GL_CreateContext(window);
  if (!context || SDL_GL_MakeCurrent(window, context) != 0) {
    std::fprintf(stderr, "R36S_GLES_CONTEXT_FAILED=%s\n", SDL_GetError());
    if (context) SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  const char* gpu = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
  const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
  const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
  GLint major = 0;
  glGetIntegerv(GL_MAJOR_VERSION, &major);
  std::fprintf(stderr,"R36S_SDL_VIDEO=%s\nR36S_GPU=%s\nR36S_VENDOR=%s\nR36S_GLES=%s\n",
    SDL_GetCurrentVideoDriver(), gpu ? gpu : "(null)",
    vendor ? vendor : "(null)", version ? version : "(null)");
  int result = 1;
  if (major >= 3) {
    re4gx::GxRenderer renderer;
    re4gx::GxOptions opts;
    renderer.SetOptions(opts);
    std::string error;
    if (renderer.Init(1, &error)) {
      std::fprintf(stderr, "R36S_GX_INIT=PASS\nR36S_LOW_MEM=%s\n",
        std::getenv("RE4_R36S_LOW_MEM") ? std::getenv("RE4_R36S_LOW_MEM") : "off");
      renderer.Release();
      result = 0;
    } else std::fprintf(stderr, "R36S_GX_INIT=FAIL reason=%s\n",error.c_str());
  } else std::fprintf(stderr, "R36S_GLES3_REQUIRED=YES\n");
  SDL_Event event{};
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) stopRequested = 1;
  }
  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();
  std::fprintf(stderr, "R36S_PROBE_RESULT=%s\n", result ? "FAIL" : "PASS");
  return result;
}
