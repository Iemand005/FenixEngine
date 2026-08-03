#pragma once

#ifdef __EMSCRIPTEN__
#define FE_EXCLUDE_GLFW
#endif

#ifndef FE_EXCLUDE_GLFW

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
	GLFW3Window(std::string title, int width, int height, bool hidden = false, bool fullscreen = false, WindowOptions options = {}, bool useVulkan = false);
	~GLFW3Window();

	bool InitGlfw(std::string title, bool fullscreen = true, bool tenBit = true);

	void StartMouseCapture() override;
	void StopMouseCapture() override;

	void PollGLFWEvents() { glfwPollEvents(); }

	void SetSwapInterval(int interval) override { glfwSwapInterval(interval); }

	// //void GetSize(int* w, int* h) override { glfwGetWindowSize(impl->window, w, h); }
	// void Resize(int w, int h) override { glfwSetWindowSize(impl->window, w, h); }
	// void Move(int x, int y) override { glfwSetWindowPos(impl->window, x, y); }
	// void SetBordered(bool enabled) override;
	// void SetFullscreen(bool enabled = false) override;

	// void SetVSync(bool enabled) override;

	// void SetTitle(const std::string& title) override { glfwSetWindowTitle(impl->window, title.c_str()); }
	// void SetMouseCapture(bool captureMouse = true) override;

	// void SetCursorVisible(bool visible) override { glfwSetInputMode(impl->window, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED); }
	// bool IsCursorVisible() const override { return impl->cursorVisible; }

	// void Show() override { glfwShowWindow(impl->window); }
	// void Hide() override { glfwHideWindow(impl->window); }
	// bool IsVisible() override { return glfwGetWindowAttrib(impl->window, GLFW_VISIBLE); }

	// void Focus() override { glfwFocusWindow(impl->window); }
	// bool IsFocused() override { return glfwGetWindowAttrib(impl->window, GLFW_FOCUSED); }

	// void SetIcon(const std::string& path) override;

	// void* GetNativeHandle() override { return glfwGetWin32Window(impl->window); }
	// void* GetSDLGLContext() override { return nullptr; }

	// GLFWwindow* GetWindow();

	// VulkanExtensions GetVulkanExtensions() override;
	// void* CreateVulkanSurface(void* instance) override;

	// bool ShouldClose() override { return glfwWindowShouldClose(impl->window); }

	// ~GLFW3Window() override;



	void SwapBuffers() const override;

	void Destroy() override;

	void* GetWindow();

	void GetMousePosition(double *x, double *y);

	bool ShouldClose();
	void PrepareClose();

	double GetTime() override;

	void GetFramebufferSize(int *width, int *height);
	WindowSize GetFramebufferSize() override;

	bool HideMouse();

	void SetFullscreen(bool enabled = false) override;
	void GoBorderlessFullscreen() override;

	void SetTitle(const char *newTitle) override;

	void Hide();
  	void Show();

	void AttachToNativeParent(void* parent);

	void MakeCurrentGLContext() const override;
	void UnbindGLContext() const override;

	VulkanExtensions GetVulkanExtensions() override;
	void *CreateVulkanSurface(void *instance) override;
	  
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
#endif // FE_EXCLUDE_GLFW
