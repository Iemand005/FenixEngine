#include <SDL3/SDL.h>
#include <glad/glad.h>

#include "SDLWindow.hpp"


inline void CheckError(bool success = false) {
  if (!success) {
    auto error = SDL_GetError();
    std::cerr << "SDL Error: " << error;
  }
}

struct fe::SDLWindow::Impl {
  SDL_Window* window;
  SDL_GLContext gl_context;

  void SDL_FlushOnResizeAndMove(SDL_Window* window) {
    HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    if (hwnd) {
      WNDPROC ogProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)CustomWndProc);
      SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)(ogProc));
    }
  }

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
        if (window->mouseMoveEvent && window->capturingMouse) window->mouseMoveEvent(event->motion.xrel, event->motion.yrel);
        if (window->capturingMouse) {
          SDL_WarpMouseInWindow(window->impl->window, window->width/2.0f, window->height/2.0f);
        }
        break;
    }
    return false;
  }

  
}; // Impl


fe::SDLWindow::SDLWindow(std::string title, int width, int height) : IWindow(width, height) {
    CheckError(SDL_Init(SDL_INIT_VIDEO));

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

    impl->window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!impl->window) {
      CheckError();
      SDL_Quit();
    }

    // SDL_FlushOnResizeAndMove(window);

    impl->gl_context = SDL_GL_CreateContext(impl->window);
    if (!impl->gl_context) {
      CheckError();
      SDL_DestroyWindow(impl->window);
      SDL_Quit();
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
      std::cout << "Failed to initialize GLAD" << std::endl;
      return;
    }

    keyboardState = SDL_GetKeyboardState(NULL);

    // SDL_AddEventWatch(EventWatch, this);
  }

void fe::SDLWindow::SwapBuffers() {
  SDL_GL_SwapWindow(impl->window);
}

  void fe::SDLWindow::SetSwapInterval(int interval) {
    SDL_GL_SetSwapInterval(interval);
  }

  void fe::SDLWindow::SetMouseCapture(bool captureMouse = true) {
    SDL_SetWindowMouseGrab(impl->window, captureMouse);
    SDL_SetWindowRelativeMouseMode(impl->window, captureMouse);
    capturingMouse = captureMouse;
  }

  void fe::SDLWindow::StartMouseCapture() {
    SetMouseCapture(true);
    SDL_HideCursor();
  }

  void fe::SDLWindow::StopMouseCapture() {
    SetMouseCapture(false);
    SDL_ShowCursor();

    // io.WantCaptureMouse = true;
  }

  void fe::SDLWindow::GetSize(int* w, int* h) { SDL_GetWindowSize(impl->window, w, h); }


  void fe::SDLWindow::Destroy() {
    SDL_GL_DestroyContext(impl->gl_context);
    SDL_DestroyWindow(impl->window);
    SDL_Quit();
  }