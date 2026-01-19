#pragma once
#include <iostream>
#include <SDL3/SDL.h>

#include <glad/glad.h>


#include "IWindow.hpp"

namespace fe {

  class SDLWindow :public IWindow {

    SDL_Window* window;
    SDL_GLContext gl_context;
    bool shouldClose = false;

    public:
    SDLWindow(std::string title) {
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
    
    window = SDL_CreateWindow(
        title.c_str(),
        800, 600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    
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
  }
    void SetSwapInterval(int interval) override {
      SDL_GL_SetSwapInterval(interval);
    }

    void SwapBuffers() override {
      SDL_GL_SwapWindow(window);
    }

    bool PollSDLEvents(SDL_Event *event) {
      return SDL_PollEvent(event);
    }

    SDL_Window* GetSDLWindow() {
      return window;
    }

    SDL_GLContext GetSDLGLContext() {
      return gl_context;
    }

    // bool ShouldClose() override {
    //   return shouldClose;
    // }

    void Destroy() override {
      SDL_GL_DestroyContext(gl_context);
      SDL_DestroyWindow(window);
      SDL_Quit();
    }
  };

}