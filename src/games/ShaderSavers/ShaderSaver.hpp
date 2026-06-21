#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define FE_EXCLUDE_GLFW

#include <cstdio>
#include <iostream>

#include "../../engine/Renderer.hpp"

enum class ScreenSaverMode { Window, Preview, Fullscreen, Config };

class ShaderSaver : public fe::Renderer {
   public:
	int width = 0, height = 0;
	float startX, startY;
	bool fullscreened = false;

	ShaderSaver() : ShaderSaver(500, 500) {}
	ShaderSaver(int w, int h) : fe::Renderer(w, h, false, true) {}

	void Resize(int w, int h) {
		if (w <= 0 || h <= 0) return;
		width = w;
		height = h;
		glViewport(0, 0, w, h);
	}

	void ProcessInput() {
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
	}

	bool Reload(const char* path, const char* vs, SDL_Time* out = nullptr) {
		SDL_PathInfo i;
		if (!SDL_GetPathInfo(path, &i)) return false;

		try {
			LoadShaders(fe::Shader::Vertex(vs), fe::Shader::Fragment(path));
			shader->Use();
		} catch (...) {
			return false;
		}

		if (out) *out = i.modify_time;
		return true;
	}

	void Run(ScreenSaverMode mode = ScreenSaverMode::Window, HWND parent = nullptr) {
		auto window = GetWindow<fe::SDLWindow>();
		window->EnableVSync();

		if (mode == ScreenSaverMode::Preview) {
			RECT r;
			GetClientRect(parent, &r);
			window->AttachToNativeParent(parent);
			window->Resize(r.right - r.left, r.bottom - r.top);
		} else if (mode == ScreenSaverMode::Fullscreen) {
			window->GoBorderlessFullscreen();
			window->Show();
			SDL_HideCursor();
			fullscreened = true;
			SDL_GetMouseState(&startX, &startY);
		} else
			window->Show();

		const char* vs = R"(
        #version 330 core
        void main(){
            vec2 v[3]=vec2[3](vec2(-1,-1),vec2(3,-1),vec2(-1,3));
            gl_Position=vec4(v[gl_VertexID],0,1);
        })";

		const char* fs = "E:\\FenixEngine\\src\\games\\ShaderSavers\\FragmentShader.glsl";

		SDL_Time last = 0;
		Reload(fs, vs, &last);

		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		GLint uTime = glGetUniformLocation(shader->getId(), "time");
		GLint uRes = glGetUniformLocation(shader->getId(), "resolution");

		SDL_GetWindowSize(window->GetSDLWindow(), &width, &height);
		glViewport(0, 0, width, height);
		SDL_GetMouseState(&startX, &startY);

		while (!window->ShouldClose()) {
			ProcessInput();

			if (fullscreened)
			{
				float x, y;
				SDL_GetMouseState(&x, &y);

				if (abs(x - startX) > 3 || abs(y - startY) > 3)
					window->PrepareClose();
			}

			SDL_PathInfo i;
			if (SDL_GetPathInfo(fs, &i) && i.modify_time > last) Reload(fs, vs, &last);

			SDL_GetWindowSize(window->GetSDLWindow(), &width, &height);
			glViewport(0, 0, width, height);

			shader->Use();

			float t = SDL_GetTicks() * 0.001f;

			if (uTime >= 0) glUniform1f(uTime, t);
			if (uRes >= 0) glUniform2f(uRes, width, height);

			glBindVertexArray(vao);
			glDrawArrays(GL_TRIANGLES, 0, 3);

			window->SwapBuffers();
		}

		Destroy();
	}
};