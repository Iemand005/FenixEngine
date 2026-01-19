#pragma once
#include <iostream>
#include <functional>
#include <SDL3/SDL.h>

#include <glad/glad.h>


#include "IWindow.hpp"



namespace fe {

  using ResizeDelegate = std::function<void(int, int)>;

  class SDLWindow :public IWindow {

    SDL_Window* window;
    SDL_GLContext gl_context;
    bool shouldClose = false;

    static bool SDLCALL resize_watch(void* userdata, SDL_Event* event) {
  SDLWindow *window = (SDLWindow*)userdata;
    if (event->type == SDL_EVENT_WINDOW_EXPOSED) {
        // The window is being resized and needs a redraw.
        // You can update the viewport and render here.
        // glViewport(0, 0, 100, 100);
        window->resizeEvent(window->width, window->height);
        
        // SDL_GL_SwapWindow(window);
    }
    // Also watch for the standard resize events to update your stored size.
    if (event->type == SDL_EVENT_WINDOW_RESIZED ||
        event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
        // Update your application's stored window size.
        // current_width = event->window.data1;
        // current_height = event->window.data2;
        window->width = event->window.data1;
        window->height = event->window.data2;
        std::cout << "resized";

    }
    return false; // Return 0 to allow the event to continue.
}

    public:

    ResizeDelegate resizeEvent;

    int width, height;


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

    

// Before your main loop:
// SDL_EventFilter
    SDL_AddEventWatch(resize_watch, (void*)this);
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