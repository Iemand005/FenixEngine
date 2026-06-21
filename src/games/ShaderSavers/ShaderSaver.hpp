#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#if defined(FE_USE_SDL)
#define FE_EXCLUDE_GLFW
#else
#define FE_EXCLUDE_SDL
#endif

#include "../../engine/Renderer.hpp"
#if defined(FE_USE_SDL)
#include <SDL3/SDL.h>
#else
#include <GLFW/glfw3.h>
#endif
#include <windows.h>

#include <filesystem>

enum class ScreenSaverMode { Window, Preview, Fullscreen, Config };

class ShaderSaver : public fe::Renderer {
   public:
	int width = 0, height = 0;
	double startX = 0, startY = 0;
	bool fullscreened = false;

	GLint uTime;
	GLint uRes;

	std::filesystem::file_time_type lastWrite;

	ShaderSaver(bool fullscreen = false) : ShaderSaver(500, 500, fullscreen) {}
	ShaderSaver(int w, int h, bool fullscreen = false) : fe::Renderer(w, h, false, true, fullscreen) { fullscreened = fullscreen; }

	void Resize(int w, int h) {
		if (w <= 0 || h <= 0) return;
		width = w;
		height = h;
		glViewport(0, 0, w, h);

		if (uRes >= 0) glUniform2f(uRes, width, height);
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
					SDL_GetWindowSize(window->GetWindow(), &w, &h);
					Resize(w, h);
					window->resizeEvent(w, h);
					break;
				}
			}
		}

		if (fullscreened) {
			float x, y;
			SDL_GetMouseState(&x, &y);

			if (abs(x - startX) > 3 || abs(y - startY) > 3) window->PrepareClose();
		}
#else
		auto window = GetWindow<fe::GLFW3Window>();
		glfwPollEvents();

		if (fullscreened) {
			double x, y;
			window->GetMousePosition(&x, &y);

			double dx = x - startX;
			double dy = y - startY;

			if (std::abs(dx) > 3.0 || std::abs(dy) > 3.0) window->PrepareClose();
		}
#endif
	}

	bool Reload(const char* path, const char* vs) {
		try {
			LoadShaders(fe::Shader::Vertex(vs), fe::Shader::Fragment(path));
			shader->Use();

			uTime = glGetUniformLocation(shader->getId(), "time");
			uRes = glGetUniformLocation(shader->getId(), "resolution");
			return true;
		} catch (...) {
			return false;
		}
	}

	static std::filesystem::file_time_type GetFileTime(const char* path) {
		std::error_code ec;
		return std::filesystem::last_write_time(path, ec);
	}

	void Run(ScreenSaverMode mode = ScreenSaverMode::Window, HWND parent = nullptr) {
#if defined(FE_USE_SDL)
		auto window = GetWindow<fe::SDLWindow>();
#else
		auto window = GetWindow<fe::GLFW3Window>();
#endif

		window->SetAnyKeyCallback([&]() {
			window->PrepareClose();
		});

		window->SetFramebufferResizeCallback([&](int width, int height) {
			Resize(width, height);
		});

		if (mode == ScreenSaverMode::Fullscreen) {
			fullscreened = true;

			window->HideMouse();
			window->GetMousePosition(&startX, &startY);
		}
		if (mode == ScreenSaverMode::Preview)
			window->AttachToNativeParent(parent);
		else
			window->Show();

		const char* vs = R"(
			#version 330 core
			void main() {
				vec2 v[3]=vec2[3](vec2(-1,-1), vec2(3,-1), vec2(-1,3));
				gl_Position=vec4(v[gl_VertexID],0,1);
			})";

		const char* fs = "FragmentShader.glsl";

		lastWrite = GetFileTime(fs);
		Reload(fs, vs);

		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		

		window->GetFramebufferSize(&width, &height);
		Resize(width, height);


		while (!window->ShouldClose()) {
			ProcessInput();

			if (mode != ScreenSaverMode::Fullscreen) {
				auto now = GetFileTime(fs);
				if (now != lastWrite) {
					lastWrite = now;
					Reload(fs, vs);
				}
			}

			float t = (float)window->GetTime();

			if (uTime >= 0) glUniform1f(uTime, t);


			glBindVertexArray(vao);
			glDrawArrays(GL_TRIANGLES, 0, 3);

			window->SwapBuffers();
		}

		glDeleteVertexArrays(1, &vao);

		Destroy();
	}
};