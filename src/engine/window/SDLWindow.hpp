#pragma once
#include <SDL3/SDL.h>
#include <glad/glad.h>

#include <functional>
#include <iostream>

#include "IWindow.hpp"

namespace fe {

using ResizeDelegate = std::function<void(int, int)>;
using MouseMoveDelegate = std::function<void(int, int)>;

class SDLWindow : public IWindow {
  SDL_Window* window;
  SDL_GLContext gl_context;
  bool shouldClose = false;

  static bool SDLCALL EventWatch(void* userdata, SDL_Event* event) {
    SDLWindow* window = (SDLWindow*)userdata;
    switch (event->type) {
      case SDL_EVENT_WINDOW_EXPOSED:
        if (window->resizeEvent) window->resizeEvent(window->width, window->height);
        break;
      case SDL_EVENT_WINDOW_RESIZED:
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        window->width = event->window.data1, window->height = event->window.data2;
        break;

      case SDL_EVENT_MOUSE_MOTION:
        if (window->mouseMoveEvent) window->mouseMoveEvent(event->motion.xrel, event->motion.yrel);
        if (window->capturingMouse) {
          SDL_WarpMouseInWindow(window->window, window->width/2.0f, window->height/2.0f);
        }
        break;
    }
    return false;
  }

  static void CheckError(bool success = false) {
    if (!success) {
      auto error = SDL_GetError();
      std::cerr << "SDL Error: " << error;
    }
  }

 public:
  ResizeDelegate resizeEvent;
  MouseMoveDelegate mouseMoveEvent;

  int width, height;
  bool capturingMouse = false;

  SDLWindow(std::string title, int width, int height) : width(width), height(height) {
    CheckError(SDL_Init(SDL_INIT_VIDEO));

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

    window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!window) {
      CheckError();
      SDL_Quit();
    }

    gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
      CheckError();
      SDL_DestroyWindow(window);
      SDL_Quit();
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
      std::cout << "Failed to initialize GLAD" << std::endl;
      return;
    }

    SDL_AddEventWatch(EventWatch, this);
  }
  void SetSwapInterval(int interval) override { SDL_GL_SetSwapInterval(interval); }

  void StartMouseCapture() override {
    SDL_SetWindowMouseGrab(window, true);
    SDL_SetWindowRelativeMouseMode(window, true);
    SDL_HideCursor();
    capturingMouse = true;
  }

  void StopMouseCapture() override {
    SDL_SetWindowMouseGrab(window, false);
    SDL_SetWindowRelativeMouseMode(window, true);
    SDL_ShowCursor();
    capturingMouse = false;

    // io.WantCaptureMouse = true;
  }

  void SwapBuffers() override { SDL_GL_SwapWindow(window); }

  bool PollSDLEvents(SDL_Event* event) { return SDL_PollEvent(event); }

  SDL_Window* GetSDLWindow() { return window; }

  SDL_GLContext GetSDLGLContext() { return gl_context; }

  void Destroy() override {
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
  }
};

}  // namespace fe