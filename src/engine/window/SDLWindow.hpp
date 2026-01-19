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
      if (window->resizeEvent)window->resizeEvent(window->width, window->height);
break;
case SDL_EVENT_WINDOW_RESIZED:
case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
window->width = event->window.data1, window->height = event->window.data2;
      break;
    
    case SDL_EVENT_MOUSE_MOTION:
      if (window->mouseMoveEvent) window->mouseMoveEvent(event->motion.xrel, event->motion.yrel)
      ;
      break;
    }
    return false;
    // if (event->type == SDL_EVENT_WINDOW_EXPOSED) {
    //   // The window is being resized and needs a redraw.
    //   // You can update the viewport and render here.
    //   // glViewport(0, 0, 100, 100);

    //   // SDL_GL_SwapWindow(window);
    // }
    // // Also watch for the standard resize events to update your stored size.
    // if (event->type == SDL_EVENT_WINDOW_RESIZED || event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
    //   // Update your application's stored window size.
    //   // current_width = event->window.data1;
    //   // current_height = event->window.data2;
    //   window->width = event->window.data1;
    //   window->height = event->window.data2;
    //   std::cout << "resized";
    // }
    // return false;  // Return 0 to allow the event to continue.
  }

 public:
  ResizeDelegate resizeEvent;
  MouseMoveDelegate mouseMoveEvent;

  int width, height;

  SDLWindow(std::string title, int width, int height) : width(width), height(height) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
      std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
      return;
    }

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
      std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
      SDL_Quit();
    }

    // 4. Create OpenGL context
    gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
      std::cerr << "OpenGL context creation failed: " << SDL_GetError() << std::endl;
      SDL_DestroyWindow(window);
      SDL_Quit();
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
      std::cout << "Failed to initialize GLAD" << std::endl;
      return;
    }

    // Before your main loop:
    SDL_AddEventWatch(EventWatch, this);
  }
  void SetSwapInterval(int interval) override { SDL_GL_SetSwapInterval(interval); }

  void StartMouseCapture() override {
    SDL_SetWindowMouseGrab(window, true);
    SDL_HideCursor();
  }

  void StopMouseCapture() override {
    SDL_SetWindowMouseGrab(window, false);
    SDL_ShowCursor();
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