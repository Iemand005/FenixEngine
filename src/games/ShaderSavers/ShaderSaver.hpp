#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define FE_EXCLUDE_SDL

#include "../../engine/Renderer.hpp"
#include <GLFW/glfw3.h>
#include <windows.h>
#include <chrono>

enum class ScreenSaverMode { Window, Preview, Fullscreen, Config };

class ShaderSaver : public fe::Renderer {
public:
	int width = 0, height = 0;
	double startX = 0, startY = 0;
	bool fullscreened = false;

	GLFWwindow* glfwWindow = nullptr;
	HWND hwnd = nullptr;

	ShaderSaver() : ShaderSaver(500, 500) {}
	ShaderSaver(int w, int h) : fe::Renderer(w, h, false, true) {}

	void Resize(int w, int h) {
		if (w <= 0 || h <= 0) return;
		width = w;
		height = h;
		glViewport(0, 0, w, h);
	}

	// ---------------- INPUT ----------------
	void ProcessInput() {
		glfwPollEvents();

		if (glfwWindowShouldClose(glfwWindow)) {
			auto window = GetWindow<fe::GLFW3Window>();
			window->PrepareClose();
			return;
		}

		// fullscreen mouse exit logic
		if (fullscreened) {
			double x, y;
			glfwGetCursorPos(glfwWindow, &x, &y);

			if (std::abs(x - startX) > 3 || std::abs(y - startY) > 3) {
				auto window = GetWindow<fe::SDLWindow>();
				window->PrepareClose();
			}
		}
	}

	// ---------------- FILE RELOAD ----------------
	bool Reload(const char* path, const char* vs, SDL_Time* out = nullptr) {
		FILETIME ft;
		WIN32_FILE_ATTRIBUTE_DATA data;

		if (!GetFileAttributesExA(path, GetFileExInfoStandard, &data))
			return false;

		try {
			LoadShaders(fe::Shader::Vertex(vs), fe::Shader::Fragment(path));
			shader->Use();
		}
		catch (...) {
			return false;
		}

		if (out) {
			ULARGE_INTEGER ull;
			ull.LowPart = data.ftLastWriteTime.dwLowDateTime;
			ull.HighPart = data.ftLastWriteTime.dwHighDateTime;
			*out = ull.QuadPart;
		}

		return true;
	}

	// ---------------- RUN ----------------
	void Run(ScreenSaverMode mode = ScreenSaverMode::Window, HWND parent = nullptr) {
		glfwInit();

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		auto window = GetWindow<fe::SDLWindow>();

		if (mode == ScreenSaverMode::Fullscreen) {
			const GLFWvidmode* modeVid = glfwGetVideoMode(glfwGetPrimaryMonitor());

			glfwWindow = glfwCreateWindow(
				modeVid->width,
				modeVid->height,
				"ShaderSaver",
				glfwGetPrimaryMonitor(),
				nullptr
			);

			fullscreened = true;
		}
		else if (mode == ScreenSaverMode::Preview) {
			RECT r;
			GetClientRect(parent, &r);

			glfwWindow = glfwCreateWindow(
				r.right - r.left,
				r.bottom - r.top,
				"ShaderSaver",
				nullptr,
				nullptr
			);

			glfwSetWindowPos(glfwWindow, r.left, r.top);
			hwnd = parent;
		}
		else {
			glfwWindow = glfwCreateWindow(500, 500, "ShaderSaver", nullptr, nullptr);
		}

		glfwMakeContextCurrent(glfwWindow);
		glfwSwapInterval(1);

		if (mode == ScreenSaverMode::Fullscreen) {
			glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
			glfwGetCursorPos(glfwWindow, &startX, &startY);
		}

		const char* vs = R"(
		#version 330 core
		void main(){
			vec2 v[3]=vec2[3](vec2(-1,-1),vec2(3,-1),vec2(-1,3));
			gl_Position=vec4(v[gl_VertexID],0,1);
		})";

		const char* fs = "E:\\FenixEngine\\src\\games\\ShaderSavers\\FragmentShader.glsl";

		auto last = std::filesystem::last_write_time(fs);

		Reload(fs, vs, nullptr);

		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		GLint uTime = glGetUniformLocation(shader->getId(), "time");
		GLint uRes = glGetUniformLocation(shader->getId(), "resolution");

		glfwGetFramebufferSize(glfwWindow, &width, &height);
		glViewport(0, 0, width, height);

		while (!glfwWindowShouldClose(glfwWindow)) {
			ProcessInput();

			// hot reload
			auto now = std::filesystem::last_write_time(fs);
			if (now != last) {
				last = now;
				Reload(fs, vs, nullptr);
			}

			glfwGetFramebufferSize(glfwWindow, &width, &height);
			glViewport(0, 0, width, height);

			shader->Use();

			float t = (float)glfwGetTime();

			if (uTime >= 0) glUniform1f(uTime, t);
			if (uRes >= 0) glUniform2f(uRes, width, height);

			glBindVertexArray(vao);
			glDrawArrays(GL_TRIANGLES, 0, 3);

			glfwSwapBuffers(glfwWindow);
		}

		glDeleteVertexArrays(1, &vao);
		glfwDestroyWindow(glfwWindow);
		glfwTerminate();

		Destroy();
	}
};