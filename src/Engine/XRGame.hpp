#include "Game.hpp"
#include "Graphics/Renderer.hpp"
#include "window/IWindow.hpp"

#pragma once
#define XR_USE_GRAPHICS_API_OPENGL
#ifdef WIN32
#define XR_USE_PLATFORM_WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
// #define XR_EXTENSION_PROTOTYPES
// #define XR_KHR_opengl_enable
#ifdef XR_USE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <unknwn.h>
#endif
#else
#define XR_USE_PLATFORM_WAYLAND
#define XR_USE_PLATFORM_XLIB
#endif

#include <memory>

typedef int64_t XrTime;

namespace fe {

	struct XRGameOptions : RendererOptions {
		bool launchVR = false;
		bool drawWindow = true;

		XRGameOptions() = default; 

    	XRGameOptions(int w, int h) : fe::RendererOptions(w, h) {} 
	};

	class XRGame : public fe::Game {
	private:
		struct Impl;
		std::unique_ptr<Impl> impl;

	public:

		float playerHeight = 1.7f;

		bool drawWindow = true;

		glm::vec3 positionOffset = glm::vec3(1.0f);

		bool running = true;
		
		uint32_t swapchainImageIndex;

		XRGame(bool launchVR = true);
		XRGame(int width, int height, bool launchVR = true, bool drawWindow = true, bool showWindow = true);
		XRGame(GLADloadproc loadProc);
		XRGame(XRGameOptions optioins);
		~XRGame();

		bool IsInstanceValid();

		void initOpenXR();
		void initOpenXR(void *next);
#ifdef WIN32
		void initOpenXR(HDC hDC, HGLRC hGLRC);
#endif
		
		void EnableXR();
		void DisableVR();
		void LaunchVR();

		void Redraw(GLuint fbo = 0) ;
		void RedrawVR();
		void RedrawWindow(GLuint fbo = 0);

		void PollActionsAndUpdateMovement(XrTime predictedDisplayTime);

		void DestroyXR();

		void Destroy() {
			DestroyXR();
			if (window) window->Destroy();
		}
	};
}
