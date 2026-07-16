#include <openvr.h>
#include <iostream>

#include "OpenVR.hpp"

using namespace fe;

void OpenVR::Init() {
	std::cout << "Initializing OpenVR..." << std::endl;

	vr::EVRInitError initError = vr::VRInitError_None;
    vr::IVRSystem* vrSystem = vr::VR_Init(&initError, vr::VRApplication_Scene);

	if (initError != vr::VRInitError_None) {
        std::cout << "OpenVR Init Failed! Error code: " 
                  << vr::VR_GetVRInitErrorAsEnglishDescription(initError) 
                  << std::endl;
        std::cout << "Note: Make sure SteamVR is installed and running." << std::endl;
        return;
    }

    std::cout << "Successfully connected to SteamVR!" << std::endl;
}

void OpenVR::Shutdown() {
	vr::VR_Shutdown();
}