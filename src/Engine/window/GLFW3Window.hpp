#pragma once

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <iostream>
#include <memory>
#include "IWindow.hpp"

namespace fe {

class GLFW3Window : public IWindow {
public:
	GLFW3Window(std::string title, int width, int height, bool hidden = false, bool fullscreen = false);
	~GLFW3Window();

	bool InitGlfw(bool fullscreen = true, bool tenBit = true);

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

	void Hide();
  	void Show();

#ifdef _WIN32
	void AttachToNativeParent(void* parent);
#endif

private:
	struct Impl;
	std::unique_ptr<Impl> impl;
	std::string title;
};

} // namespace fe