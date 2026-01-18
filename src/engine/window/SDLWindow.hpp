#pragma once
#include <iostream>
#include <SDL3/SDL.h>

#include <glad/glad.h>


#include "IWindow.hpp"

namespace fe {

  class SDLWindow :public IWindow {

    SDL_Window* window;
    SDL_GLContext gl_context;

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

    // 5. Enable VSync (optional but recommended)
    // SDL_GL_SetSwapInterval(1);
    // Enable

    // glClearColor(0.2f, 0.3f, 0.4f, 1.0f);  // Background color
    // glViewport(0, 0, 800, 600);
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

    void Destroy() override {
      SDL_GL_DestroyContext(gl_context);
      SDL_DestroyWindow(window);
      SDL_Quit();
    }
  };

}