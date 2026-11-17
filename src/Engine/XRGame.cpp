#include "XRGame.hpp"
#include "vr/OpenXR.hpp"

#ifdef FE_HAS_VULKAN
#include "Graphics/VulkanDevice.hpp"
#endif

#include <algorithm>
#include <cmath>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <SDL3/SDL.h>

#ifndef FE_EXCLUDE_OPENXR
#if defined(__linux__) || defined(__unix__)
    #include <X11/Xlib.h>
    #include <GL/glx.h>
#endif
#endif

#include <imgui.h>

using namespace fe;

#ifndef FE_EXCLUDE_OPENXR
namespace {
// Builds an off-axis projection matrix already expressed in Vulkan clip space
// (y down, z in [0, 1]). OpenXR's fov angles use the OpenGL convention
// (angleLeft/angleDown negative), so start from a GL frustum and then apply the
// exact GL -> Vulkan clip conversion once, here, instead of patching matrices
// with global flags in the device.
glm::mat4 MakeVulkanProjectionFromFov(const glm::vec4& fov, float nearZ, float farZ) {
	const float tanLeft = std::tan(fov.x);
	const float tanRight = std::tan(fov.y);
	const float tanDown = std::tan(fov.z);
	const float tanUp = std::tan(fov.w);

	glm::mat4 gl = glm::frustum(tanLeft * nearZ, tanRight * nearZ,
		tanDown * nearZ, tanUp * nearZ, nearZ, farZ);

	glm::mat4 vk = gl;
	// Mirror Y (including the off-axis skew term).
	for (int c = 0; c < 4; ++c)
		vk[c][1] = -vk[c][1];
	// Remap depth z from [-1, 1] to [0, 1]: row2' = 0.5 * (row2 + row3).
	for (int c = 0; c < 4; ++c)
		vk[c][2] = 0.5f * (vk[c][2] + vk[c][3]);
	return vk;
}
}
#endif

struct fe::XRGame::Impl {
	bool useVulkan = false;
	bool drawVR = false;
	bool drawOpenVR = false;
	OpenXR input;
	OpenXR::SwapchainInfo swapchainInfo;
	std::vector<std::vector<uint64_t>> framebuffers;
	std::vector<GLuint> depthTextures;

	void initSwapchain(IRenderDevice* renderDevice) {
#ifndef FE_EXCLUDE_OPENXR
		if (!input.CreateSwapchain(renderDevice->GetSwapchainFormat(), swapchainInfo)) {
			fe::LogWarning("Failed to create OpenXR swapchain");
			return;
		}

		framebuffers.resize(swapchainInfo.viewCount);
		for (uint32_t eye = 0; eye < swapchainInfo.viewCount; ++eye) {
			framebuffers[eye].resize(swapchainInfo.images.size());
			for (size_t imageIndex = 0; imageIndex < swapchainInfo.images.size(); ++imageIndex) {
				framebuffers[eye][imageIndex] = renderDevice->CreateFramebuffer(
					swapchainInfo.images[imageIndex], swapchainInfo.width,
					swapchainInfo.height, eye, 0, swapchainInfo.format);
			}
		}

		if (!useVulkan) {
			depthTextures.resize(swapchainInfo.images.size());
			glGenTextures(static_cast<GLsizei>(depthTextures.size()), depthTextures.data());
			for (GLuint texture : depthTextures) {
				glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
				glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F,
					swapchainInfo.width, swapchainInfo.height,
					swapchainInfo.viewCount, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		}
#else
		(void)renderDevice;
#endif
	}
#ifndef FE_EXCLUDE_OPENXR

	void CreateAction(XrActionType type, std::string name, XrAction* action) {
		// NOTE: Build and pretty-print the name in a fixed buffer; actionName
		// is XR_MAX_ACTION_NAME_SIZE (64) and localizedActionName 128.
		std::string pretty = name;
		std::replace(pretty.begin(), pretty.end(), '_', ' ');

		XrActionCreateInfo actionInfo{XR_TYPE_ACTION_CREATE_INFO};
		actionInfo.actionType = type;
		strncpy(actionInfo.actionName, name.c_str(), XR_MAX_ACTION_NAME_SIZE - 1);
		strncpy(actionInfo.localizedActionName, pretty.c_str(), XR_MAX_LOCALIZED_ACTION_NAME_SIZE - 1);
		outputError(xrCreateAction(actionSet, &actionInfo, action));
	}

	void SuggestProfileBindings(const char* profilePath, const std::vector<XrActionSuggestedBinding>& bindings) {
		XrPath profilePathHandle;
		XrResult r = xrStringToPath(instance, profilePath, &profilePathHandle);
		if (XR_FAILED(r)) return;

		XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
		suggestedBindings.interactionProfile = profilePathHandle;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		xrSuggestInteractionProfileBindings(instance, &suggestedBindings);
	}

	XrPath Path(const std::string& path) {
		XrPath p;
		xrStringToPath(instance, path.c_str(), &p);
		return p;
	}

	void CreateActions() {
		#ifndef FE_EXCLUDE_OPENXR

		XrActionSetCreateInfo actionSetInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
		strcpy(actionSetInfo.actionSetName, "gameplay");
		strcpy(actionSetInfo.localizedActionSetName, "Gameplay");
		xrCreateActionSet(instance, &actionSetInfo, &actionSet);

		CreateAction(XR_ACTION_TYPE_VECTOR2F_INPUT, "move", &moveAction);
		CreateAction(XR_ACTION_TYPE_VECTOR2F_INPUT, "look", &lookAction);
		CreateAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "toggle_xr", &toggleXrAction);
		CreateAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "break_block", &breakBlockAction);
		CreateAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "place_block", &placeBlockAction);
		CreateAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "prev_block", &prevBlockAction);
		CreateAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "next_block", &nextBlockAction);

		// Pose actions: grip = the physical controller body, aim = the
		// pointer/ray pose from the tip (used for ray casting). One per hand,
		// so four poses in total.
		CreateAction(XR_ACTION_TYPE_POSE_INPUT, "left_grip_pose", &leftGripPoseAction);
		CreateAction(XR_ACTION_TYPE_POSE_INPUT, "left_aim_pose", &leftAimPoseAction);
		CreateAction(XR_ACTION_TYPE_POSE_INPUT, "right_grip_pose", &rightGripPoseAction);
		CreateAction(XR_ACTION_TYPE_POSE_INPUT, "right_aim_pose", &rightAimPoseAction);

		// Movement on the right thumbstick, look/turn on the left thumbstick.
		// Binding is suggested for the common interaction profiles that expose
		// a thumbstick or trackpad on each hand.
		const char* thumbstickProfiles[] = {
			"/interaction_profiles/oculus/touch_controller",
			"/interaction_profiles/microsoft/motion_controller",
			"/interaction_profiles/valve/index_controller",
			"/interaction_profiles/bytedance/pico_touch_controller",
			"/interaction_profiles/meta/touch_pro_controller",
		};

		for (auto profile : thumbstickProfiles) {
			SuggestProfileBindings(profile, {
				{moveAction, Path("/user/hand/right/input/thumbstick")},
				{lookAction, Path("/user/hand/left/input/thumbstick")},
				// Toggle the XR session from the face buttons (X on the left hand, A on the right).
				{toggleXrAction, Path("/user/hand/left/input/x/click")},
				{toggleXrAction, Path("/user/hand/right/input/a/click")},
				// The index-finger trigger sits on top of these controllers, so it
				// maps to the "shoulder" buttons: L1/R1 switch the hotbar slot.
				{prevBlockAction, Path("/user/hand/left/input/trigger")},
				{nextBlockAction, Path("/user/hand/right/input/trigger")},
				// The grip (squeeze) button sits lower, like a trigger: L2 breaks,
				// R2 places.
				{breakBlockAction, Path("/user/hand/left/input/squeeze")},
				{placeBlockAction, Path("/user/hand/right/input/squeeze")},
				// Grip pose = controller body, aim pose = pointer/ray tip. Both
				// paths are part of the base controller layout on all these profiles.
				{leftGripPoseAction, Path("/user/hand/left/input/grip/pose")},
				{leftAimPoseAction, Path("/user/hand/left/input/aim/pose")},
				{rightGripPoseAction, Path("/user/hand/right/input/grip/pose")},
				{rightAimPoseAction, Path("/user/hand/right/input/aim/pose")},
			});
		}

		// HTC Vive only has a trackpad on each controller.
		SuggestProfileBindings("/interaction_profiles/htc/vive_controller", {
			{moveAction, Path("/user/hand/right/input/trackpad")},
			{lookAction, Path("/user/hand/left/input/trackpad")},
			// L1/R1 = trigger (hotbar), L2/R2 = squeeze (break/place).
			{prevBlockAction, Path("/user/hand/left/input/trigger")},
			{nextBlockAction, Path("/user/hand/right/input/trigger")},
			{breakBlockAction, Path("/user/hand/left/input/squeeze")},
			{placeBlockAction, Path("/user/hand/right/input/squeeze")},
			{leftGripPoseAction, Path("/user/hand/left/input/grip/pose")},
			{leftAimPoseAction, Path("/user/hand/left/input/aim/pose")},
			{rightGripPoseAction, Path("/user/hand/right/input/grip/pose")},
			{rightAimPoseAction, Path("/user/hand/right/input/aim/pose")},
		});

		XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
		attachInfo.actionSets = &actionSet;
		attachInfo.countActionSets = 1;
		xrAttachSessionActionSets(session, &attachInfo);

		// Action spaces can only be created after the action set is attached
		// to the session. Identity pose in the action space: the located space
		// pose *is* the controller pose.
		auto createActionSpace = [&](XrAction action, XrSpace* space) {
			if (action == XR_NULL_HANDLE) return;
			XrActionSpaceCreateInfo spaceInfo{XR_TYPE_ACTION_SPACE_CREATE_INFO};
			spaceInfo.action = action;
			spaceInfo.subactionPath = XR_NULL_PATH;
			spaceInfo.poseInActionSpace.orientation = {0.0f, 0.0f, 0.0f, 1.0f};
			spaceInfo.poseInActionSpace.position = {0.0f, 0.0f, 0.0f};
			outputError(xrCreateActionSpace(session, &spaceInfo, space));
		};
		createActionSpace(leftGripPoseAction, &gripPoseSpace[0]);
		createActionSpace(leftAimPoseAction, &aimPoseSpace[0]);
		createActionSpace(rightGripPoseAction, &gripPoseSpace[1]);
		createActionSpace(rightAimPoseAction, &aimPoseSpace[1]);
		#endif

	}
	#endif

	// Note: this lives inside the XRGame pimpl. Calling fe::Log("%s", msg.c_str())
	// so XR diagnostics also reach logcat on Android.
	void Log(const std::string& message) { fe::Log("%s", message.c_str()); }

	void BeginSession() {
		#ifndef FE_EXCLUDE_OPENXR

		XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
		beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

		XrResult result = xrBeginSession(session, &beginInfo);
		outputError(result);
		// The Oculus runtime will not transition the session to VISIBLE/FOCUSED
		// until the application starts its frame loop (xrWaitFrame). Start it
		// immediately after begin so the compositor leaves the loading screen.
		if (XR_SUCCEEDED(result)) {
			drawVR = true;
			Log("xrBeginSession succeeded - starting frame loop");
		}
		#endif

	}
#ifndef FE_EXCLUDE_OPENXR

	void HandleSessionStateChange(XrSessionState state, XrTime time) {
		switch (state) {
			case XR_SESSION_STATE_IDLE:
				Log("Session state: IDLE");
				break;
			case XR_SESSION_STATE_READY:
				Log("Session state: READY - Calling xrBeginSession");
				BeginSession();
				break;
			case XR_SESSION_STATE_SYNCHRONIZED:
				Log("Session state: SYNCHRONIZED");
				break;
			case XR_SESSION_STATE_VISIBLE:
				Log("Session state: VISIBLE - Render but don't submit");
				drawVR = true;
				break;
			case XR_SESSION_STATE_FOCUSED:
				Log("Session state: FOCUSED - Render and submit");
				drawVR = true;
				break;
			case XR_SESSION_STATE_STOPPING:
				Log("Session state: STOPPING - Should call xrEndSession");
				xrEndSession(session);
				drawVR = false;
				break;
		}
	}
#endif

	void PollEvents() {
		#ifndef FE_EXCLUDE_OPENXR

		if (instance == XR_NULL_HANDLE) return;

		XrEventDataBuffer event = {XR_TYPE_EVENT_DATA_BUFFER};

		while (xrPollEvent(instance, &event) == XR_SUCCESS) {
			switch (event.type) {
				case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
					XrEventDataSessionStateChanged* stateChanged = (XrEventDataSessionStateChanged*)&event;
		XrSessionState currentState = stateChanged->state;
		XrTime time = stateChanged->time;
		sessionState = currentState;
		HandleSessionStateChange(currentState, time);
				} break;
			}
			event = {XR_TYPE_EVENT_DATA_BUFFER};
		}
		#endif

	}

	#ifndef FE_EXCLUDE_OPENXR

	void outputError(XrResult result) {
		if (XR_SUCCEEDED(result)) return;
		char buf[XR_MAX_RESULT_STRING_SIZE];
		if (xrResultToString(instance, result, buf) == XR_SUCCESS) {
			std::cerr << "Error: " << buf << " (" << result << ")" << std::endl;
			Log("OpenXR Error: " + std::string(buf) + " (" + std::to_string(result) + ")");
		}
	}

	void LoadPassthroughFunctions() {
		xrGetInstanceProcAddr(instance, "xrCreatePassthroughFB", (PFN_xrVoidFunction*)&pfnCreatePassthroughFB);
		xrGetInstanceProcAddr(instance, "xrDestroyPassthroughFB", (PFN_xrVoidFunction*)&pfnDestroyPassthroughFB);
		xrGetInstanceProcAddr(instance, "xrPassthroughStartFB", (PFN_xrVoidFunction*)&pfnPassthroughStartFB);
		xrGetInstanceProcAddr(instance, "xrPassthroughPauseFB", (PFN_xrVoidFunction*)&pfnPassthroughPauseFB);
		xrGetInstanceProcAddr(instance, "xrCreatePassthroughLayerFB", (PFN_xrVoidFunction*)&pfnCreatePassthroughLayerFB);
		xrGetInstanceProcAddr(instance, "xrDestroyPassthroughLayerFB", (PFN_xrVoidFunction*)&pfnDestroyPassthroughLayerFB);
		xrGetInstanceProcAddr(instance, "xrPassthroughLayerResumeFB", (PFN_xrVoidFunction*)&pfnPassthroughLayerResumeFB);
		xrGetInstanceProcAddr(instance, "xrPassthroughLayerPauseFB", (PFN_xrVoidFunction*)&pfnPassthroughLayerPauseFB);
	}

	bool EnsurePassthroughCreated() {
		if (!passthroughSupported) return false;

		if (passthrough == XR_NULL_HANDLE && pfnCreatePassthroughFB) {
			XrPassthroughCreateInfoFB passthroughInfo{XR_TYPE_PASSTHROUGH_CREATE_INFO_FB};
			passthroughInfo.flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB;
			outputError(pfnCreatePassthroughFB(session, &passthroughInfo, &passthrough));
		}

		if (passthrough == XR_NULL_HANDLE) return false;

		if (passthroughLayer == XR_NULL_HANDLE && pfnCreatePassthroughLayerFB) {
			XrPassthroughLayerCreateInfoFB layerInfo{XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB};
			layerInfo.passthrough = passthrough;
			layerInfo.flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB;
			layerInfo.purpose = XR_PASSTHROUGH_LAYER_PURPOSE_RECONSTRUCTION_FB;
			outputError(pfnCreatePassthroughLayerFB(session, &layerInfo, &passthroughLayer));
		}

		return passthroughLayer != XR_NULL_HANDLE;
	}

	void EnablePassthrough() {
		if (!passthroughSupported) return;
		if (!EnsurePassthroughCreated()) return;

		if (pfnPassthroughStartFB) outputError(pfnPassthroughStartFB(passthrough));
		if (pfnPassthroughLayerResumeFB) outputError(pfnPassthroughLayerResumeFB(passthroughLayer));

		passthroughEnabled = true;
		Log("Passthrough enabled");
	}

	void DisablePassthrough() {
		if (pfnPassthroughLayerPauseFB && passthroughLayer != XR_NULL_HANDLE) outputError(pfnPassthroughLayerPauseFB(passthroughLayer));
		if (pfnPassthroughPauseFB && passthrough != XR_NULL_HANDLE) outputError(pfnPassthroughPauseFB(passthrough));

		passthroughEnabled = false;
		Log("Passthrough disabled");
	}

	void DestroyPassthrough() {
		passthroughEnabled = false;

		if (passthroughLayer != XR_NULL_HANDLE && pfnDestroyPassthroughLayerFB) {
			outputError(pfnDestroyPassthroughLayerFB(passthroughLayer));
		}
		if (passthrough != XR_NULL_HANDLE && pfnDestroyPassthroughFB) {
			outputError(pfnDestroyPassthroughFB(passthrough));
		}

		passthroughLayer = XR_NULL_HANDLE;
		passthrough = XR_NULL_HANDLE;
	}
	#endif

}; // Impl

XRGame::XRGame(bool launchVR) : XRGame(0, 0, launchVR, true) {}

XRGame::XRGame(int width, int height, bool launchVR, bool drawWindow, bool showWindow) : Game(width, height, false, showWindow), impl(std::make_unique<Impl>()) {
	this->drawWindow = drawWindow;
	if (launchVR) LaunchVR();
}

XRGame::XRGame(GLADloadproc loadProc) : Game(loadProc), impl(std::make_unique<Impl>()) {}

XRGame::XRGame(XRGameOptions options) : Game((RendererOptions)options), impl(std::make_unique<Impl>()) {
	this->drawWindow = options.drawWindow;
	if (options.launchVR) LaunchVR();
}

XRGame::~XRGame() {
	Destroy();
};

OpenXR& XRGame::GetOpenXR() {
	return impl->input;
}

const OpenXR& XRGame::GetOpenXR() const {
	return impl->input;
}

void XRGame::initOpenXR() {
#ifndef FE_EXCLUDE_OPENXR
	OpenXR::GraphicsBinding binding;
	impl->useVulkan = false;

#ifdef XR_USE_GRAPHICS_API_VULKAN
	if (useVulkan && renderDevice && renderDevice->IsVulkan()) {
		auto* vk = static_cast<VulkanDevice*>(renderDevice.get());
		binding.useVulkan = true;
		binding.instance = vk->GetInstance();
		binding.physicalDevice = vk->GetPhysicalDevice();
		binding.device = vk->GetDevice();
		binding.queueFamilyIndex = vk->GetGraphicsQueueFamily();
		impl->useVulkan = true;
	}
#endif

	if (!binding.useVulkan) {
		auto window = GetWindow<fe::SDLWindow>();
#ifdef _WIN32
		binding.display = window->GetDrawingContext();
		binding.context = window->GetOpenGLRenderingContext();
#elif defined(__ANDROID__)
		EGLDisplay display = static_cast<EGLDisplay>(window->GetEGLDisplay());
		if (!display)
			display = eglGetCurrentDisplay();
		EGLContext context = static_cast<EGLContext>(window->GetEGLContext());
		if (!context)
			context = eglGetCurrentContext();
		EGLConfig config = static_cast<EGLConfig>(window->GetEGLConfig());
		if (!config && display && context) {
			EGLint configId = 0;
			if (eglQueryContext(display, context, EGL_CONFIG_ID, &configId)) {
				EGLint attributes[] = {EGL_CONFIG_ID, configId, EGL_NONE};
				int configCount = 0;
				eglChooseConfig(display, attributes, &config, 1, &configCount);
			}
		}
		binding.display = display;
		binding.config = config;
		binding.context = context;
#else
		const char* videoDriver = SDL_GetCurrentVideoDriver();
		if (videoDriver && SDL_strcmp(videoDriver, "wayland") == 0) {
			binding.windowSystem = OpenXR::GraphicsBinding::WindowSystem::Wayland;
			binding.display = window->GetWaylandDisplay();
		} else if (videoDriver && SDL_strcmp(videoDriver, "x11") == 0) {
			binding.windowSystem = OpenXR::GraphicsBinding::WindowSystem::X11;
			Display* display = static_cast<Display*>(window->GetX11Display());
			GLXContext context = glXGetCurrentContext();
			int framebufferConfigId = 0;
			glXQueryContext(display, context, GLX_FBCONFIG_ID, &framebufferConfigId);
			int attributes[] = {GLX_FBCONFIG_ID, framebufferConfigId, None};
			int configCount = 0;
			GLXFBConfig* configs = glXChooseFBConfig(display, DefaultScreen(display), attributes, &configCount);
			if (!configs || configCount == 0) {
				fe::LogWarning("Failed to find the OpenGL framebuffer config for OpenXR");
				return;
			}
			GLXFBConfig framebufferConfig = configs[0];
			XFree(configs);
			int visualId = 0;
			glXGetFBConfigAttrib(display, framebufferConfig, GLX_VISUAL_ID, &visualId);
			binding.display = display;
			binding.context = context;
			binding.drawable = static_cast<uintptr_t>(window->GetGLXDrawable());
			binding.framebufferConfig = reinterpret_cast<uintptr_t>(framebufferConfig);
			binding.visualId = static_cast<uintptr_t>(visualId);
		} else {
			fe::LogWarning("Unsupported SDL video driver for OpenXR");
			return;
		}
#endif
	}

	if (!impl->input.Initialize(binding))
		fe::LogWarning("OpenXR initialization failed");
#endif
}

void XRGame::PollActionsAndUpdateMovement() {
	#ifndef FE_EXCLUDE_OPENXR

	const OpenXR::InputState& rawInput = impl->input.GetInputState();

	const float lookDeadzone = 0.1f;
	const float turnSpeed = 1.2f;
	if (fabsf(rawInput.lookInput.x) > lookDeadzone) {
		double dt = std::max(fpsCounter.deltaTime, 0.0001);
		playerYaw -= rawInput.lookInput.x * turnSpeed * static_cast<float>(dt);
	}

	glm::vec3 playerPosition = player ? player->state.position : glm::vec3(0.0f);
	glm::quat yawQ = glm::angleAxis(playerYaw, glm::vec3(0.0f, 1.0f, 0.0f));
	const OpenXR::InputState input = impl->input.GetWorldInputState(playerPosition + positionOffset, yawQ);

	glm::quat viewOrientation = input.headOrientation;

	const float moveDeadzone = 0.1f;
	if (fabsf(input.moveInput.x) > moveDeadzone || fabsf(input.moveInput.y) > moveDeadzone) {
		glm::vec3 forward = viewOrientation * glm::vec3(0.0f, 0.0f, 1.0f);
		glm::vec3 right = viewOrientation * glm::vec3(1.0f, 0.0f, 0.0f);
		
		forward.y = 0.0f;
		right.y = 0.0f;
		forward = glm::normalize(forward);
		right = glm::normalize(right);

		if (player) {
			player->pendingMovement += forward * -input.moveInput.y;
			player->pendingMovement += right * input.moveInput.x;
		}
	}
	#endif
}

bool XRGame::IsInstanceValid() {
	return impl->input.IsInstanceValid();
}

#ifdef FE_INCLUDE_OPENVR
void XRGame::StartOpenVR() {
	openVR = std::make_unique<fe::OpenVR>();
	openVR->InitHMD(renderDevice.get());
	if (openVR->mode == OpenVR::Mode::Scene) {
		impl->drawOpenVR = true;
		impl->drawVR = false;
		window->StopMouseCapture();
	}
}
#endif

void XRGame::DisableVR() {
	impl->input.RequestExitSession();
}

void XRGame::ToggleXR() {
	if (IsInstanceValid()) {
		DestroyXR();
	} else {
		EnableXR();
	}
}

bool XRGame::IsPassthroughSupported() const {
	return impl->input.IsPassthroughSupported();
}

bool XRGame::IsPassthroughEnabled() const {
	return impl->input.IsPassthroughEnabled();
}

void XRGame::SetPassthroughEnabled(bool enabled) {
	if (enabled == impl->input.IsPassthroughEnabled()) return;

	impl->input.SetPassthroughEnabled(enabled);
	SetClearColor(clearColorR_, clearColorG_, clearColorB_,
		impl->input.IsPassthroughEnabled() ? 0.0f : 1.0f);
}

void XRGame::TogglePassthrough() {
	SetPassthroughEnabled(!IsPassthroughEnabled());
}

void XRGame::DrawUI() {
	#ifdef FE_INCLUDE_OPENVR
	if (openVR && openVR->mode == OpenVR::Mode::Scene) {
		ImGui::Begin("XR");
		ImGui::Text("OpenVR HMD active");
		ImGui::End();
		return;
	}

	#ifndef FE_EXCLUDE_OPENXR
	if (impl->drawVR) return;
	#endif

	ImGui::Begin("XR");
	if (ImGui::Button("Start OpenVR HMD")) {
		StartOpenVR();
	}
	ImGui::End();
	#endif
}

void XRGame::DestroyXR() {
	impl->drawVR = false;

	for (auto& eyeFBs : impl->framebuffers) {
		for (auto fb : eyeFBs) {
			if (renderDevice)
				renderDevice->DestroyFramebuffer(fb);
		}
	}
	impl->framebuffers.clear();

	if (!impl->useVulkan && !impl->depthTextures.empty()) {
		glDeleteTextures(static_cast<GLsizei>(impl->depthTextures.size()), impl->depthTextures.data());
		impl->depthTextures.clear();
	}

	impl->input.Shutdown();
	impl->swapchainInfo = OpenXR::SwapchainInfo{};

	#ifdef FE_INCLUDE_OPENVR
	if (openVR && openVR->mode == OpenVR::Mode::Scene) {
		openVR->ShutdownHMD();
	}
	openVR.reset();
	impl->drawOpenVR = false;
	#endif
}

void XRGame::LaunchVR() {
	#ifndef FE_EXCLUDE_OPENXR
	initOpenXR();
	if (!IsInstanceValid()) {
		fe::LogWarning("OpenXR instance invalid or no runtime found - staying in standard mode");
		return;
	}
	impl->initSwapchain(renderDevice.get());
	if (impl->swapchainInfo.images.empty()) {
		DestroyXR();
		return;
	}
	impl->drawVR = true;
	GetWindow()->StopMouseCapture();
	#endif
}

void XRGame::RedrawWindow(uint64_t fbo) {
	BindFrameBuffer((int)fbo);
	CheckErrors();
	Renderer::Redraw();
}

void XRGame::EnableXR() {
	#ifndef FE_EXCLUDE_OPENXR
	if (!IsInstanceValid()) LaunchVR();
	if (IsInstanceValid()) impl->drawVR = true;
	#endif
}

void XRGame::Redraw(uint64_t fbo) {
	if (xrToggleRequested) {
		xrToggleRequested = false;
		ToggleXR();
	}
	{
		impl->input.PollEvents();

		#ifdef FE_INCLUDE_OPENVR
		if (impl->drawOpenVR && openVR && openVR->mode == OpenVR::Mode::Scene) {
			openVR->RenderHMDFrame([this]() {
				for (auto& object : scene->GetObjects()) {
					RenderObject(*object);
				}
			});
		}
		#endif

		#ifndef FE_EXCLUDE_OPENXR
		if (impl->drawVR) RedrawVR();
		#endif
		if (drawWindow) RedrawWindow(fbo);
		CheckErrors();
	}
}

void XRGame::RedrawVR() {
#ifndef FE_EXCLUDE_OPENXR
	if (!impl->input.IsSessionRunning())
		return;

	std::vector<OpenXR::View> views;
	if (!impl->input.BeginFrame(swapchainImageIndex, views))
		return;
	PollActionsAndUpdateMovement();

	if (shader)
		UploadLights(shader.get(), scene.get());

	const glm::quat yaw = glm::angleAxis(playerYaw, glm::vec3(0.0f, 1.0f, 0.0f));
	const glm::vec3 playerPosition = player ? player->state.position : glm::vec3(0.0f);
	const glm::vec3 worldOrigin = playerPosition + positionOffset;

	for (uint32_t eye = 0; eye < views.size(); ++eye) {
		const auto& view = views[eye];
		const glm::vec3 worldEyePosition = worldOrigin + yaw * view.position;
		const glm::quat worldEyeOrientation = yaw * view.orientation;
		camera->update(worldEyePosition, worldEyeOrientation, view.fov);

		if (!impl->useVulkan) {
			const GLuint framebuffer = static_cast<GLuint>(impl->framebuffers[eye][swapchainImageIndex]);
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
				impl->depthTextures[swapchainImageIndex], 0, static_cast<GLint>(eye));
		}

		renderDevice->BeginVRFrame();
		if (impl->useVulkan) {
			const glm::mat4 vkProjection = MakeVulkanProjectionFromFov(
				view.fov, camera->nearDist, camera->farDist);
			renderDevice->SetVRViewProjection(camera->GetViewMatrix(),
				camera->GetProjectionMatrix(), vkProjection);
		} else {
			renderDevice->SetMat4("view", camera->GetViewMatrix());
			renderDevice->SetMat4("projection", camera->GetProjectionMatrix());
		}

		renderDevice->BeginEyeFrame(impl->framebuffers[eye][swapchainImageIndex],
			impl->swapchainInfo.width, impl->swapchainInfo.height);
		for (auto& object : scene->GetObjects())
			RenderObject(*object);
		OnDraw();
		renderDevice->EndEyeFrame();
		renderDevice->EndVRFrame();
	}

	if (impl->useVulkan)
		renderDevice->WaitForVRFrame();
	impl->input.EndFrame();
#endif
}
