#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include "../../engine/EditableGame.hpp"

class ShaderSaver : public fe::Renderer {
public:

  std::vector<std::string> messages;

  double lastUpdateTime = 0.0f;

  bool canJump = true;

  int mapIndex = 0;

  ShaderSaver() : ShaderSaver(800, 640) {}

  ShaderSaver(int width, int height) : fe::Renderer(width, height) {
    
  }

  void ProcessInput() {
    SDL_Event event;
    fe::SDLWindow *window = (fe::SDLWindow*)this->window.get();
    while (window->PollSDLEvents(&event)) {
      switch (event.type) {
        case SDL_EVENT_QUIT: window->PrepareClose(); break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:

          break;
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
          int w, h;
          SDL_GetWindowSize(window->GetSDLWindow(), &w, &h);
          glViewport(0, 0, w, h);
          window->resizeEvent(w, h);

          break;
      }
    }

    // if (window->IsKeyDown(SDL_SCANCODE_LSHIFT)) this->player->Move(fe::Direction::Down, camera.get());

    window->StopMouseCapture();
  }


  void Run() {
    auto window = this->GetWindow<fe::SDLWindow>();
    window->DisableVSync();

    const char *vertexShader = R"(
    #version 330 core
    void main() {
    vec2 vertices[3] = vec2[3](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    gl_Position = vec4(vertices[gl_VertexID], 0.0, 1.0);
}
    )";

    const char *fragmentShader = R"(
    #version 330 core
    out vec4 FragColor;
    void main() {
      float x = gl_FragCoord.x;
      float y = gl_FragCoord.y;
      FragColor = vec4(1.0, 0.0 + x / 100, 0.0, 1.0);
    }
    )";


    LoadShaderTexts(vertexShader, fragmentShader);
    shader->Use();
  
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    int w, h;
    SDL_GetWindowSize(window->GetSDLWindow(), &w, &h);
    glViewport(0, 0, w, h);

    while (!window->ShouldClose()) {
      ProcessInput();

      SDL_GetWindowSize(window->GetSDLWindow(), &w, &h);
      glViewport(0, 0, w, h);
      
      glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      shader->Use();
      glBindVertexArray(vao);

      glDrawArrays(GL_TRIANGLES, 0, 3);

      window->SwapBuffers();
    }

    Destroy();
  }
};
