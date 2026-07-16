#pragma once

#include <glad/glad.h>

namespace vr {
	struct VROverlayHandle_t_;
	typedef uint64_t VROverlayHandle_t;
}

namespace fe {
	class OpenVR {
	public:
		vr::VROverlayHandle_t overlayHandle;
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
	};
}