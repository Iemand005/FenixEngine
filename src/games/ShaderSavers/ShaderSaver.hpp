#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define FE_EXCLUDE_SDL

#include "../../engine/Renderer.hpp"
#if defined(FE_USE_GLFW)
    #include <GLFW/glfw3.h>
#elif defined(FE_USE_SDL)
    #include <SDL.h>
#endif
#include <windows.h>
#include <filesystem>

#if defined(FE_USE_SDL)
using NativeWindow = SDL_Window;
#else
using NativeWindow = GLFWwindow;
#endif

enum class ScreenSaverMode { Window, Preview, Fullscreen, Config };

class ShaderSaver : public fe::Renderer {
public:
	int width = 0, height = 0;
	double startX = 0, startY = 0;
	bool fullscreened = false;

	NativeWindow* nativeWindow = nullptr;

	std::filesystem::file_time_type lastWrite;

	ShaderSaver() : ShaderSaver(2560, 1600) {}
	ShaderSaver(int w, int h) : fe::Renderer(w, h, false, true) {}

	void Resize(int w, int h) {
		if (w <= 0 || h <= 0) return;
		width = w;
		height = h;
		glViewport(0, 0, w, h);
	}

	void ProcessInput() {

#ifdef FE_USE_SDL
		SDL_Event event;
		fe::SDLWindow* window = (fe::SDLWindow*)this->window.get();
		while (window->PollSDLEvents(&event)) {
			switch (event.type) {
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
				case SDL_EVENT_KEY_DOWN:
					if (!fullscreened) break;
				case SDL_EVENT_QUIT:
					window->PrepareClose();
					break;
				case SDL_EVENT_WINDOW_RESIZED:
				case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
					int w, h;
					SDL_GetWindowSize(window->GetSDLWindow(), &w, &h);
					Resize(w, h);
					window->resizeEvent(w, h);
					break;
				}
			}
		}

		if (fullscreened) {
			float x, y;
			SDL_GetMouseState(&x, &y);

			if (abs(x - startX) > 3 || abs(y - startY) > 3)
				window->PrepareClose();
		}
#else
		glfwPollEvents();

		if (glfwWindowShouldClose(nativeWindow)) {
			window->PrepareClose();
			return;
		}

		if (fullscreened) {
			double x, y;
			glfwGetCursorPos(glfwWindow, &x, &y);

			if (std::abs(x - startX) > 3 || std::abs(y - startY) > 3) {
				window->PrepareClose();
			}
		}
#endif
	}

	bool Reload(const char* path, const char* vs) {
		try {
			LoadShaders(
				fe::Shader::Vertex(vs),
				fe::Shader::Fragment(path)
			);
			shader->Use();
			return true;
		}
		catch (...) {
			return false;
		}
	}

	static std::filesystem::file_time_type GetFileTime(const char* path) {
		std::error_code ec;
		return std::filesystem::last_write_time(path, ec);
	}

	void Run(ScreenSaverMode mode = ScreenSaverMode::Window, HWND parent = nullptr) {
		auto glfwWindowWrapper = GetWindow<fe::GLFW3Window>();
		glfwWindow = (GLFWwindow*)glfwWindowWrapper->GetGLFWWindow();

		if (mode == ScreenSaverMode::Fullscreen) {
			glfwSetWindowMonitor(
				glfwWindow,
				glfwGetPrimaryMonitor(),
				0,
				0,
				width,
				height,
				GLFW_DONT_CARE);

			// window->GoBorderlessFullscreen();
			// window->Show();

			fullscreened = true;
			
			glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
			glfwGetCursorPos(glfwWindow, &startX, &startY);
		}

		const char* vs = R"(
			#version 330 core
			void main() {
				vec2 v[3]=vec2[3](
					vec2(-1,-1),
					vec2(3,-1),
					vec2(-1,3)
				);
				gl_Position=vec4(v[gl_VertexID],0,1);
			})";

		const char* fs =
			"E:\\FenixEngine\\src\\games\\ShaderSavers\\FragmentShader.glsl";

		lastWrite = GetFileTime(fs);
		Reload(fs, vs);

		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		GLint uTime = glGetUniformLocation(shader->getId(), "time");
		GLint uRes = glGetUniformLocation(shader->getId(), "resolution");

		glfwGetFramebufferSize(glfwWindow, &width, &height);
		glViewport(0, 0, width, height);

		while (!window->ShouldClose()) {
			ProcessInput();

			auto now = GetFileTime(fs);
			if (now != lastWrite) {
				lastWrite = now;
				Reload(fs, vs);
			}

			glfwGetFramebufferSize(glfwWindow, &width, &height);
			glViewport(0, 0, width, height);

			shader->Use();

			float t = (float)glfwGetTime();

			if (uTime >= 0)
				glUniform1f(uTime, t);

			if (uRes >= 0)
				glUniform2f(uRes, width, height);

			glBindVertexArray(vao);
			glDrawArrays(GL_TRIANGLES, 0, 3);

			glfwSwapBuffers(glfwWindow);
		}

		glDeleteVertexArrays(1, &vao);

		Destroy();
	}
};