#pragma once
#include <iostream>
#include <SDL3/SDL.h>

#include "IWindow.hpp"

namespace fe {

  class SDLWindow :public IWindow {

    SDL_Window* window;
    SDL_GLContext gl_context;

    public:
    SDLWindow() {
      if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        // return EXIT_FAILURE;
    }

    // 2. Set OpenGL attributes BEFORE creating window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    
    window = SDL_CreateWindow(
        "SDL2 OpenGL Window",
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

    // 5. Enable VSync (optional but recommended)
    SDL_GL_SetSwapInterval(1);
  }
    void SetSwapInterval(int interval) override {
      SDL_GL_SetSwapInterval(interval);
    }

    void SwapBuffers() override {
      SDL_GL_SwapWindow(window);
    }

    void Destroy() override {
      SDL_GL_DestroyContext(gl_context);
      SDL_DestroyWindow(window);
      SDL_Quit();
    }
  };

}