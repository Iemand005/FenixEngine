#pragma once

#include <glad/glad.h>
#include <cstdint>

namespace fe {
	class OpenVR {
	public:
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
	};
}