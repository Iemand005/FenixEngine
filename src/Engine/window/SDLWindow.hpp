#pragma once
#include <SDL3/SDL.h>
#include <dwmapi.h>
#include <glad/glad.h>

#include <functional>
#include <iostream>
#pragma comment(lib, "dwmapi.lib")

#include "IWindow.hpp"

namespace fe {

inline LRESULT CALLBACK CustomWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  WNDPROC ogProc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  LRESULT res = CallWindowProc(ogProc, hwnd, msg, wParam, lParam);

  switch (msg) {
    case WM_MOVING:
    case WM_TIMER: {
      DwmFlush();
    }
  }

  return res;
}
class SDLWindow : public IWindow {
  SDL_Window* window;
  SDL_GLContext gl_context;
  bool shouldClose = false;

  const bool* keyboardState = nullptr;

  void SDL_FlushOnResizeAndMove(SDL_Window* window) {
    HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    if (hwnd) {
      WNDPROC ogProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)CustomWndProc);
      SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)(ogProc));
    }
  }

  static bool SDLCALL EventWatch(void* userdata, SDL_Event* event) {
    SDLWindow* window = (SDLWindow*)userdata;
    switch (event->type) {
      case SDL_EVENT_WINDOW_EXPOSED:
        // DwmFlush();

        if (window->resizeEvent) window->resizeEvent(window->width, window->height);
        break;
      case SDL_EVENT_WINDOW_RESIZED:
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        // 31st of August :3
        // DwmFlush();

        window->width = event->window.data1, window->height = event->window.data2;
        // if (window->resizeEvent) window->resizeEvent(window->width, window->height);
        break;

      case SDL_EVENT_MOUSE_MOTION:
        if (window->mouseMoveEvent && window->capturingMouse) window->mouseMoveEvent(event->motion.xrel, event->motion.yrel);
        if (window->capturingMouse) {
          // SDL_WarpMouseInWindow(window->window, window->width/2.0f, window->height/2.0f);
        }
        break;
    }
    return false;
  }

  static void CheckError(bool success = false) {
    if (!success) {
      auto error = SDL_GetError();
      std::cerr << "SDL Error: " << error;
    }
  }

 public:
  bool capturingMouse = false;

  SDLWindow(std::string title, int width, int height) : IWindow(width, height) {
    CheckError(SDL_Init(SDL_INIT_VIDEO));

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

    window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!window) {
      CheckError();
      SDL_Quit();
    }

    // SDL_FlushOnResizeAndMove(window);

    gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
      CheckError();
      SDL_DestroyWindow(window);
      SDL_Quit();
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
      std::cout << "Failed to initialize GLAD" << std::endl;
      return;
    }

    keyboardState = SDL_GetKeyboardState(NULL);

    // SDL_AddEventWatch(EventWatch, this);
  }
  void SetSwapInterval(int interval) override { SDL_GL_SetSwapInterval(interval); }

  void SetMouseCapture(bool captureMouse = true) {
    SDL_SetWindowMouseGrab(window, captureMouse);
    SDL_SetWindowRelativeMouseMode(window, captureMouse);
    capturingMouse = captureMouse;
  }

  void StartMouseCapture() override {
    SetMouseCapture(true);
    SDL_HideCursor();
  }

  void StopMouseCapture() override {
    SetMouseCapture(false);
    SDL_ShowCursor();

    // io.WantCaptureMouse = true;
  }

  void GetSize(int* w, int* h) { SDL_GetWindowSize(window, w, h); }

  void SwapBuffers() override {
    // DwmFlush();
    // DwmFlush();
    // DwmFlush();
    // DwmFlush();
    // DwmFlush();
    // DwmFlush();
    // DwmFlush();
    // DwmFlush();
    // DwmFlush();
    // DwmFlush();
    // DwmFlush();
    // DwmFlush();
    SDL_GL_SwapWindow(window);
  }

  bool IsKeyDown(SDL_Scancode key) { return keyboardState[key]; }

  bool PollSDLEvents(SDL_Event* event, bool getKeyboardState = true) {
    if (getKeyboardState) keyboardState = SDL_GetKeyboardState(NULL);
    if (!SDL_PollEvent(event)) return false;

    switch (event->type) {
      case SDL_EVENT_WINDOW_EXPOSED:
        if (resizeEvent) resizeEvent(width, height);
        break;
      case SDL_EVENT_WINDOW_RESIZED:
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        width = event->window.data1;
        height = event->window.data2;
        if (resizeEvent) resizeEvent(width, height);
        break;
      case SDL_EVENT_MOUSE_MOTION:
        if (mouseMoveEvent && capturingMouse) {
          mouseMoveEvent(event->motion.xrel, event->motion.yrel);
        }
        break;
      default:
        break;
    }

    return true;
  }

  SDL_Window* GetSDLWindow() { return window; }

  SDL_GLContext GetSDLGLContext() { return gl_context; }

  void Destroy() override {
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
  }
};

}  // namespace fe
