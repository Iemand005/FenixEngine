#pragma once
#include "Game.hpp"
#include "window/IWindow.hpp"

#include <memory>

#ifdef FE_INCLUDE_OPENVR
#include "vr/OpenVR.hpp"
#else
namespace fe { class OpenVR; }
#endif

namespace fe {

	class OpenXR;

	struct XRGameOptions : RendererOptions {
		bool launchVR = false;
		bool drawWindow = true;

		XRGameOptions() { preferOpenXRDevice = true; }

		XRGameOptions(int width, int height) : fe::RendererOptions(width, height) { preferOpenXRDevice = true; }

		XRGameOptions(bool useVulkan, int width, int height) : fe::RendererOptions(width, height, useVulkan, false, false) { preferOpenXRDevice = true; }

		XRGameOptions(int width, int height, bool launchVR, bool drawWindow = true) : fe::RendererOptions(width, height), launchVR(launchVR), drawWindow(drawWindow) { preferOpenXRDevice = true; }
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

		// World-space offset added to the player position before applying the
		// reference-space tracking transform. Y is the eye height above the
		// player's feet; X/Z should normally stay zero (they previously were 1,
		// which shifted the head sideways/forward).
		glm::vec3 positionOffset = glm::vec3(0.0f, 1.0f, 0.0f);

		// Yaw "look" turn (radians) accumulated from the look input (e.g. left
		// thumbstick). Applied on top of the HMD orientation each VR frame.
		float playerYaw = 0.0f;

		// Set this to toggle the XR session on/off next frame (used by a
		// hotkey or a gamepad button).
		bool xrToggleRequested = false;

		bool running = true;
		
		uint32_t swapchainImageIndex;

		XRGame(bool launchVR = true);
		XRGame(int width, int height, bool launchVR = true, bool drawWindow = true, bool showWindow = true);
		XRGame(GLADloadproc loadProc);
		XRGame(XRGameOptions optioins);
		~XRGame();

		bool IsInstanceValid();
		OpenXR& GetOpenXR();
		const OpenXR& GetOpenXR() const;

		void initOpenXR();

		void EnableXR();
#ifdef FE_INCLUDE_OPENVR
		void StartOpenVR();
#endif
		void DisableVR();
		void LaunchVR();
		void ToggleXR();

		bool IsPassthroughSupported() const;
		bool IsPassthroughEnabled() const;
		void SetPassthroughEnabled(bool enabled);
		void TogglePassthrough();

		void Redraw(uint64_t fbo = 0) ;
		void RedrawVR();
		void RedrawWindow(uint64_t fbo = 0);

		void PollActionsAndUpdateMovement();

		void DrawUI() override;

		void DestroyXR();

		void Destroy() {
			DestroyXR();
			Game::Destroy();
		}
	};
}