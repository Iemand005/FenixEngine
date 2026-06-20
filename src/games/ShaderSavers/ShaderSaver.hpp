#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/imgui.h>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include "../../engine/Renderer.hpp"

class ShaderSaver : public fe::Renderer {
 public:
  std::vector<std::string> messages;

  double lastUpdateTime = 0.0f;

  bool stepRequested = false;
  bool STEPPED = false;
  bool spaceWasDown = false;
  bool reloadRequested = false;
  bool rWasDown = false;

  ShaderSaver() : ShaderSaver(500, 500) {}

  ShaderSaver(int width, int height) : fe::Renderer(width, height) {}

  void ProcessInput() {
    SDL_Event event;
    fe::SDLWindow* window = (fe::SDLWindow*)this->window.get();
    while (window->PollSDLEvents(&event)) {
      switch (event.type) {
        case SDL_EVENT_QUIT:
          window->PrepareClose();
          break;
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

    if (window->IsKeyDown(SDL_SCANCODE_SPACE) && !spaceWasDown)
      stepRequested = true, spaceWasDown = true;
    else if (spaceWasDown)
      spaceWasDown = false;

    if (window->IsKeyDown(SDL_SCANCODE_R) && !rWasDown)
      reloadRequested = true, rWasDown = true;
    else if (rWasDown)
      rWasDown = false;

    window->StopMouseCapture();
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

    if (outWriteTime)
      *outWriteTime = currentInfo.modify_time;

    return true;
  }

  void Run(bool previewMode = false, HWND previewParent = nullptr) {
    auto window = this->GetWindow<fe::SDLWindow>();
    window->EnableVSync();

    if (previewMode && previewParent)
    {
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(window->GetSDLWindow(), &wmInfo);

        HWND child = wmInfo.info.win.window;

        SetParent(child, previewParent);

        LONG style = GetWindowLong(child, GWL_STYLE);
        style &= ~(WS_POPUP | WS_OVERLAPPEDWINDOW);
        style |= WS_CHILD;

        SetWindowLong(child, GWL_STYLE, style);

        SetWindowPos(child, NULL, 0, 0, 0, 0,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }

    const char* vertexShaderText = /** GLSL */ R"(
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

    const char* fragShaderPath = "E:\\FenixEngine\\src\\games\\ShaderSavers\\FragmentShader.glsl";
    SDL_Time lastWriteTime = 0;
    if (!ReloadFragmentShader(fragShaderPath, vertexShaderText, &lastWriteTime)) {
      std::cerr << "Initial shader load failed." << std::endl;
    }

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    int w, h;
    SDL_GetWindowSize(window->GetSDLWindow(), &w, &h);
    glViewport(0, 0, w, h);

    shader->Use();
    GLint prevLoc = glGetUniformLocation(shader->getId(), "prevFrame");
    if (prevLoc >= 0) glUniform1i(prevLoc, 0);
    GLint resLoc = glGetUniformLocation(shader->getId(), "resolution");
    if (resLoc >= 0) glUniform2f(resLoc, (float)w, (float)h);

    GLuint fbos[2], textures[2];
    glGenFramebuffers(2, fbos);
    glGenTextures(2, textures);

    for (int i = 0; i < 2; i++) {
      glBindFramebuffer(GL_FRAMEBUFFER, fbos[i]);
      glBindTexture(GL_TEXTURE_2D, textures[i]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[i], 0);
      GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0};
      glDrawBuffers(1, drawBuffers);
      glReadBuffer(GL_COLOR_ATTACHMENT0);
    }

    bool firstDraw = true;
    int frameCount = 0;

    while (!window->ShouldClose()) {
      ProcessInput();
      if (firstDraw) {
        stepRequested = true;
      }

      // SDL_GetPathInfo(fragShaderPath,
      SDL_PathInfo currentInfo;
      if (SDL_GetPathInfo(fragShaderPath, &currentInfo) && currentInfo.modify_time > lastWriteTime) {
        printf("Reloading shader because file changed...\n");
        if (!ReloadFragmentShader(fragShaderPath, vertexShaderText, &lastWriteTime)) {
          std::cout << "Failed to reload shader after file change." << std::endl;
        }
      }

      if (reloadRequested) {
        reloadRequested = false;
        printf("Reloading shader on R press...\n");
        if (!ReloadFragmentShader(fragShaderPath, vertexShaderText, &lastWriteTime)) {
          std::cout << "Failed to reload shader on R press." << std::endl;
        }
      }
      int newW, newH;
      SDL_GetWindowSize(window->GetSDLWindow(), &newW, &newH);

      if (newW != w || newH != h) {
        w = newW;
        h = newH;

        glViewport(0, 0, w, h);

        for (int i = 0; i < 2; i++) {
          glBindTexture(GL_TEXTURE_2D, textures[i]);
          glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        printf("Resized naar: %dx%d\n", w, h);
      }

      // SDL_GetWindowSize(window->GetSDLWindow(), &w, &h);
      // glViewport(0, 0, w, h);
      shader->Use();
      GLint prevLoc = glGetUniformLocation(shader->getId(), "prevFrame");
      if (prevLoc >= 0) glUniform1i(prevLoc, 0);
      GLint resLoc = glGetUniformLocation(shader->getId(), "resolution");
      if (resLoc >= 0) glUniform2f(resLoc, (float)w, (float)h);
      float t = SDL_GetTicks() / 1000.0f;
      glUniform1f(glGetUniformLocation(shader->getId(), "time"), t);

      glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      if (stepRequested || !STEPPED) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[frameCount % 2]);

        glBindVertexArray(vao);

        int source = frameCount % 2;
        int dest = (frameCount + 1) % 2;

        glBindFramebuffer(GL_FRAMEBUFFER, fbos[dest]);
        GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, drawBuffers);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbos[dest]);

        frameCount++;
        firstDraw = false;
        stepRequested = false;
      }

      glBindFramebuffer(GL_READ_FRAMEBUFFER, fbos[frameCount % 2]);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
      glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);

      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      window->SwapBuffers();
    }

    Destroy();
  }
};
