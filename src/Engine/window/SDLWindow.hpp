#pragma once


#include <functional>
#include <iostream>
#include <memory>

#include <SDL3/SDL.h>

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

  // void SetMouseCapture(bool captureMouse = true);

  void GetSize(int* w, int* h);

  void SwapBuffers() override;


// struct SDL_Window;
// enum SDL_Scancode;
  bool IsKeyDown(SDL_Scancode key);

  // union SDL_Event;

  bool PollSDLEvents(SDL_Event* event, bool getKeyboardState = true);;
  

  // struct SDL_Window;
  // struct SDL_GLContext;

  SDL_Window* GetSDLWindow();

  SDL_GLContext GetSDLGLContext();

  void Destroy() override;
};

}  // namespace fe
