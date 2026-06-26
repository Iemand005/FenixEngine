#pragma once

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <iostream>
#include <memory>
#include <functional>
#include "IWindow.hpp"

namespace fe {

class GLFW3Window : public IWindow {
public:
	GLFW3Window(std::string title, int width, int height, bool hidden = false, bool fullscreen = false);
	~GLFW3Window();

	bool InitGlfw(std::string title, bool fullscreen = true, bool tenBit = true);

	void StartMouseCapture() override;
	void StopMouseCapture() override;

	void PollGLFWEvents() { glfwPollEvents(); }

	void SetSwapInterval(int interval) override { glfwSwapInterval(interval); }

	void SwapBuffers() override;

	void Destroy() override;

	void* GetWindow();

	void GetMousePosition(double *x, double *y);

	bool ShouldClose();
	void PrepareClose();

	double GetTime();

	void GetFramebufferSize(int *width, int *height);

	bool HideMouse();

  void SetTitle(const char *newTitle) override;

	void Hide();
  	void Show();

	void AttachToNativeParent(void* parent);

	void getDeltaTime() override;;

	  
#ifdef _WIN32
	HWND GetNativeWindow();
	HDC GetDrawingContext();
	HGLRC GetOpenGLRenderingContext();
#endif

	void SetAnyKeyCallback(std::function<void()> cb) { onAnyKey = std::move(cb); }
	void SetFramebufferResizeCallback(std::function<void(int width, int height)> cb) { onFramebufferResize = std::move(cb); }

private:
	struct Impl;
	std::unique_ptr<Impl> impl;
	std::string title;

	std::function<void()> onAnyKey = NULL;
	std::function<void(int width, int height)> onFramebufferResize = NULL;
};

} // namespace fe
