#pragma once

#include <functional>
#include <stdlib.h>
#include <string.h>

namespace fe {


  // Returns 1 if Wayland is active, 0 if X11 (or fallback)
  int is_running_wayland() {
    // 1. Check the primary desktop session type
    const char* session = getenv("XDG_SESSION_TYPE");
    if (session && strcmp(session, "wayland") == 0) {
      return 1;
    }

    // 2. Fallback check for the specific Wayland socket
    const char* wayland_display = getenv("WAYLAND_DISPLAY");
    if (wayland_display != NULL) {
      return 1;
    }

    return 0; // Assume X11 or XWayland context
  }


  using ResizeDelegate = std::function<void(int, int)>;
  using MouseMoveDelegate = std::function<void(int, int)>;

  class IWindow {

    bool shouldClose = false;

public:

  int width, height;

  ResizeDelegate resizeEvent;
  MouseMoveDelegate mouseMoveEvent;

  IWindow(int width, int height) : width(width), height(height) {}

  virtual bool ShouldClose() { return shouldClose; }

  virtual void PrepareClose() { shouldClose = true; }

	virtual void SetSwapInterval(int interval) = 0;

  void EnableVSync() {
    SetSwapInterval(1);
  }

  void DisableVSync() {
    SetSwapInterval(0);
  }

  virtual void StartMouseCapture() {
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }

  virtual void StopMouseCapture() {
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

  }

  virtual void SetTitle(const char *newTitle) {};

  bool CapturingMouse() {return false;};

  	void GetMousePosition(double *x, double *y);
  
  virtual void SwapBuffers() = 0;

  // virtual void PollEvents() = 0;

	virtual void Destroy() {};

  };
}
