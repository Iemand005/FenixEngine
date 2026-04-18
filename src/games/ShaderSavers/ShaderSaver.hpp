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
    window->EnableVSync();

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

    uniform sampler2D prevFrame;
    uniform vec2 resolution;

    void main() {
      float x = gl_FragCoord.x, y = gl_FragCoord.y;
      vec2 uv = gl_FragCoord.xy / resolution.xy;
      vec3 lastColor = texture(prevFrame, uv).rgb;
      if (x <= 100 && y <= 100) {
        FragColor = vec4(1.0 - lastColor, 1.0);
      } else {
        FragColor = vec4(lastColor, 1.0);
      }
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

    GLint resLoc = glGetUniformLocation(shader->GetFragmentShader()->id, "resolution");
    glUniform2f(resLoc, (float)w, (float)h);

    GLuint fbos[2], textures[2];
    glGenFramebuffers(2, fbos);
    glGenTextures(2, textures);

    for(int i=0; i<2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbos[i]);
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[i], 0);
    }

    int frameCount = 0;

    while (!window->ShouldClose()) {
      ProcessInput();

      SDL_GetWindowSize(window->GetSDLWindow(), &w, &h);
      glViewport(0, 0, w, h);
      
      glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      shader->Use();
      glBindVertexArray(vao);

      int source = frameCount % 2;
      int dest = (frameCount + 1) % 2;

      glBindFramebuffer(GL_FRAMEBUFFER, fbos[dest]);
      glBindTexture(GL_TEXTURE_2D, textures[source]);

      glDrawArrays(GL_TRIANGLES, 0, 3);

      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); 
      glBindFramebuffer(GL_READ_FRAMEBUFFER, fbos[dest]);

      glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);

      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      window->SwapBuffers();

      frameCount++;
    }

    Destroy();
  }
};
