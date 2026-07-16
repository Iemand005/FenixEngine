#include <openvr.h>
#include <iostream>

#include "OpenVR.hpp"

const char* OVERLAY_KEY = "be.lasse.fenixoverlay";
const char* OVERLAY_NAME = "Fenix Engine Overlay";

using namespace fe;

void OpenVR::Init() {
	std::cout << "Initializing OpenVR..." << std::endl;

	vr::EVRInitError initError = vr::VRInitError_None;
    vr::IVRSystem* vrSystem = vr::VR_Init(&initError, vr::VRApplication_Overlay);

	if (initError != vr::VRInitError_None) {
        std::cout << "OpenVR Init Failed! Error code: " 
                  << vr::VR_GetVRInitErrorAsEnglishDescription(initError) 
                  << std::endl;
        std::cout << "Note: Make sure SteamVR is installed and running." << std::endl;
        return;
    }

    std::cout << "Successfully connected to SteamVR!" << std::endl;
}

void OpenVR::InitOverlay() {
	vr::IVROverlay* vrOverlay = vr::VROverlay();
    if (!vrOverlay) {
        std::cout << "Could not get IVROverlay interface!" << std::endl;
        vr::VR_Shutdown();
        return;
    }

    vr::VROverlayHandle_t handle;
    vr::EVROverlayError overlayError = vr::VROverlay()->CreateOverlay(OVERLAY_KEY, OVERLAY_NAME, &handle);
    overlayHandle = (uint64_t)handle;

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
    vrOverlay->SetOverlayTransformTrackedDeviceRelative(handle, vr::TrackedDeviceIndex_Hmd, &transform);

    vrOverlay->SetOverlayFlag(handle, vr::VROverlayFlags_MakeVisibleToDashboard, true);

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

    vr::VROverlayHandle_t handle = (vr::VROverlayHandle_t)overlayHandle;
    vr::Texture_t vrTex = {(void*)(uintptr_t)overlayTexture, vr::TextureType_OpenGL, vr::ColorSpace_Gamma};
    vr::VROverlay()->SetOverlayTexture(handle, &vrTex);
}

void OpenVR::Shutdown() {
    if (overlayTexture) glDeleteTextures(1, &overlayTexture);
    if (overlayDepth) glDeleteRenderbuffers(1, &overlayDepth);
    if (overlayFbo) glDeleteFramebuffers(1, &overlayFbo);
	vr::VR_Shutdown();
}
