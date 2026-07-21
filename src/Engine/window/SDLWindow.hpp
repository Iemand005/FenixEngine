#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#else
//#include <x11/Xlib.h>
#endif

#include <memory>
#include <string>

#include <SDL3/SDL.h>

#include "IWindow.hpp"
#include "Joystick.hpp"

namespace fe {


class SDLWindow : public IWindow {

	struct Impl;
	std::unique_ptr<Impl> impl;

	bool shouldClose = false;

	const bool* keyboardState = nullptr;

  bool capturingMouse = false;


 public:

  SDLWindow(std::string title, int width, int height, bool hidden = false, bool fullscreen = false, WindowOptions options = {}, bool useVulkan = false);
  ~SDLWindow();

  void SetSwapInterval(int interval) override;

  void SetMouseCapture(bool captureMouse = true);

  void StartMouseCapture() override;

  void StopMouseCapture() override;

  bool IsCapturingMouse();

  // void SetMouseCapture(bool captureMouse = true);

  void GetSize(int* w, int* h);
  void Resize(int w, int h);

  void Move(int x, int y);

	void SetBordered(bool enabled);
	void SetFullscreen(bool enabled = false) override;
  

	void SetBorderless() {
		SetBordered(false);
	}

  void SetTitle(const char *newTitle) override;

	void GoBorderlessFullscreen() override;

  void Hide();
  void Show();

  void SwapBuffers() override;

  VulkanExtensions GetVulkanExtensions() override;
  void *CreateVulkanSurface(void *instance) override;

  WindowSize GetFramebufferSize() override;


// struct SDL_Window;
// enum SDL_Scancode;
  bool IsKeyDown(SDL_Scancode key);

  	void GetMousePosition(double *x, double *y);

  // union SDL_Event;

  bool PollSDLEvent(SDL_Event* event, bool getKeyboardState = true);
  

  // struct SDL_Window;
  // struct SDL_GLContext;

  SDL_Window* GetWindow();

  SDL_GLContext GetSDLGLContext();

  static void SDLCALL OnFileDialogResult(void* userdata, const char* const* files, int filter);

  void Destroy() override;

  void OpenFileDialog(FileDialogCallback callback, const char* filterName = "Model Files", const char* filterPattern = "*.glb;*.gltf;*.obj") override;

  double GetTime() override;

  void GetFramebufferSize(int *width, int *height);

  bool HideMouse();

  std::vector<Joystick> GetJoysticks();

  void UpdateJoysticks();

	void AttachToNativeParent(void* parent);
#ifdef _WIN32
	HWND GetNativeWindow();
	HDC GetDrawingContext();
	HGLRC GetOpenGLRenderingContext();
#else
  void* GetWaylandSurface();
  void* GetWaylandDisplay();

  void* GetX11Display();
  unsigned long GetGLXDrawable();
#endif
};

}  // namespace fe
