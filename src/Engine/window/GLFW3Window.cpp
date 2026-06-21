
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>

#endif

#include <glad/glad.h>

#include "GLFW3Window.hpp"

namespace fe {

struct GLFW3Window::Impl {
    GLFWwindow* window = nullptr;
};

}
fe::GLFW3Window::GLFW3Window(std::string title, int width, int height, bool hidden, bool fullscreen) : IWindow(width, height), title(title) {
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

	// glfwSwapInterval(vsync ? 1 : 0);  // Enable vsync
	EnableVSync();

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
	  std::cout << "Failed to initialize GLAD" << std::endl;
	  return false;
	}

	glfwSetWindowUserPointer(impl->window, this);

	// glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yOffset) {
	//   auto game = static_cast<Game*>(glfwGetWindowUserPointer(window));
	//   ImGuiIO& io = ImGui::GetIO();
	//   if (io.WantCaptureMouse) return;
	//   game->fov -= (float)yOffset;
	//   if (game->fov < 1.0f) game->fov = 1.0f;
	//   if (game->fov > 45.0f) game->fov = 45.0f;
	// });
	// glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
	//   ImGuiIO& io = ImGui::GetIO();
	//   if (io.WantCaptureMouse) return;
	// });

	glfwSetCursorPosCallback(impl->window, [](GLFWwindow* window, double xPos, double yPos) {
		if (!(glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)) {
			// ImGui::SetNextFrameWantCaptureMouse(false);
			return;
		}

		// auto game = static_cast<Game*>(glfwGetWindowUserPointer(window));
		// ImGuiIO& io = ImGui::GetIO();
		// if (io.WantCaptureMouse) return;

		// float xOffset = xPos - game->lastX;
		// float yOffset = game->lastY - yPos;
		// if (game->lastX == 0 && game->lastY == 0) {
		//   xOffset = 0;
		//   yOffset = 0;
		// }
		// game->lastX = xPos;
		// game->lastY = yPos;

		// const float sensitivity = 0.1f;
		// xOffset *= sensitivity;
		// yOffset *= sensitivity;

		// game->yaw += xOffset;
		// game->pitch += yOffset;

		// if (game->pitch > 89.0f) game->pitch = 89.0f;
		// if (game->pitch < -89.0f) game->pitch = -89.0f;

		// glm::vec3 direction;
		// direction.x = cos(glm::radians(game->yaw)) * cos(glm::radians(game->pitch));
		// direction.y = sin(glm::radians(game->pitch));
		// direction.z = sin(glm::radians(game->yaw)) * cos(glm::radians(game->pitch));
		// game->camera->front = glm::normalize(direction);
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

// void fe::GLFW3Window::GetDisplaySize(int *width, int *height) {
// 	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
// 	*width = mode->width;
// 	*height = mode->height;
// }

void *fe::GLFW3Window::GetWindow() { return impl->window; }

void fe::GLFW3Window::StartMouseCapture() {
	glfwSetInputMode(impl->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	// io.WantCaptureMouse = false;
}

void fe::GLFW3Window::StopMouseCapture() {
	glfwSetInputMode(impl->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	// io.WantCaptureMouse = true;
}

void fe::GLFW3Window::SwapBuffers() { glfwSwapBuffers(impl->window); }

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

void fe::GLFW3Window::GetFramebufferSize(int *width, int* height) {
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

void fe::GLFW3Window::PrepareClose() {
    glfwSetWindowShouldClose(impl->window, true);
}

void fe::GLFW3Window::AttachToNativeParent(void *parent) {
#ifdef _WIN32
#include <Windows.h>
    HWND parentHwnd = (HWND)parent;

    // Get GLFW internal Win32 HWND
    HWND childHwnd = glfwGetWin32Window(impl->window);

    // Change window style to child window
    LONG style = GetWindowLong(childHwnd, GWL_STYLE);
    style &= ~(WS_POPUP | WS_OVERLAPPEDWINDOW);
    style |= WS_CHILD;

    SetWindowLong(childHwnd, GWL_STYLE, style);

    // Set parent
    SetParent(childHwnd, parentHwnd);

    // Resize to fit parent
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