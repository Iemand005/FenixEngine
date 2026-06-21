#include <SDL3/SDL.h>
#include <glad/glad.h>

#include "SDLWindow.hpp"

#ifdef WIN32
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

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
#endif

inline void CheckError(bool success = false) {
  if (!success) {
    auto error = SDL_GetError();
    std::cerr << "SDL Error: " << error;
  }
}

// inline bool SDLCALL EventWatch(void* userdata, SDL_Event* event) {
//   SDLWindow* window = (SDLWindow*)userdata;
//   switch (event->type) {
//     case SDL_EVENT_WINDOW_EXPOSED:
//       if (window->resizeEvent) window->resizeEvent(window->width, window->height);
//       break;
//     case SDL_EVENT_WINDOW_RESIZED:
//     case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
//       window->width = event->window.data1, window->height = event->window.data2;
//       break;

//     case SDL_EVENT_MOUSE_MOTION:
//       if (window->mouseMoveEvent && window->capturingMouse) window->mouseMoveEvent(event->motion.xrel, event->motion.yrel);
//       if (window->capturingMouse) {
//         SDL_WarpMouseInWindow(window->impl->window, window->width/2.0f, window->height/2.0f);
//       }
//       break;
//   }
//   return false;
// }

struct fe::SDLWindow::Impl {
  SDL_Window* window;
  SDL_GLContext gl_context;

  void SDL_FlushOnResizeAndMove(SDL_Window* window) {
#ifdef WIN32
    HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    if (hwnd) {
      WNDPROC ogProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)CustomWndProc);
      SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)(ogProc));
    }
#endif
  }

    

  
}; // Impl

fe::SDLWindow::~SDLWindow() {
  Destroy();
}

fe::SDLWindow::SDLWindow(std::string title, int width, int height, bool hidden, bool fullscreen) : IWindow(width, height) {
	impl = std::make_unique<Impl>();
	CheckError(SDL_Init(SDL_INIT_VIDEO));

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

	auto windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
	if (hidden) windowFlags |= SDL_WINDOW_HIDDEN;

	impl->window = SDL_CreateWindow(title.c_str(), width, height, windowFlags);

	if (!impl->window) {
		CheckError();
		SDL_Quit();
	}

    // SDL_FlushOnResizeAndMove(window);

    impl->gl_context = SDL_GL_CreateContext(impl->window);
    if (!impl->gl_context) {
      CheckError();
      SDL_DestroyWindow(impl->window);
      SDL_Quit();
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
      std::cout << "Failed to initialize GLAD" << std::endl;
      return;
    }

    keyboardState = SDL_GetKeyboardState(NULL);

    // SDL_AddEventWatch(EventWatch, this);
}

void fe::SDLWindow::SwapBuffers() {
	SDL_GL_SwapWindow(impl->window);
}

void fe::SDLWindow::SetSwapInterval(int interval) {
	SDL_GL_SetSwapInterval(interval);
}

void fe::SDLWindow::SetMouseCapture(bool captureMouse) {
	SDL_SetWindowMouseGrab(impl->window, captureMouse);
	SDL_SetWindowRelativeMouseMode(impl->window, captureMouse);
	capturingMouse = captureMouse;
}

void fe::SDLWindow::StartMouseCapture() {
	SetMouseCapture(true);
	SDL_HideCursor();
	}

void fe::SDLWindow::StopMouseCapture() {
	SetMouseCapture(false);
	SDL_ShowCursor();
}

void fe::SDLWindow::GetSize(int* w, int* h) { SDL_GetWindowSize(impl->window, w, h); }


  void fe::SDLWindow::Destroy() {
    if (!impl) return;

    if (impl->gl_context) {
      SDL_GL_DestroyContext(impl->gl_context);
      impl->gl_context = nullptr;
    }

    if (impl->window) {
      SDL_DestroyWindow(impl->window);
      impl->window = nullptr;
    }

    SDL_Quit();
  }

SDL_Window* fe::SDLWindow::GetWindow() { return impl->window; }

SDL_GLContext fe::SDLWindow::GetSDLGLContext() { return impl->gl_context; }

 bool fe::SDLWindow::IsKeyDown(SDL_Scancode key) { return keyboardState[key]; }

  bool fe::SDLWindow::PollSDLEvents(SDL_Event* event, bool getKeyboardState) {
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
          SDL_WarpMouseInWindow(impl->window, width/2.0f, height/2.0f);
        }
        break;
      default:
        break;
    }

    return true;
  }

void fe::SDLWindow::Resize(int w, int h) {
  SDL_SetWindowSize(this->impl->window, w, h);
}

#ifdef _WIN32
#include <SDL3/SDL.h>
#include <windows.h>

void fe::SDLWindow::AttachToNativeParent(void* parent)
{
	if (!parent)
		return;

	SDL_PropertiesID props = SDL_GetWindowProperties(GetWindow());
	if (!props)
		return;

	HWND hwnd = (HWND)SDL_GetPointerProperty(
		props,
		SDL_PROP_WINDOW_WIN32_HWND_POINTER,
		NULL
	);

	if (!hwnd)
		return;

	SetParent(hwnd, (HWND)parent);

	LONG style = GetWindowLong(hwnd, GWL_STYLE);

	style &= ~(WS_POPUP | WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_THICKFRAME);
	style |= WS_CHILD;

	SetWindowLong(hwnd, GWL_STYLE, style);

	SetWindowPos(
		hwnd,
		NULL,
		0, 0, 0, 0,
		SWP_FRAMECHANGED |
		SWP_NOZORDER |
		SWP_NOACTIVATE |
		SWP_SHOWWINDOW
	);
}
#endif

void fe::SDLWindow::Show() {
	SDL_ShowWindow(impl->window);
}

void fe::SDLWindow::Hide() {
	SDL_HideWindow(impl->window);
}

void fe::SDLWindow::Move(int x, int y) {
	SDL_SetWindowPosition(impl->window, x, y);
}


void fe::SDLWindow::SetBordered(bool enabled) {
	SDL_SetWindowBordered(impl->window, enabled);
}

void fe::SDLWindow::SetFullscreen(bool enabled) {
	SDL_SetWindowFullscreen(impl->window, enabled);
}

void fe::SDLWindow::GoBorderlessFullscreen() {
	int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
	int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
	int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	SetBorderless();

	Move(x, y);
	Resize(w, h);

	return;

	SDL_PropertiesID props = SDL_GetWindowProperties(impl->window);

	HWND hwnd = (HWND)SDL_GetPointerProperty(
		props,
		SDL_PROP_WINDOW_WIN32_HWND_POINTER,
		NULL
	);

	LONG style = GetWindowLong(hwnd, GWL_STYLE);

style &= ~(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

style |= WS_POPUP;

SetWindowLong(hwnd, GWL_STYLE, style);

LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);

SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);

	// int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
	// int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
	// int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	// int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	SetWindowPos(
    	hwnd,
		HWND_TOPMOST,
		x, y, w, h,
		SWP_NOZORDER |
		SWP_NOACTIVATE |
		SWP_FRAMECHANGED |
		SWP_SHOWWINDOW
	);
}

void fe::SDLWindow::GetMousePosition(double *x, double *y) {
	SDL_GetMouseState((float*)x, (float*)y);
}

double fe::SDLWindow::GetTime() {
	return SDL_GetTicks() * 0.001f;
}

void fe::SDLWindow::GetFramebufferSize(int *width, int* height) {
	SDL_GetWindowSize(impl->window, width, height);
}

bool fe::SDLWindow::HideMouse() {
	return SDL_HideCursor();
}