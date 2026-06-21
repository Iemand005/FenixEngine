
#include "GLFW3Window.hpp"

struct fe::GLFW3Window::Impl {
	GLFWwindow* window;
};	// Impl

fe::GLFW3Window::GLFW3Window(std::string title, int width, int height, bool hidden) : IWindow(width, height), title(title) {
	impl = std::make_unique<Impl>();
	InitGlfw();
}

bool fe::GLFW3Window::InitGlfw(bool tenBit) {
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

	impl->window = glfwCreateWindow(width, height, "FoxEngine", NULL, NULL);
	if (impl->window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(impl->window);

	// glfwSwapInterval(vsync ? 1 : 0);  // Enable vsync
	EnableVSync();

	// if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
	//   std::cout << "Failed to initialize GLAD" << std::endl;
	//   return false;
	// }

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

	glfwSetFramebufferSizeCallback(impl->window, [](GLFWwindow* window, int width, int height) {
		// auto game = static_cast<Game*>(glfwGetWindowUserPointer(window));
		// // game->width = width;
		// // game->height = height;
		// // game->scene->resize(width, height);

		// // game->updateAspect();
		// game->Resize(width, height);

		// game->Redraw();
	});
	glfwSetWindowSizeCallback(impl->window, [](GLFWwindow* window, int width, int height) {
		// auto game = static_cast<Game*>(glfwGetWindowUserPointer(window));
		// game->Resize(width, height);
		// game->Redraw();
	});

	// glfwGetWindowAttrib(window, GLFW_TOUCH);
	return true;
}

void *fe::GLFW3Window::GetGLFWWindow() { return impl->window; }

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