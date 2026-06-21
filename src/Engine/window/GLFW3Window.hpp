#pragma once

#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include "IWindow.hpp"

namespace fe {

class GLFW3Window : public IWindow {
public:
	GLFW3Window(std::string title, int width, int height, bool hidden = false);
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

	double GetTime();

	void GetFramebufferSize(int *width, int *height);

	bool HideMouse();

private:
	struct Impl;
	std::unique_ptr<Impl> impl;
	std::string title;
};

} // namespace fe