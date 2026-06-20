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
	bool stepRequested = false;
	bool stepped = false;
	bool spaceWasDown = false;
	bool reloadRequested = false;
	bool rWasDown = false;
	bool fullscreened = false;

	int width = 0, height = 0;
	GLuint textures[2] = {0, 0};

	float startX, startY;

	ShaderSaver() : ShaderSaver(500, 500) {}

	ShaderSaver(int width, int height) : fe::Renderer(width, height, false, true) {}

	void Resize(int w, int h) {
		if (w <= 0 || h <= 0) return;

		width = w;
		height = h;
		glViewport(0, 0, w, h);

		if (!textures[0]) return;

		for (int i = 0; i < 2; i++) {
			glBindTexture(GL_TEXTURE_2D, textures[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		}
		printf("Resized naar: %dx%d\n", w, h);
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

		if (window->IsKeyDown(SDL_SCANCODE_SPACE) && !spaceWasDown)
			stepRequested = true, spaceWasDown = true;
		else if (spaceWasDown)
			spaceWasDown = false;

		if (window->IsKeyDown(SDL_SCANCODE_R) && !rWasDown)
			reloadRequested = true, rWasDown = true;
		else if (rWasDown)
			rWasDown = false;

		if (fullscreened) {
			float x, y;
			SDL_GetMouseState(&x, &y);

			if (abs(x - startX) > 3 || abs(y - startY) > 3)
			{
				window->PrepareClose();
			}
		}
	}

	bool ReloadFragmentShader(const char* fragShaderPath, const char* vertexShaderText, SDL_Time* outWriteTime = nullptr) {
		SDL_PathInfo currentInfo;
		if (!SDL_GetPathInfo(fragShaderPath, &currentInfo)) {
			std::cerr << "Failed to query shader file info: " << fragShaderPath << std::endl;
			return false;
		}

		try {
			LoadShaders(fe::Shader::Vertex(vertexShaderText), fe::Shader::Fragment(fragShaderPath));
			shader->Use();
		} catch (const std::exception& ex) {
			std::cout << ex.what() << std::endl;
			return false;
		}

		if (outWriteTime) *outWriteTime = currentInfo.modify_time;

		return true;
	}

	void Run(ScreenSaverMode mode = ScreenSaverMode::Window, HWND previewParent = nullptr)
{
    auto window = GetWindow<fe::SDLWindow>();
    window->EnableVSync();

    // -------------------------
    // Window setup
    // -------------------------
    if (mode == ScreenSaverMode::Preview)
    {
        RECT r;
        GetClientRect(previewParent, &r);

        window->AttachToNativeParent(previewParent);
        window->Resize(r.right - r.left, r.bottom - r.top);
    }
    else if (mode == ScreenSaverMode::Fullscreen)
    {
        window->GoBorderlessFullscreen();
        window->Show();

        SDL_HideCursor();
        SDL_SetCursor(nullptr);

        fullscreened = true;
    }
    else
    {
        window->Show();
    }

    // -------------------------
    // Shader setup
    // -------------------------
    const char* vertexShaderText = R"(
    #version 330 core
    void main() {
        vec2 v[3] = vec2[3](
            vec2(-1.0, -1.0),
            vec2( 3.0, -1.0),
            vec2(-1.0,  3.0)
        );
        gl_Position = vec4(v[gl_VertexID], 0.0, 1.0);
    })";

    const char* fragShaderPath =
        "E:\\FenixEngine\\src\\games\\ShaderSavers\\FragmentShader.glsl";

    SDL_Time lastWriteTime = 0;

    if (!ReloadFragmentShader(fragShaderPath, vertexShaderText, &lastWriteTime))
        std::cerr << "Initial shader load failed.\n";

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // cache uniforms ONCE (important)
    GLint uTime = glGetUniformLocation(shader->getId(), "time");
    GLint uRes  = glGetUniformLocation(shader->getId(), "resolution");

    // -------------------------
    // initial size
    // -------------------------
    SDL_GetWindowSize(window->GetSDLWindow(), &width, &height);
    glViewport(0, 0, width, height);

    SDL_GetMouseState(&startX, &startY);

    // -------------------------
    // MAIN LOOP (minimal)
    // -------------------------
    while (!window->ShouldClose())
    {
        ProcessInput();

        // shader hot reload
        SDL_PathInfo info;
        if (SDL_GetPathInfo(fragShaderPath, &info) &&
            info.modify_time > lastWriteTime)
        {
            ReloadFragmentShader(fragShaderPath, vertexShaderText, &lastWriteTime);
        }

        if (reloadRequested)
        {
            reloadRequested = false;
            ReloadFragmentShader(fragShaderPath, vertexShaderText, &lastWriteTime);
        }

        // resize (cheap check could be added, but kept minimal)
        SDL_GetWindowSize(window->GetSDLWindow(), &width, &height);
        glViewport(0, 0, width, height);

        // uniforms
        shader->Use();

        float t = SDL_GetTicks() * 0.001f;

        if (uTime >= 0) glUniform1f(uTime, t);
        if (uRes  >= 0) glUniform2f(uRes, (float)width, (float)height);

        // render fullscreen triangle
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        window->SwapBuffers();
    }

    Destroy();
}
};
