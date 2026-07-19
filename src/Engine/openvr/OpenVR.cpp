#ifdef FE_INCLUDE_OPENVR
#include <openvr.h>
#include <iostream>

#include "OpenVR.hpp"
#include "../Graphics/IRenderDevice.hpp"
#include "../Graphics/VulkanDevice.hpp"

const char* OVERLAY_KEY = "be.lasse.fenixoverlay";
const char* OVERLAY_NAME = "Fenix Engine Overlay";

using namespace fe;

// ---------------------------------------------------------------------------
// Matrix conversion helpers
// ---------------------------------------------------------------------------
static glm::mat4 Hmd34ToGlm(const vr::HmdMatrix34_t& m) {
	glm::mat4 out{1.0f};
	out[0][0] = m.m[0][0]; out[1][0] = m.m[0][1]; out[2][0] = m.m[0][2]; out[3][0] = m.m[0][3];
	out[0][1] = m.m[1][0]; out[1][1] = m.m[1][1]; out[2][1] = m.m[1][2]; out[3][1] = m.m[1][3];
	out[0][2] = m.m[2][0]; out[1][2] = m.m[2][1]; out[2][2] = m.m[2][2]; out[3][2] = m.m[2][3];
	return out;
}

static glm::mat4 Hmd44ToGlm(const vr::HmdMatrix44_t& m) {
	glm::mat4 out{1.0f};
	out[0][0] = m.m[0][0]; out[1][0] = m.m[0][1]; out[2][0] = m.m[0][2]; out[3][0] = m.m[0][3];
	out[0][1] = m.m[1][0]; out[1][1] = m.m[1][1]; out[2][1] = m.m[1][2]; out[3][1] = m.m[1][3];
	out[0][2] = m.m[2][0]; out[1][2] = m.m[2][1]; out[2][2] = m.m[2][2]; out[3][2] = m.m[2][3];
	out[0][3] = m.m[3][0]; out[1][3] = m.m[3][1]; out[2][3] = m.m[3][2]; out[3][3] = m.m[3][3];
	return out;
}

// ---------------------------------------------------------------------------
// Legacy overlay methods (untouched)
// ---------------------------------------------------------------------------
void OpenVR::Init() {
	std::cout << "Initializing OpenVR..." << std::endl;
}

void OpenVR::InitOverlay() {
	vr::EVRInitError initError = vr::VRInitError_None;
	vr::IVRSystem* dummy = vr::VR_Init(&initError, vr::VRApplication_Overlay);
	if (initError != vr::VRInitError_None) {
		std::cout << "OpenVR Init Failed! Error code: "
		          << vr::VR_GetVRInitErrorAsEnglishDescription(initError)
		          << std::endl;
		std::cout << "Note: Make sure SteamVR is installed and running." << std::endl;
		return;
	}
	std::cout << "Successfully connected to SteamVR!" << std::endl;

	vr::IVROverlay* vrOverlay = vr::VROverlay();
	if (!vrOverlay) {
		std::cout << "Could not get IVROverlay interface!" << std::endl;
		vr::VR_Shutdown();
		return;
	}

	vr::VROverlayHandle_t handle;
	vr::EVROverlayError overlayError = vr::VROverlay()->CreateOverlay(OVERLAY_KEY, OVERLAY_NAME, &handle);
	overlayHandle = static_cast<uint64_t>(handle);

	if (overlayError != vr::VROverlayError_None) {
		std::cout << "Failed to create overlay handle!" << std::endl;
		vr::VR_Shutdown();
		return;
	}

	vrOverlay->SetOverlayWidthInMeters(handle, 2.0f);

	vr::HmdMatrix34_t transform = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, -0.5f,
		0.0f, 0.0f, 1.0f, -3.0f
	};
	vrOverlay->SetOverlayTransformTrackedDeviceRelative(handle, vr::k_unTrackedDeviceIndex_Hmd, &transform);
	vrOverlay->SetOverlayFlag(handle, vr::VROverlayFlags_VisibleInDashboard, true);

	if (overlayTexture) {
		vr::Texture_t vrTex = {(void*)(uintptr_t)overlayTexture, vr::TextureType_OpenGL, vr::ColorSpace_Gamma};
		vr::VROverlay()->SetOverlayTexture(handle, &vrTex);
	}

	vrOverlay->ShowOverlay(handle);
	std::cout << "Overlay registered — 3m in front of HMD, 0.5m below eye level." << std::endl;
}

void OpenVR::CreateOverlayTexture(int width, int height) {
	fboWidth = width;
	fboHeight = height;

	glGenTextures(1, &overlayTexture);
	glBindTexture(GL_TEXTURE_2D, overlayTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenRenderbuffers(1, &overlayDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, overlayDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glGenFramebuffers(1, &overlayFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, overlayFbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, overlayTexture, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, overlayDepth);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "OpenVR overlay FBO not complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenVR::CaptureAndSubmit() {
	if (!overlayHandle || !overlayTexture) return;

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, overlayFbo);
	glBlitFramebuffer(0, 0, fboWidth, fboHeight, 0, 0, fboWidth, fboHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	vr::VROverlayHandle_t handle = static_cast<vr::VROverlayHandle_t>(overlayHandle);
	vr::Texture_t vrTex = {(void*)(uintptr_t)overlayTexture, vr::TextureType_OpenGL, vr::ColorSpace_Gamma};
	vr::VROverlay()->SetOverlayTexture(handle, &vrTex);
}

void OpenVR::Shutdown() {
	if (overlayTexture) glDeleteTextures(1, &overlayTexture);
	if (overlayDepth) glDeleteRenderbuffers(1, &overlayDepth);
	if (overlayFbo) glDeleteFramebuffers(1, &overlayFbo);
	vr::VR_Shutdown();
}

// ---------------------------------------------------------------------------
// HMD methods
// ---------------------------------------------------------------------------
void OpenVR::InitHMD(IRenderDevice* renderDevice) {
	rd = renderDevice;

	vr::EVRInitError eError = vr::VRInitError_None;
	auto* sys = vr::VR_Init(&eError, vr::VRApplication_Scene);
	if (eError != vr::VRInitError_None) {
		std::cerr << "[OpenVR] HMD init failed: "
		          << vr::VR_GetVRInitErrorAsEnglishDescription(eError) << std::endl;
		return;
	}
	vrSystem = sys;
	vrCompositor = vr::VRCompositor();

	sys->GetRecommendedRenderTargetSize(&renderWidth, &renderHeight);
	std::cout << "[OpenVR] HMD ready — " << renderWidth << "x" << renderHeight
	          << " per eye (" << (rd->IsVulkan() ? "Vulkan" : "OpenGL") << ")" << std::endl;

	uint64_t depthFormat = rd->IsVulkan() ? 0 : GL_DEPTH_COMPONENT24;

	for (int eye = 0; eye < 2; ++eye) {
		eyeColorImages[eye]  = rd->CreateColorAttachment(renderWidth, renderHeight);
		eyeFramebuffers[eye] = rd->CreateFramebuffer(eyeColorImages[eye],
			renderWidth, renderHeight, 0, depthFormat);
	}

	mode = Mode::Scene;
}

void OpenVR::WaitGetPoses() {
	auto* compositor = static_cast<vr::IVRCompositor*>(vrCompositor);
	compositor->WaitGetPoses(hmdPoses_, vr::k_unMaxTrackedDeviceCount,
	                         gamePoses_, vr::k_unMaxTrackedDeviceCount);
}

void OpenVR::GetEyeViewProjection(uint32_t eye, glm::mat4& view, glm::mat4& proj) const {
	auto* sys = static_cast<vr::IVRSystem*>(vrSystem);

	// World-space HMD transform
	glm::mat4 hmdWorld = Hmd34ToGlm(hmdPoses_[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking);

	// Eye offset in head space
	glm::mat4 eyeLocal = Hmd34ToGlm(sys->GetEyeToHeadTransform(static_cast<vr::EVREye>(eye)));

	// World-space eye transform → view = inverse
	glm::mat4 eyeWorld = hmdWorld * eyeLocal;
	view = glm::inverse(eyeWorld);

	// Projection — legacy API (no convention enum in this SDK)
	vr::HmdMatrix44_t raw = sys->GetProjectionMatrix(static_cast<vr::EVREye>(eye), 0.01f, 100.0f);
	proj = Hmd44ToGlm(raw);
}

void OpenVR::RenderHMDFrame(const std::function<void()>& renderScene) {
	if (mode != Mode::Scene) return;

	auto* compositor = static_cast<vr::IVRCompositor*>(vrCompositor);

	// 1) Wait for poses
	compositor->WaitGetPoses(hmdPoses_, vr::k_unMaxTrackedDeviceCount,
	                         gamePoses_, vr::k_unMaxTrackedDeviceCount);

	// 2) Render each eye
	for (int eye = 0; eye < 2; ++eye) {
		glm::mat4 view, proj;
		GetEyeViewProjection(static_cast<uint32_t>(eye), view, proj);

		rd->SetMat4("view", view);
		rd->SetMat4("projection", proj);

		rd->BeginExternalFrame(eyeFramebuffers[eye], renderWidth, renderHeight);
		renderScene();
		rd->EndExternalFrame();

		// 3) Submit to compositor
		vr::Texture_t vrTex{};
		vr::VRTextureBounds_t bounds{0.0f, 0.0f, 1.0f, 1.0f};

		if (rd->IsVulkan()) {
			auto* vk = static_cast<VulkanDevice*>(rd);
			VkImage img = vk->GetColorAttachmentImage(eyeColorImages[eye]);

			vk->TransitionImageLayout(img,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

			vrTex = { (void*)img, vr::TextureType_Vulkan, vr::ColorSpace_Gamma };
		} else {
			GLuint tex = static_cast<GLuint>(eyeColorImages[eye]);
			vrTex = { (void*)(uintptr_t)tex, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		}

		compositor->Submit(static_cast<vr::EVREye>(eye), &vrTex, &bounds);
	}
}

void OpenVR::ShutdownHMD() {
	if (rd) {
		for (int eye = 0; eye < 2; ++eye) {
			if (eyeFramebuffers[eye])  rd->DestroyFramebuffer(eyeFramebuffers[eye]);
			if (eyeColorImages[eye])   rd->DestroyColorAttachment(eyeColorImages[eye]);
		}
	}
	vr::VR_Shutdown();
	vrSystem = nullptr;
	vrCompositor = nullptr;
	rd = nullptr;
	mode = Mode::None;
}

#endif