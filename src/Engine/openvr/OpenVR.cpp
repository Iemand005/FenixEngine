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

    vr::VROverlayHandle_t overlayHandle;
    vr::EVROverlayError overlayError = vr::VROverlay()->CreateOverlay(OVERLAY_KEY, OVERLAY_NAME, &overlayHandle);
    
    if (overlayError != vr::VROverlayError_None) {
        std::cout << "Failed to create overlay handle!" << std::endl;
        vr::VR_Shutdown();
        return;
    }

    // vrOverlay->SetOverlayFlag(overlayHandle, vr::VROverlayFlags_MakeVisibleToDashboard, true);
    // vrOverlay->SetOverlayFlag(overlayHandle, vr::VROverlayFlags_SendVRDiscreteScrollEvents, true);

	// vr::VROverlay()->SetOverlayFlag(overlayHandle, vr::VROverlayFlags_SideBySide_Parallel, true);
    
    vrOverlay->SetOverlayWidthInMeters(overlayHandle, 1.5f);

    vrOverlay->ShowOverlay(overlayHandle);

    std::cout << "Overlay successfully registered and visible in SteamVR!" << std::endl;
}

void OpenVR::Shutdown() {
	vr::VR_Shutdown();
}