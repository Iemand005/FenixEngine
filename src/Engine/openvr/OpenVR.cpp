#include <openvr.h>
#include <iostream>

#include "OpenVR.hpp"

using namespace fe;

void LogError(initError)

OpenVR::Init() {
	std::cout << "Initializing OpenVR..." << std::endl;

	vr::EVRInitError initError = vr::VRInitError_None;
    vr::IVRSystem* vrSystem = vr::VR_Init(&initError, vr::VRApplication_Scene);
}

OpenVR::Shutdown() {
	vr::VR_Shutdown();
}