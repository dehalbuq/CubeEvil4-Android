// Experimental native Linux SDL2/GLES3 frontend for CubeEvil4 / R36S.
// No Android JNI, proprietary assets, or emulated console is required by this file.
// --probe tests EGL/GLES3 + upstream GX initialization without starting the game.
// Gameplay requires the user-built, matching AArch64 libre4_main.so and modules.
#include "gx_gl.h"
#include "re4_core.h"
#include "re4_gx_frame.h"

#include <SDL.h>
#include <GLES3/gl3.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <memory>
#include <string>
#include <thread>
#include <time.h>

namespace {
std::atomic<bool> stopRequested{false};
void signalStop(int) { stopRequested = true; }

struct Core {
    void* handle = nullptr;
    int (*reserve)(const char*, const char*) = nullptr;
    int (*start)() = nullptr;
    int (*state)() = nullptr;
    const char* (*message)() = nullptr;
    void (*pad)(const Re4CorePad*) = nullptr;
    void (*vsync)(int64_t) = nullptr;
    int (*audio)(int16_t*, int) = nullptr;
    const Re4GxChunk* (*acquire)() = nullptr;
    void (*release)(const Re4GxChunk*) = nullptr;
    void (*detach)() = nullptr;
    void (*depth)(const unsigned*, unsigned) = nullptr;

    template<typename T> bool bind(T& symbol, const char* name, bool required = true) {
        symbol = reinterpret_cast<T>(dlsym(handle, name));
        if (!symbol && required)
            std::fprintf(stderr, "[r36s] missing core symbol %s\n", name);
        return symbol || !required;
    }
    bool open(const std::string& filename) {
        handle = dlopen(filename.c_str(), RTLD_NOW | RTLD_GLOBAL);
        if (!handle) {
            std::fprintf(stderr, "[r36s] dlopen(%s): %s\n", filename.c_str(), dlerror());
            return false;
        }
        bool ok = true;
        ok = bind(reserve, "re4_core_reserve") && ok;
        ok = bind(start, "re4_core_start") && ok;
        ok = bind(state, "re4_core_state") && ok;
        ok = bind(message, "re4_core_message") && ok;
        ok = bind(pad, "re4_core_pad") && ok;
        ok = bind(vsync, "re4_core_vsync", false) && ok;
        ok = bind(audio, "re4_core_audio") && ok;
        ok = bind(acquire, "re4_core_gx_acquire") && ok;
        ok = bind(release, "re4_core_gx_release") && ok;
        ok = bind(detach, "re4_core_gx_detach") && ok;
        ok = bind(depth, "re4_core_gx_depth", false) && ok;
        return ok;
    }
    ~Core() { if (handle) dlclose(handle); }
};

struct Controller {
    SDL_GameController* mapped = nullptr;
    SDL_Joystick* raw = nullptr;
    SDL_JoystickID instance = -1;
    int startButton = 9, selectButton = 8;
    ~Controller() { close(); }
    void close() {
        if (mapped) SDL_GameControllerClose(mapped);
        else if (raw) SDL_JoystickClose(raw);
        mapped = nullptr;
        raw = nullptr;
        instance = -1;
    }
    void open() {
        if (raw || mapped) return;
        for (int i = 0; i < SDL_NumJoysticks(); ++i) {
            if (SDL_IsGameController(i)) {
                mapped = SDL_GameControllerOpen(i);
                if (mapped) raw = SDL_GameControllerGetJoystick(mapped);
            } else {
                raw = SDL_JoystickOpen(i);
            }
            if (raw) {
                instance = SDL_JoystickInstanceID(raw);
                std::fprintf(stderr, "[r36s] controller: %s mapped=%d buttons=%d start=%d select=%d\n",
                             SDL_JoystickName(raw), mapped != nullptr, SDL_JoystickNumButtons(raw),
                             startButton, selectButton);
                return;
            }
        }
    }
    bool button(SDL_GameControllerButton b) const {
        return mapped && SDL_GameControllerGetButton(mapped, b) != 0;
    }
    bool rawButton(int index) const {
        return raw && index >= 0 && index < SDL_JoystickNumButtons(raw) &&
               SDL_JoystickGetButton(raw, index) != 0;
    }
    int axis(SDL_GameControllerAxis a) const {
        if (!mapped) return 0;
        const int v = SDL_GameControllerGetAxis(mapped, a);
        return std::abs(v) < 8000 ? 0 : std::max(-100, std::min(100, v * 100 / 32767));
    }
    bool start() const { return button(SDL_CONTROLLER_BUTTON_START) || rawButton(startButton); }
    bool select() const { return button(SDL_CONTROLLER_BUTTON_BACK) || rawButton(selectButton); }
};

int rawIndex(const char* var, int fallback) {
    const char* s = std::getenv(var);
    if (!s || !*s) return fallback;
    char* end = nullptr;
    const long v = std::strtol(s, &end, 10);
    return end != s && *end == '\0' && v >= 0 && v <= 63 ? static_cast<int>(v) : fallback;
}

int64_t monoNs() {
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<int64_t>(ts.tv_sec) * 1000000000LL + ts.tv_nsec;
}

void audioCallback(void* userdata, Uint8* stream, int len) {
    auto* core = static_cast<Core*>(userdata);
    std::memset(stream, 0, static_cast<size_t>(len));
    if (core && core->audio && len > 0)
        core->audio(reinterpret_cast<int16_t*>(stream), len / 4); // 32 kHz, stereo, S16
}

std::string directoryOfExecutable() {
    char* base = SDL_GetBasePath();
    if (!base) return ".";
    const std::string path(base);
    SDL_free(base);
    return path;
}

bool has(const Uint8* keys, SDL_Scancode key) { return keys[key] != 0; }

Re4CorePad padState(const Controller& c) {
    Re4CorePad p{};
    const Uint8* k = SDL_GetKeyboardState(nullptr);
    const auto pressed = [&c](SDL_GameControllerButton b, SDL_Scancode key) {
        return c.button(b) || has(SDL_GetKeyboardState(nullptr), key);
    };
    if (pressed(SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_SCANCODE_LEFT))  p.buttons |= 0x0001;
    if (pressed(SDL_CONTROLLER_BUTTON_DPAD_RIGHT, SDL_SCANCODE_RIGHT)) p.buttons |= 0x0002;
    if (pressed(SDL_CONTROLLER_BUTTON_DPAD_DOWN, SDL_SCANCODE_DOWN))  p.buttons |= 0x0004;
    if (pressed(SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_SCANCODE_UP))      p.buttons |= 0x0008;
    if (pressed(SDL_CONTROLLER_BUTTON_A, SDL_SCANCODE_J))            p.buttons |= 0x0100;
    if (pressed(SDL_CONTROLLER_BUTTON_B, SDL_SCANCODE_K))            p.buttons |= 0x0200;
    if (pressed(SDL_CONTROLLER_BUTTON_X, SDL_SCANCODE_U))            p.buttons |= 0x0400;
    if (pressed(SDL_CONTROLLER_BUTTON_Y, SDL_SCANCODE_I))            p.buttons |= 0x0800;
    if (c.start() || has(k, SDL_SCANCODE_RETURN))                     p.buttons |= 0x1000;
    if (pressed(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, SDL_SCANCODE_Q)) p.buttons |= 0x0040;
    if (pressed(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, SDL_SCANCODE_E))p.buttons |= 0x0020;
    if (pressed(SDL_CONTROLLER_BUTTON_GUIDE, SDL_SCANCODE_TAB))      p.buttons |= 0x0010;
    p.stickX = static_cast<int8_t>(c.axis(SDL_CONTROLLER_AXIS_LEFTX));
    p.stickY = static_cast<int8_t>(-c.axis(SDL_CONTROLLER_AXIS_LEFTY));
    p.substickX = static_cast<int8_t>(c.axis(SDL_CONTROLLER_AXIS_RIGHTX));
    p.substickY = static_cast<int8_t>(-c.axis(SDL_CONTROLLER_AXIS_RIGHTY));
    if (has(k, SDL_SCANCODE_A)) p.stickX = -80;
    if (has(k, SDL_SCANCODE_D)) p.stickX = 80;
    if (has(k, SDL_SCANCODE_W)) p.stickY = 80;
    if (has(k, SDL_SCANCODE_S)) p.stickY = -80;
    if (c.mapped) {
        p.triggerL = static_cast<uint8_t>(SDL_GameControllerGetAxis(c.mapped, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / 128);
        p.triggerR = static_cast<uint8_t>(SDL_GameControllerGetAxis(c.mapped, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / 128);
    }
    return p;
}

void usage(const char* exe) {
    std::fprintf(stderr,
        "usage: %s [--probe] [--windowed] [--data DIR] [--modules DIR] [--core FILE]\n"
        "SDL2 + GLES3 native GX frontend. --probe needs no game files.\n"
        "Hold Start+Select (or Enter+Right Shift) for 1.5 seconds to exit.\n"
        "Alternative: Escape or SDL_QUIT; configurable raw button indices:\n"
        "RE4_R36S_START_BUTTON=9 RE4_R36S_SELECT_BUTTON=8\n", exe);
}
} // namespace

int main(int argc, char** argv) {
    bool probe = false, windowed = false;
    std::string data, modules, library;
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--probe") probe = true;
        else if (arg == "--windowed") windowed = true;
        else if (arg == "--data" && i + 1 < argc) data = argv[++i];
        else if (arg == "--modules" && i + 1 < argc) modules = argv[++i];
        else if (arg == "--core" && i + 1 < argc) library = argv[++i];
        else { usage(argv[0]); return 2; }
    }
    signal(SIGINT, signalStop);
    signal(SIGTERM, signalStop);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_EVENTS) != 0) {
        std::fprintf(stderr, "[r36s] SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    const std::string root = directoryOfExecutable();
    if (data.empty()) data = root + "data";
    if (modules.empty()) modules = root + "modules";
    if (library.empty()) library = root + "libre4_main.so";
    SDL_SetHint(SDL_HINT_VIDEO_X11_FORCE_EGL, "1");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_Window* window = SDL_CreateWindow("CubeEvil4 R36S - experimental",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI |
                              (windowed ? SDL_WINDOW_SHOWN : SDL_WINDOW_FULLSCREEN_DESKTOP));
    if (!window) {
        std::fprintf(stderr, "[r36s] SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_GLContext gl = SDL_GL_CreateContext(window);
    if (!gl || SDL_GL_MakeCurrent(window, gl) != 0) {
        std::fprintf(stderr, "[r36s] GLES3 context: %s (GLES2 is not supported by this renderer)\n", SDL_GetError());
        if (gl) SDL_GL_DeleteContext(gl);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    int major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const char* rendererName = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    std::fprintf(stderr, "[r36s] GLES version=%s major=%d minor=%d vendor=%s gpu=%s\n",
                 version ? version : "?", major, minor, vendor ? vendor : "?",
                 rendererName ? rendererName : "?");
    int result = 0;
    if (major < 3) {
        std::fprintf(stderr, "[r36s] requires GLES 3.0+, no software GLES2 fallback.\n");
        result = 1;
    } else {
        // Keep the actual existing GLES3 GX renderer alive on the SDL render thread.
        re4gx::GxRenderer gx;
        std::string problem;
        gx.SetOptions(re4gx::GxOptions{});
        if (!gx.Init(1, &problem)) {
            std::fprintf(stderr, "[r36s] GX initialization FAILED: %s\n", problem.c_str());
            result = 1;
        } else {
            std::fprintf(stderr, "[r36s] GX initialized: %s; lowmem=%s\n",
                         gx.ApiInfo().c_str(), std::getenv("RE4_R36S_LOW_MEM") ?
                         std::getenv("RE4_R36S_LOW_MEM") : "off");
            if (!probe) {
                Core core;
                if (!core.open(library) || !core.reserve(data.c_str(), modules.c_str()) ||
                    !core.start()) {
                    std::fprintf(stderr, "[r36s] core unavailable: %s\n",
                                 core.message ? core.message() : "no core library");
                    result = 1;
                } else {
                    gx.SetDepthSink([&core](const std::uint32_t* cells, std::uint32_t serial) {
                        if (core.depth) core.depth(cells, serial);
                    });
                    Controller controller;
                    controller.startButton = rawIndex("RE4_R36S_START_BUTTON", 9);
                    controller.selectButton = rawIndex("RE4_R36S_SELECT_BUTTON", 8);
                    controller.open();
                    SDL_AudioDeviceID audio = 0;
                    if (SDL_InitSubSystem(SDL_INIT_AUDIO) == 0) {
                        SDL_AudioSpec want{}, got{};
                        want.freq = 32000;
                        want.format = AUDIO_S16SYS;
                        want.channels = 2;
                        want.samples = 512;
                        want.callback = audioCallback;
                        want.userdata = &core;
                        audio = SDL_OpenAudioDevice(nullptr, 0, &want, &got, 0);
                        if (audio) SDL_PauseAudioDevice(audio, 0);
                        else std::fprintf(stderr, "[r36s] audio disabled: %s\n", SDL_GetError());
                    }
                    // Core's simulation gets a fixed 60 Hz retrace independently of GPU frame rate.
                    // Do NOT speed up gameplay when frames are skipped or slow it to GPU throughput.
                    std::atomic<bool> active{true};
                    std::thread clock([&]() {
                        auto at = std::chrono::steady_clock::now();
                        while (active && !stopRequested) {
                            at += std::chrono::nanoseconds(16666667);
                            if (core.vsync) core.vsync(monoNs());
                            std::this_thread::sleep_until(at);
                        }
                    });
                    uint64_t comboSince = 0, previousLog = SDL_GetTicks64();
                    unsigned long long chunks = 0;
                    while (!stopRequested) {
                        SDL_Event e{};
                        while (SDL_PollEvent(&e)) {
                            if (e.type == SDL_QUIT) stopRequested = true;
                            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
                                stopRequested = true;
                            if (e.type == SDL_CONTROLLERDEVICEADDED || e.type == SDL_JOYDEVICEADDED)
                                controller.open();
                            if ((e.type == SDL_CONTROLLERDEVICEREMOVED ||
                                 e.type == SDL_JOYDEVICEREMOVED) &&
                                e.jdevice.which == controller.instance) {
                                controller.close();
                                controller.open();
                            }
                        }
                        const Uint8* k = SDL_GetKeyboardState(nullptr);
                        const bool combo = (controller.start() && controller.select()) ||
                            (has(k, SDL_SCANCODE_RETURN) && has(k, SDL_SCANCODE_RSHIFT));
                        const uint64_t now = SDL_GetTicks64();
                        if (!combo) comboSince = 0;
                        else if (!comboSince) comboSince = now;
                        else if (now - comboSince >= 1500) {
                            std::fprintf(stderr, "[r36s] exit combo held 1500 ms: clean shutdown\n");
                            break;
                        }
                        const Re4CorePad pad = padState(controller);
                        core.pad(&pad);
                        for (int i = 0; i < 3; ++i) {
                            const Re4GxChunk* chunk = core.acquire();
                            if (!chunk) break;
                            gx.Run(*chunk);
                            core.release(chunk);
                            ++chunks;
                        }
                        int w = 640, h = 480;
                        SDL_GL_GetDrawableSize(window, &w, &h);
                        const int vw = std::min(w, h * 4 / 3), vh = std::min(h, w * 3 / 4);
                        const int vx = (w - vw) / 2, vy = (h - vh) / 2;
                        glBindFramebuffer(GL_FRAMEBUFFER, 0);
                        glViewport(0, 0, w, h);
                        glDisable(GL_SCISSOR_TEST);
                        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                        glClear(GL_COLOR_BUFFER_BIT);
                        gx.Present(vx, vy, vw, vh);
                        SDL_GL_SwapWindow(window);
                        if (now - previousLog >= 5000) {
                            std::fprintf(stderr, "[r36s] GX chunks=%llu core state=%d; %s\n",
                                         chunks, core.state(), core.message());
                            previousLog = now;
                        }
                        if (core.state() == RE4_CORE_FAILED || core.state() == RE4_CORE_ENDED) {
                            std::fprintf(stderr, "[r36s] game core stopped: %s\n", core.message());
                            result = 1;
                            break;
                        }
                        SDL_Delay(1);
                    }
                    active = false;
                    clock.join();
                    if (audio) SDL_CloseAudioDevice(audio);
                    if (core.detach) core.detach();
                    controller.close();
                    SDL_QuitSubSystem(SDL_INIT_AUDIO);
                }
            }
            gx.SetDepthSink(nullptr);
            gx.Release();
        }
    }
    SDL_GL_DeleteContext(gl);
    SDL_DestroyWindow(window);
    SDL_Quit();
    if (probe && result == 0) std::fprintf(stderr, "[r36s] GX / GLES3 probe PASS (no game tested)\n");
    return result;
}
