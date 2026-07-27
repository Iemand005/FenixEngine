
#include "IWindow.hpp"
#include "Joystick.hpp"
#ifdef _WIN32
#include <windows.h>
#elif !defined(EMSCRIPTEN)
#include <X11/Xlib.h>

//#include <GL/glx.h>
#endif

#include <functional>
#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_dialog.h>
#include <glad/glad.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/emscripten.h>
#endif

#include "SDLWindow.hpp"

using namespace fe;

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
  SDL_Window* window = nullptr;
  SDL_GLContext gl_context = nullptr;

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

fe::SDLWindow::SDLWindow(std::string title, int width, int height, bool hidden, bool fullscreen, WindowOptions options, bool useVulkan) : IWindow(width, height) {

	std::cout << "Creating window with " << (useVulkan? "Vulkan" : "OpenGL") << std::endl;

	SDL_SetHint(SDL_HINT_JOYSTICK_DIRECTINPUT, "1");
	SDL_SetHint(SDL_HINT_HIDAPI_ENUMERATE_ONLY_CONTROLLERS, "0");
	SDL_SetHint(SDL_HINT_HIDAPI_ENUMERATE_ONLY_CONTROLLERS, "0");
	SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "0");

	impl = std::make_unique<Impl>();
	CheckError(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC));

	SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

	bool forceX11 = true;

	if (!IsWayland() || forceX11)
		SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11");
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		std::cout << "Failed to initialize video driver uhm" << std::endl;
		return;
    }

	auto windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
	if (useVulkan) windowFlags |= SDL_WINDOW_VULKAN;
	else windowFlags |= SDL_WINDOW_OPENGL;
	if (hidden) windowFlags |= SDL_WINDOW_HIDDEN;

	impl->window = SDL_CreateWindow(title.c_str(), width, height, windowFlags);

	if (!impl->window) {
		CheckError();
		SDL_Quit();
	}

	if (!useVulkan) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

		if (options.tenBit) {
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 10);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 10);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 10);
			SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 2);
		}

		impl->gl_context = SDL_GL_CreateContext(impl->window);
		if (!impl->gl_context) {
			CheckError();
			SDL_DestroyWindow(impl->window);
			SDL_Quit();
		}

		static bool gladLoaded = false;
		if (!gladLoaded) {
			if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
				std::cout << "Failed to initialize GLAD" << std::endl;
				return;
			}
			gladLoaded = true;
		}
	}

    keyboardState = SDL_GetKeyboardState(NULL);
}

fe::SDLWindow::SDLWindow(std::string title, int width, int height, bool hidden, bool fullscreen, WindowOptions options, bool useVulkan, SDL_GLContext sharedContext) : IWindow(width, height) {
	impl = std::make_unique<Impl>();
	CheckError(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK));

	SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

	bool forceX11 = true;

	if (!IsWayland() || forceX11)
		SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11");
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		std::cout << "Failed to initialize video driver uhm" << std::endl;
		return;
    }

	auto windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
	if (!useVulkan) windowFlags |= SDL_WINDOW_OPENGL;
	if (hidden) windowFlags |= SDL_WINDOW_HIDDEN;

	impl->window = SDL_CreateWindow(title.c_str(), width, height, windowFlags);

	if (!impl->window) {
		CheckError();
		SDL_Quit();
	}

	if (!useVulkan) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

		if (options.tenBit) {
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 10);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 10);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 10);
			SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 2);
		}

		if (sharedContext) {
			SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
		}

		impl->gl_context = SDL_GL_CreateContext(impl->window);

		if (sharedContext) {
			SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);
		}

		if (!impl->gl_context) {
			CheckError();
			SDL_DestroyWindow(impl->window);
			SDL_Quit();
		}

		static bool gladLoaded = false;
		if (!gladLoaded) {
			if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
				std::cout << "Failed to initialize GLAD" << std::endl;
				return;
			}
			gladLoaded = true;
		}
	}

    keyboardState = SDL_GetKeyboardState(NULL);
}

void fe::SDLWindow::SwapBuffers() const {  // TOOD: this 
	// TODO: return if vulkan or weell do vulkan impl not gl
	// if (true) return;
	SDL_GL_SwapWindow(impl->window);
}

void fe::SDLWindow::SetSwapInterval(int interval) {
	if (false)
	SDL_GL_SetSwapInterval(interval);
	// else SDL_Vulkan_Get
	// else SDL_Vulkan_Set
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

bool fe::SDLWindow::IsCapturingMouse() {
	return capturingMouse;
}

void fe::SDLWindow::GetSize(int* w, int* h) { SDL_GetWindowSize(impl->window, w, h); }


void fe::SDLWindow::OpenFileDialog(FileDialogCallback callback, const char* filterName, const char* filterPattern) {
	#ifdef __EMSCRIPTEN__
		auto* cb = new FileDialogCallback(std::move(callback));
		char script[1024];
		snprintf(script, sizeof(script),
			"console.log('[FileDialog] Opening file dialog...');"
			"var input = document.createElement('input');"
			"input.type = 'file';"
			"input.accept = '%s,%s';"
			"input.onchange = function(e) {"
			"  console.log('[FileDialog] File selected');"
			"  var file = e.target.files[0];"
			"  if (file) {"
			"    console.log('[FileDialog] File:', file.name, file.size);"
			"    var reader = new FileReader();"
			"    reader.onload = function() {"
			"      console.log('[FileDialog] File loaded, writing to FS');"
			"      var bytes = new Uint8Array(reader.result);"
			"      var fs = Module.FS;"
			"      var path = '/tmp/uploaded_' + file.name;"
			"      fs.writeFile(path, bytes, { encoding: 'binary' });"
			"      console.log('[FileDialog] File written to:', path);"
			"      Module.ccall('emscripten_file_dialog_callback', 'void', ['number', 'string'], [%ld, path]);"
			"    };"
			"    reader.readAsArrayBuffer(file);"
			"  }"
			"};"
			"input.click();",
			filterPattern, filterName, reinterpret_cast<intptr_t>(cb));
		emscripten_run_script(script);
	#else
	auto* cb = new FileDialogCallback(std::move(callback));
	SDL_DialogFileFilter filters[] = {{ filterName, filterPattern }};
	SDL_ShowOpenFileDialog(SDLWindow::OnFileDialogResult, cb, impl->window, filters, 1, nullptr, false);
	SDL_Log("SDL Dialog Error status: %s", SDL_GetError());
	#endif
}

void SDLCALL fe::SDLWindow::OnFileDialogResult(void* userdata, const char* const* files, int filter) {
	auto cb = static_cast<FileDialogCallback*>(userdata);
	if (files && files[0]) {
		(*cb)(files[0]);
	}
	delete cb;
}

#ifdef __EMSCRIPTEN__
extern "C" void emscripten_file_dialog_callback(intptr_t userdata, const char* path) {
	char script[512];
	snprintf(script, sizeof(script), "console.log('[FileDialog] C++ callback invoked with path:', UTF8ToString(%p));", path);
	emscripten_run_script(script);
	auto cb = reinterpret_cast<fe::IWindow::FileDialogCallback*>(userdata);
	if (cb && path) {
		(*cb)(path);
	}
	delete cb;
}
#endif

void fe::SDLWindow::Destroy() {
	if (!impl) return;

	if (impl->gl_context) {
		SDL_GL_MakeCurrent(impl->window, nullptr); 
		SDL_GL_DestroyContext(impl->gl_context);
		impl->gl_context = nullptr;
	}

	if (impl->window) {
		SDL_DestroyWindow(impl->window);
		impl->window = nullptr;
	}

	SDL_Quit();
}

SDL_Window* fe::SDLWindow::GetWindow() const { return impl->window; }

SDL_GLContext fe::SDLWindow::GetSDLGLContext() const { return impl->gl_context; }

bool fe::SDLWindow::IsKeyDown(SDL_Scancode key) { return keyboardState[key]; }

bool fe::SDLWindow::PollSDLEvent(SDL_Event* event, bool getKeyboardState) {
    if (getKeyboardState) keyboardState = SDL_GetKeyboardState(NULL);
    if (!SDL_PollEvent(event)) return false;

    switch (event->type) {
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_KEY_DOWN:
			if (!_isScreensaving) break;
		case SDL_EVENT_QUIT:
			PrepareClose();
			break;
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
			if (_isScreensaving) {
				float x, y;
				SDL_GetMouseState(&x, &y);
				if (capturingMouse && abs(x - startX) > 3 || abs(y - startY) > 3)
					PrepareClose();
			} else if (mouseMoveEvent && capturingMouse) {
				mouseMoveEvent(event->motion.xrel, event->motion.yrel);
				SDL_WarpMouseInWindow(impl->window, width/2.0f, height/2.0f);
			}
			break;
    }

    return true;
  }

void fe::SDLWindow::Resize(int w, int h) {
	SDL_SetWindowSize(this->impl->window, w, h);
}

// void fe::SDLWindow::ActivateScreenSaverMode() {
//     SDL_GetMouseState(&startX, &startY);
// 	_isScreensaving = true;
// }


void fe::SDLWindow::AttachToNativeParent(void* parent)
{
	if (!parent)
		return;

	SDL_PropertiesID props = SDL_GetWindowProperties(GetWindow());
	if (!props)
		return;

	unsigned int w, h;

#ifdef _WIN32

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

	RECT r;
	GetClientRect((HWND)parent, &r);

	w = r.right - r.left;
	h = r.bottom - r.top;
#elif !defined(__EMSCRIPTEN__)


	Window sdl_xwindow = (Window)(uintptr_t)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
	Display *display = (Display *)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);

	if (!display || sdl_xwindow == 0) {
		SDL_Log("Failed to get X11 window properties");
		return;
	}

	Window parent_window_id = (Window)(uintptr_t)parent;
	XReparentWindow(display, sdl_xwindow, parent_window_id, 0, 0);

	Window root;
	int x, y;
	unsigned int border_width, depth;
	XGetGeometry(display, parent_window_id, &root, &x, &y, &w, &h, &border_width, &depth);
	XSelectInput(display, sdl_xwindow, StructureNotifyMask | ExposureMask);
	/*int oldWidth, oldHeight;
	resizeEvent = [&](int width, int height) {

		if (width != oldWidth || height != oldHeight) {
			oldWidth = width;
			oldHeight = height;
			Resize(width, height);
		}
	};*/
	XSync(display, False);
#endif
	 Resize(w, h);
}

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
	if (SDL_SetWindowFullscreen(impl->window, enabled))
		isFullscreen = enabled;
}

void fe::SDLWindow::GoBorderlessFullscreen() {
#ifdef _WIN32
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
	#endif
}

void fe::SDLWindow::GetMousePosition(double *x, double *y) {
	SDL_GetMouseState((float*)x, (float*)y);
}

double fe::SDLWindow::GetTime() {
	return SDL_GetTicks() * 0.001f;
}

bool fe::SDLWindow::HideMouse() {
	return SDL_HideCursor();
}

void fe::SDLWindow::SetTitle(const char *title) {
	SDL_SetWindowTitle(impl->window, title);
}

std::vector<Joystick> SDLWindow::GetJoysticks() {
	int count = 0;
    SDL_JoystickID *joystickIds = SDL_GetJoysticks(&count);

	std::vector<Joystick> joysticks;
		
    if (joystickIds) {
		printf("%i joysticks were found.\n\n", count );
		printf("The names of the joysticks are:\n");
        for (int i = 0; i < count; i++) {
            std::string name = SDL_GetJoystickNameForID(joystickIds[i]);
			
			std::cout << "Found joystick: " << name << std::endl;
			joysticks.emplace_back(joystickIds[i]);
        }
        
        SDL_free(joystickIds);
    }
	
	return joysticks;
}

void SDLWindow::UpdateJoysticks() {
	SDL_UpdateJoysticks();
}

fe::VulkanExtensions fe::SDLWindow::GetVulkanExtensions() {
	fe::VulkanExtensions ext;
	ext.extensions = SDL_Vulkan_GetInstanceExtensions(&ext.extensionCount);
	return ext;
}

void *fe::SDLWindow::CreateVulkanSurface(void *instance) {
	VkSurfaceKHR surface;
	if (!SDL_Vulkan_CreateSurface(impl->window, (VkInstance)instance, nullptr, &surface)) {
		throw std::runtime_error("Failed to create window surface.");
	}
	return (void*)surface;
}

fe::WindowSize fe::SDLWindow::GetFramebufferSize() {
	fe::WindowSize m;
	SDL_GetWindowSizeInPixels(impl->window, &m.width, &m.height);
	return m;
}

void SDLWindow::MakeCurrentGLContext() const {
	SDL_GL_MakeCurrent(GetWindow(), GetSDLGLContext());
}

void SDLWindow::UnbindGLContext() const {
	SDL_GL_MakeCurrent(nullptr, nullptr);
}


#ifdef _WIN32
HWND fe::SDLWindow::GetNativeWindow() {
	SDL_PropertiesID props = SDL_GetWindowProperties(impl->window);
	return (HWND)SDL_GetPointerProperty(
	props,
	SDL_PROP_WINDOW_WIN32_HWND_POINTER,
	nullptr
);
}

HDC fe::SDLWindow::GetDrawingContext() {
	return GetDC(GetNativeWindow());
}

HGLRC fe::SDLWindow::GetOpenGLRenderingContext() {
	return (HGLRC)SDL_GL_GetCurrentContext();
}
#else
void* fe::SDLWindow::GetWaylandSurface() {
	SDL_PropertiesID props = SDL_GetWindowProperties(impl->window);
	return SDL_GetPointerProperty(
		props,
		SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER,
		nullptr
	);
}

void *fe::SDLWindow::GetWaylandDisplay() {
	SDL_PropertiesID props = SDL_GetWindowProperties(impl->window);
	return SDL_GetPointerProperty(
		props,
		SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER,
		nullptr
	);
}

void *fe::SDLWindow::GetX11Display() {
	SDL_PropertiesID props = SDL_GetWindowProperties(impl->window);
	return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
}

unsigned long fe::SDLWindow::GetGLXDrawable() {
	SDL_PropertiesID props = SDL_GetWindowProperties(impl->window);
	return SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
}
#endif
