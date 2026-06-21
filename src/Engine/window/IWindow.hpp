#pragma once

#include <functional>
namespace fe {
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

  bool CapturingMouse() {return false;};

  	void GetMousePosition(double *x, double *y);
  
  virtual void SwapBuffers() = 0;

  // virtual void PollEvents() = 0;

	virtual void Destroy() {};

  };
}