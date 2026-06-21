#pragma once

#include <GLFW/glfw3.h>

#include <iostream>

#include "IWindow.hpp"

namespace fe {
	class GLFW3Window : public IWindow {
		
		public:
		GLFW3Window(std::string title, int width, int height, bool hidden = false);
		~GLFW3Window();
		
		bool InitGlfw(bool tenBit = false);
		
		void StartMouseCapture() override;
		
		void StopMouseCapture() override;
		
		void PollGLFWEvents() { glfwPollEvents(); }
		
		void SetSwapInterval(int interval) override { glfwSwapInterval(interval); }
		
		void SwapBuffers() override;
		
		void Destroy() override;
		
		void* GetGLFWWindow();
	private:
		struct Impl;
		std::unique_ptr<Impl> impl;
		std::string title;
	};
}  // namespace fe