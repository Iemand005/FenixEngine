#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#endif

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

	bool isFullscreen = false;
	bool _isScreensaving = false;
	float startX, startY;


 public:
  bool capturingMouse = false;

  SDLWindow(std::string title, int width, int height, bool hidden = false, bool fullscreen = false, WindowOptions options = {});
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
	void SetFullscreen(bool enabled = true);
  void ToggleFullscreen() {
    SetFullscreen(!isFullscreen);
  }

	void SetBorderless() {
		SetBordered(false);
	}

  void SetTitle(const char *newTitle) override;

	void GoBorderlessFullscreen();

  void Hide();
  void Show();

  void SwapBuffers() override;


// struct SDL_Window;
// enum SDL_Scancode;
  bool IsKeyDown(SDL_Scancode key);

  	void GetMousePosition(double *x, double *y);

  // union SDL_Event;

  bool PollSDLEvent(SDL_Event* event, bool getKeyboardState = true);;
  

  // struct SDL_Window;
  // struct SDL_GLContext;

  SDL_Window* GetWindow();

  SDL_GLContext GetSDLGLContext();

  void Destroy() override;

  double GetTime() override;

  void GetFramebufferSize(int *width, int *height);

  bool HideMouse();

	void ActivateScreenSaverMode();

	void AttachToNativeParent(void* parent);
#ifdef _WIN32
	HWND GetNativeWindow();
	HDC GetDrawingContext();
	HGLRC GetOpenGLRenderingContext();
#else
  void* GetWaylandSurface();
  void* GetWaylandDisplay();
#endif
};

}  // namespace fe
