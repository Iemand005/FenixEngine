#include "Game.hpp"

#pragma once
#define XR_USE_GRAPHICS_API_OPENGL
#define XR_USE_GRAPHICS_API_VULKAN
#ifdef WIN32
#define XR_USE_PLATFORM_WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <unknwn.h>
#else
#define XR_USE_PLATFORM_WAYLAND
#define XR_USE_PLATFORM_XLIB
#endif

#include <memory>

#ifdef FE_INCLUDE_OPENVR
#include "openvr/OpenVR.hpp"
#else
namespace fe { class OpenVR; }
#endif

typedef int64_t XrTime;

namespace fe {

	struct XRGameOptions : RendererOptions {
		bool launchVR = false;
		bool drawWindow = true;

		XRGameOptions() = default; 

    	XRGameOptions(int width, int height) : fe::RendererOptions(width, height) {}

		XRGameOptions(int width, int height, bool launchVR, bool drawWindow = true) : fe::RendererOptions(width, height), launchVR(launchVR), drawWindow(drawWindow) {}
	};

	class XRGame : public fe::Game {
	private:
		struct Impl;
		std::unique_ptr<Impl> impl;

	public:

#ifdef FE_INCLUDE_OPENVR
		std::unique_ptr<fe::OpenVR> openVR;
#endif

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

		void EnableXR();
#ifdef FE_INCLUDE_OPENVR
		void StartOpenVR();
#endif
		void DisableVR();
		void LaunchVR();

		void Redraw(uint64_t fbo = 0) ;
		void RedrawVR();
		void RedrawWindow(uint64_t fbo = 0);

		void PollActionsAndUpdateMovement(XrTime predictedDisplayTime);

		void DrawUI() override;

		void DestroyXR();

		void Destroy() {
			DestroyXR();
			if (window) window->Destroy();
		}
	};
}
