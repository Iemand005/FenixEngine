#pragma once


#include <functional>
#include <iostream>
#include <memory>

#include <SDL3/SDL.h>

#include "IWindow.hpp"

namespace fe {


class SDLWindow : public IWindow {

  struct Impl;
  std::unique_ptr<Impl> impl;

  bool shouldClose = false;

  const bool* keyboardState = nullptr;

 public:
  bool capturingMouse = false;

  SDLWindow(std::string title, int width, int height, bool hidden = false);
  ~SDLWindow();

  void SetSwapInterval(int interval) override;

  void SetMouseCapture(bool captureMouse = true);

  void StartMouseCapture() override;

  void StopMouseCapture() override;

  // void SetMouseCapture(bool captureMouse = true);

  void GetSize(int* w, int* h);
  void Resize(int w, int h);

  void Move(int x, int y);

	void SetBordered(bool enabled);
	void SetFullscreen(bool enabled);

	void SetBorderless() {
		SetBordered(false);
	}

	void GoBorderlessFullSCreen() {
		SetBorderless();

		int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
		int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
		int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
		int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

		Move(x, y);
		Resize(w, h);
	}

  void Hide();
  void Show();

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

#ifdef _WIN32
  void AttachToNativeParent(void* parent);
#endif
};

}  // namespace fe
