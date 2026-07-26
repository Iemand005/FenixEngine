
#include "IWindow.hpp"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>

#endif

#include <glad/glad.h>

#include "GLFW3Window.hpp"

using namespace fe;

namespace fe {

struct GLFW3Window::Impl {
    GLFWwindow* window = nullptr;
};

}
fe::GLFW3Window::GLFW3Window(std::string title, int width, int height, bool hidden, bool fullscreen, WindowOptions options, bool useVulkan) : IWindow(width, height), title(title) {
	impl = std::make_unique<Impl>();
	InitGlfw(title, fullscreen);
}

bool fe::GLFW3Window::InitGlfw(std::string title, bool fullscreen, bool tenBit) {
#ifdef WAYLAND
	if (glfwPlatformSupported(GLFW_PLATFORM_WAYLAND)) {
		glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
	} else {
		std::cerr << "No Wayland Support" << std::endl;
	}
#endif

	if (IsWayland()) {
		printf("Setting GLFW platform hint to Wayland\n");
		glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
	} else {
		printf("Setting GLFW platform hint to X11\n");
		glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
	}

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	if (tenBit) {
		glfwWindowHint(GLFW_RED_BITS, 10);
		glfwWindowHint(GLFW_GREEN_BITS, 10);
		glfwWindowHint(GLFW_BLUE_BITS, 10);
		glfwWindowHint(GLFW_ALPHA_BITS, 2);
	}

	GLFWmonitor* monitor = fullscreen ? glfwGetPrimaryMonitor() : NULL;

	if (fullscreen && monitor) {
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		width = mode->width;
		height = mode->height;
	}

	impl->window = glfwCreateWindow(width, height, title.c_str(), monitor, NULL);
	if (impl->window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(impl->window);

	EnableVSync();

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
	  std::cout << "Failed to initialize GLAD" << std::endl;
	  return false;
	}

	glfwSetWindowUserPointer(impl->window, this);


	glfwSetCursorPosCallback(impl->window, [](GLFWwindow* window, double xPos, double yPos) {
		if (!(glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)) {
			return;
		}
	});

	glfwSetKeyCallback(impl->window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto self = static_cast<GLFW3Window*>(glfwGetWindowUserPointer(window));

        if (action == GLFW_PRESS && self->onAnyKey)
            self->onAnyKey();
    });

	glfwSetFramebufferSizeCallback(impl->window, [](GLFWwindow* window, int width, int height) {
		auto self = static_cast<GLFW3Window*>(glfwGetWindowUserPointer(window));

        if (self->onFramebufferResize)
            self->onFramebufferResize(width, height);
	});
	glfwSetWindowSizeCallback(impl->window, [](GLFWwindow* window, int width, int height) {
		auto self = static_cast<GLFW3Window*>(glfwGetWindowUserPointer(window));

        if (self->onFramebufferResize)
            self->onFramebufferResize(width, height);
	});

	// glfwGetWindowAttrib(window, GLFW_TOUCH);
	return true;
}


void *fe::GLFW3Window::GetWindow() { return impl->window; }

void fe::GLFW3Window::StartMouseCapture() {
	glfwSetInputMode(impl->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void fe::GLFW3Window::StopMouseCapture() {
	glfwSetInputMode(impl->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void fe::GLFW3Window::SwapBuffers() const { glfwSwapBuffers(impl->window); }

void fe::GLFW3Window::Destroy() {
	glfwDestroyWindow(impl->window);
	glfwTerminate();
}

fe::GLFW3Window::~GLFW3Window() {
  Destroy();
}

void fe::GLFW3Window::GetMousePosition(double *x, double *y) {
	glfwGetCursorPos(impl->window, x, y);
}

bool fe::GLFW3Window::ShouldClose() {
	return glfwWindowShouldClose(impl->window);
}

double fe::GLFW3Window::GetTime() {
	return glfwGetTime();
}

WindowSize fe::GLFW3Window::GetFramebufferSize() {
	WindowSize size;
	glfwGetFramebufferSize(impl->window, &size.width, &size.height);
	return size;
}

void fe::GLFW3Window::GetFramebufferSize(int *width, int *height) {
	glfwGetFramebufferSize(impl->window, width, height);
}

bool fe::GLFW3Window::HideMouse() {
	glfwSetInputMode(impl->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	return true;
}

void fe::GLFW3Window::Show() {
	// TODO: impl
}

void fe::GLFW3Window::Hide() {
	
}

void fe::GLFW3Window::SetTitle(const char *title) {
	glfwSetWindowTitle(impl->window, title);
}

void fe::GLFW3Window::SetFullscreen(bool enabled) {
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	glfwSetWindowMonitor(impl->window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
}

void fe::GLFW3Window::GoBorderlessFullscreen() {

}

void GLFW3Window::MakeCurrentGLContext() {
	// SDL_GL_MakeCurrent(GetWindow(), GetSDLGLContext());
	// glfwGL
}

fe::VulkanExtensions fe::GLFW3Window::GetVulkanExtensions() {
	VulkanExtensions exts;
	uint32_t glfwExtensionCount = 0;
	exts.extensions = glfwGetRequiredInstanceExtensions(&exts.extensionCount);
	return exts;
}

void *fe::GLFW3Window::CreateVulkanSurface(void *instance) {
	// if (glfwCreateWindowSurface(instance, impl->window, nullptr, &surface_) != VK_SUCCESS) {
	// 	throw std::runtime_error("Failed to create window surface.");
	// }
	return nullptr;
}

void fe::GLFW3Window::PrepareClose() {
    glfwSetWindowShouldClose(impl->window, true);
}

void fe::GLFW3Window::AttachToNativeParent(void *parent) {
#ifdef _WIN32
#include <Windows.h>
    HWND parentHwnd = (HWND)parent;

    HWND childHwnd = glfwGetWin32Window(impl->window);

    LONG style = GetWindowLong(childHwnd, GWL_STYLE);
    style &= ~(WS_POPUP | WS_OVERLAPPEDWINDOW);
    style |= WS_CHILD;

    SetWindowLong(childHwnd, GWL_STYLE, style);

    SetParent(childHwnd, parentHwnd);

    RECT rect;
    GetClientRect(parentHwnd, &rect);

    SetWindowPos(
        childHwnd,
        NULL,
        0, 0,
        rect.right - rect.left,
        rect.bottom - rect.top,
        SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED
    );

    ShowWindow(childHwnd, SW_SHOW);
#endif
}

#ifdef _WIN32
HWND fe::GLFW3Window::GetNativeWindow() {
	return glfwGetWin32Window(impl->window);
}

HDC fe::GLFW3Window::GetDrawingContext() {
	return GetDC(GetNativeWindow());
}

HGLRC fe::GLFW3Window::GetOpenGLRenderingContext() {
	return glfwGetWGLContext(impl->window);
}
#endif
