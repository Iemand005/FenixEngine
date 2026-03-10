#pragma once


#include <functional>
#include <iostream>
#include <memory>

#include <dwmapi.h>
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

  struct Impl;
  std::unique_ptr<Impl> impl;

  bool shouldClose = false;

  const bool* keyboardState = nullptr;

 public:
  bool capturingMouse = false;

  SDLWindow(std::string title, int width, int height);
  ~SDLWindow();

  void SetSwapInterval(int interval) override;

  void SetMouseCapture(bool captureMouse = true);

  void StartMouseCapture() override;

  void StopMouseCapture() override;

  void GetSize(int* w, int* h);

  void SwapBuffers() override;

  // bool IsKeyDown(SDL_Scancode key) { return keyboardState[key]; }

  // bool PollSDLEvents(SDL_Event* event, bool getKeyboardState = true) {
  //   if (getKeyboardState) keyboardState = SDL_GetKeyboardState(NULL);
  //   if (!SDL_PollEvent(event)) return false;

  //   switch (event->type) {
  //     case SDL_EVENT_WINDOW_EXPOSED:
  //       if (resizeEvent) resizeEvent(width, height);
  //       break;
  //     case SDL_EVENT_WINDOW_RESIZED:
  //     case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
  //       width = event->window.data1;
  //       height = event->window.data2;
  //       if (resizeEvent) resizeEvent(width, height);
  //       break;
  //     case SDL_EVENT_MOUSE_MOTION:
  //       if (mouseMoveEvent && capturingMouse) {
  //         mouseMoveEvent(event->motion.xrel, event->motion.yrel);
  //       }
  //       break;
  //     default:
  //       break;
  //   }

  //   return true;
  // }

  SDL_Window* GetSDLWindow() { return window; }

  SDL_GLContext GetSDLGLContext() { return gl_context; }

  void Destroy() override;
};

}  // namespace fe
