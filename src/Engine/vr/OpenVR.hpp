#pragma once

#include <glad/glad.h>
#include <cstdint>
#include <functional>

#include <glm/glm.hpp>

#ifdef FE_INCLUDE_OPENVR
#include <openvr.h>
#endif

namespace fe {
	class IRenderDevice;

	class OpenVR {
	public:
		enum class Mode { None, Overlay, Scene };

		Mode mode = Mode::None;

		// ---- Overlay (existing) ----
		uint64_t overlayHandle = 0;
		GLuint overlayFbo = 0;
		GLuint overlayTexture = 0;
		GLuint overlayDepth = 0;
		int fboWidth = 0;
		int fboHeight = 0;

		void Init();
		void InitOverlay();
		void CreateOverlayTexture(int width, int height);
		void CaptureAndSubmit();
		void Shutdown();

		// ---- HMD (new) ----
		void InitHMD(IRenderDevice* renderDevice);
		void WaitGetPoses();
		void GetEyeViewProjection(uint32_t eye, glm::mat4& view, glm::mat4& proj) const;
		void RenderHMDFrame(const std::function<void()>& renderScene);
		void ShutdownHMD();

	private:
		IRenderDevice* rd = nullptr;

		// HMD state
		void* vrSystem = nullptr;
		void* vrCompositor = nullptr;

		uint32_t renderWidth = 0;
		uint32_t renderHeight = 0;
		uint64_t eyeFramebuffers[2] = {};
		uint64_t eyeColorImages[2] = {};

#ifdef FE_INCLUDE_OPENVR
		vr::TrackedDevicePose_t hmdPoses_[vr::k_unMaxTrackedDeviceCount];
		vr::TrackedDevicePose_t gamePoses_[vr::k_unMaxTrackedDeviceCount];
#endif
	};
}
