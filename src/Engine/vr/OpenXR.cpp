#include "OpenXR.hpp"

#include "../Log.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#ifdef __ANDROID__
#include <SDL3/SDL_system.h>
#endif

namespace fe {

	struct OpenXR::Impl {
		InputState inputState;
		bool buttonPressed[6] = {};
		bool buttonHeld[6] = {};
		bool gripPressedEdge[2] = {};
		bool prevGripPressed[2] = {};

#ifndef FE_EXCLUDE_OPENXR
		XrInstance instance = XR_NULL_HANDLE;
		XrSession session = XR_NULL_HANDLE;
		XrSpace appSpace = XR_NULL_HANDLE;
		XrSystemId systemId = XR_NULL_SYSTEM_ID;
		XrSwapchain swapchain = XR_NULL_HANDLE;
		XrSessionState sessionState = XR_SESSION_STATE_UNKNOWN;
		bool sessionRunning = false;
		bool useVulkan = false;
		uint32_t viewCount = 0;
		uint32_t swapchainWidth = 0;
		uint32_t swapchainHeight = 0;
		std::vector<XrView> locatedViews;
		XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
		XrFrameState frameState{XR_TYPE_FRAME_STATE};
		XrFrameBeginInfo frameBegin{XR_TYPE_FRAME_BEGIN_INFO};
		XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
		bool frameBegun = false;
		bool imageAcquired = false;
		bool passthroughExtensionAvailable = false;
		bool passthroughSupported = false;
		bool passthroughEnabled = false;
		XrPassthroughFB passthrough = XR_NULL_HANDLE;
		XrPassthroughLayerFB passthroughLayer = XR_NULL_HANDLE;

		PFN_xrCreatePassthroughFB pfnCreatePassthroughFB = nullptr;
		PFN_xrDestroyPassthroughFB pfnDestroyPassthroughFB = nullptr;
		PFN_xrPassthroughStartFB pfnPassthroughStartFB = nullptr;
		PFN_xrPassthroughPauseFB pfnPassthroughPauseFB = nullptr;
		PFN_xrCreatePassthroughLayerFB pfnCreatePassthroughLayerFB = nullptr;
		PFN_xrDestroyPassthroughLayerFB pfnDestroyPassthroughLayerFB = nullptr;
		PFN_xrPassthroughLayerResumeFB pfnPassthroughLayerResumeFB = nullptr;
		PFN_xrPassthroughLayerPauseFB pfnPassthroughLayerPauseFB = nullptr;

		XrActionSet actionSet = XR_NULL_HANDLE;
		XrAction moveAction = XR_NULL_HANDLE;
		XrAction lookAction = XR_NULL_HANDLE;
		XrAction leftGripPoseAction = XR_NULL_HANDLE;
		XrAction leftAimPoseAction = XR_NULL_HANDLE;
		XrAction rightGripPoseAction = XR_NULL_HANDLE;
		XrAction rightAimPoseAction = XR_NULL_HANDLE;
		XrAction buttonXAction = XR_NULL_HANDLE;
		XrAction buttonYAction = XR_NULL_HANDLE;
		XrAction buttonAAction = XR_NULL_HANDLE;
		XrAction buttonBAction = XR_NULL_HANDLE;
		XrAction leftTriggerAction = XR_NULL_HANDLE;
		XrAction rightTriggerAction = XR_NULL_HANDLE;
		XrAction leftHapticAction = XR_NULL_HANDLE;
		XrAction rightHapticAction = XR_NULL_HANDLE;
		XrAction leftGripAction = XR_NULL_HANDLE;
		XrAction rightGripAction = XR_NULL_HANDLE;

		XrSpace gripPoseSpace[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE};
		XrSpace aimPoseSpace[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE};

		float pendingHapticAmplitude[2] = {0.0f, 0.0f};
		float pendingHapticDuration[2] = {0.0f, 0.0f};

		void Log(const std::string& message) const {
			fe::Log("%s", message.c_str());
		}

		void OutputError(XrResult result) const {
			if (XR_SUCCEEDED(result))
				return;
			char message[XR_MAX_RESULT_STRING_SIZE] = {};
			if (instance != XR_NULL_HANDLE && XR_SUCCEEDED(xrResultToString(instance, result, message))) {
				Log("OpenXR Error: " + std::string(message) + " (" + std::to_string(result) + ")");
			} else {
				Log("OpenXR call failed with result " + std::to_string(result));
			}
		}

		bool CheckResult(XrResult result, const char* operation) const {
			if (XR_FAILED(result)) {
				fe::LogWarning("OpenXR %s failed: %d", operation, static_cast<int>(result));
				return false;
			}
			return true;
		}

		XrPath Path(const std::string& path) const {
			XrPath pathHandle = XR_NULL_PATH;
			if (!CheckResult(xrStringToPath(instance, path.c_str(), &pathHandle), "xrStringToPath"))
				return XR_NULL_PATH;
			return pathHandle;
		}

		void CreateAction(XrActionType type, const char* name, XrAction* action) {
			std::string localizedName(name);
			std::replace(localizedName.begin(), localizedName.end(), '_', ' ');

			XrActionCreateInfo actionInfo{XR_TYPE_ACTION_CREATE_INFO};
			actionInfo.actionType = type;
			std::snprintf(actionInfo.actionName, sizeof(actionInfo.actionName), "%s", name);
			std::snprintf(actionInfo.localizedActionName, sizeof(actionInfo.localizedActionName), "%s", localizedName.c_str());
			CheckResult(xrCreateAction(actionSet, &actionInfo, action), "xrCreateAction");
		}

		void SuggestProfileBindings(const char* profilePath, const std::vector<XrActionSuggestedBinding>& bindings) {
			XrPath profilePathHandle = XR_NULL_PATH;
			if (!CheckResult(xrStringToPath(instance, profilePath, &profilePathHandle), "xrStringToPath"))
				return;

			XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
			suggestedBindings.interactionProfile = profilePathHandle;
			suggestedBindings.suggestedBindings = bindings.data();
			suggestedBindings.countSuggestedBindings = static_cast<uint32_t>(bindings.size());
			CheckResult(xrSuggestInteractionProfileBindings(instance, &suggestedBindings), "xrSuggestInteractionProfileBindings");
		}

		void ApplyPendingHaptics() {
			if (session == XR_NULL_HANDLE)
				return;

			XrAction hapticActions[2] = {leftHapticAction, rightHapticAction};
			for (size_t i = 0; i < 2; ++i) {
				if (pendingHapticDuration[i] <= 0.0f || hapticActions[i] == XR_NULL_HANDLE)
					continue;

				XrHapticVibration vibration{XR_TYPE_HAPTIC_VIBRATION};
				vibration.amplitude = std::clamp(pendingHapticAmplitude[i], 0.0f, 1.0f);
				vibration.duration = static_cast<XrDuration>(static_cast<double>(pendingHapticDuration[i]) * 1.0e9);
				vibration.frequency = XR_FREQUENCY_UNSPECIFIED;

				XrHapticActionInfo hapticInfo{XR_TYPE_HAPTIC_ACTION_INFO};
				hapticInfo.action = hapticActions[i];
				hapticInfo.subactionPath = XR_NULL_PATH;

				CheckResult(xrApplyHapticFeedback(session, &hapticInfo,
					reinterpret_cast<const XrHapticBaseHeader*>(&vibration)), "xrApplyHapticFeedback");

				pendingHapticAmplitude[i] = 0.0f;
				pendingHapticDuration[i] = 0.0f;
			}
		}

		void CreateActions() {
			XrActionSetCreateInfo actionSetInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
			std::snprintf(actionSetInfo.actionSetName, sizeof(actionSetInfo.actionSetName), "%s", "gameplay");
			std::snprintf(actionSetInfo.localizedActionSetName, sizeof(actionSetInfo.localizedActionSetName), "%s", "Gameplay");
			if (!CheckResult(xrCreateActionSet(instance, &actionSetInfo, &actionSet), "xrCreateActionSet"))
				return;

			CreateAction(XR_ACTION_TYPE_VECTOR2F_INPUT, "move", &moveAction);
			CreateAction(XR_ACTION_TYPE_VECTOR2F_INPUT, "look", &lookAction);
			CreateAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "button_x", &buttonXAction);
			CreateAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "button_y", &buttonYAction);
			CreateAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "button_a", &buttonAAction);
			CreateAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "button_b", &buttonBAction);
			CreateAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "left_trigger", &leftTriggerAction);
			CreateAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "right_trigger", &rightTriggerAction);
			CreateAction(XR_ACTION_TYPE_POSE_INPUT, "left_grip_pose", &leftGripPoseAction);
			CreateAction(XR_ACTION_TYPE_POSE_INPUT, "left_aim_pose", &leftAimPoseAction);
			CreateAction(XR_ACTION_TYPE_POSE_INPUT, "right_grip_pose", &rightGripPoseAction);
			CreateAction(XR_ACTION_TYPE_POSE_INPUT, "right_aim_pose", &rightAimPoseAction);
			CreateAction(XR_ACTION_TYPE_VIBRATION_OUTPUT, "left_haptic", &leftHapticAction);
			CreateAction(XR_ACTION_TYPE_VIBRATION_OUTPUT, "right_haptic", &rightHapticAction);
			CreateAction(XR_ACTION_TYPE_FLOAT_INPUT, "left_grip", &leftGripAction);
			CreateAction(XR_ACTION_TYPE_FLOAT_INPUT, "right_grip", &rightGripAction);

			// Touch/Index/Pico expose the grip as an analog squeeze value while
			// Windows MR and Vive expose it as a boolean click. Each profile must
			// be suggested in a single call (later calls replace earlier ones).
			// faceButtons is true only for profiles with X/Y on the left and A/B
			// on the right; suggesting a path a profile does not define would
			// reject the whole binding set.
			auto suggestThumbstickProfile = [&](const char* profile, bool squeezeValue, bool faceButtons) {
				std::vector<XrActionSuggestedBinding> bindings = {
					{moveAction, Path("/user/hand/right/input/thumbstick")},
					{lookAction, Path("/user/hand/left/input/thumbstick")},
					{leftTriggerAction, Path("/user/hand/left/input/trigger")},
					{rightTriggerAction, Path("/user/hand/right/input/trigger")},
					{leftGripPoseAction, Path("/user/hand/left/input/grip/pose")},
					{leftAimPoseAction, Path("/user/hand/left/input/aim/pose")},
					{rightGripPoseAction, Path("/user/hand/right/input/grip/pose")},
					{rightAimPoseAction, Path("/user/hand/right/input/aim/pose")},
					{leftHapticAction, Path("/user/hand/left/output/haptic")},
					{rightHapticAction, Path("/user/hand/right/output/haptic")},
				};

				if (faceButtons) {
					bindings.push_back({buttonXAction, Path("/user/hand/left/input/x/click")});
					bindings.push_back({buttonYAction, Path("/user/hand/left/input/y/click")});
					bindings.push_back({buttonAAction, Path("/user/hand/right/input/a/click")});
					bindings.push_back({buttonBAction, Path("/user/hand/right/input/b/click")});
				}

				const char* squeezeComponent = squeezeValue ? "/input/squeeze/value" : "/input/squeeze/click";
				bindings.push_back({leftGripAction, Path("/user/hand/left" + std::string(squeezeComponent))});
				bindings.push_back({rightGripAction, Path("/user/hand/right" + std::string(squeezeComponent))});

				SuggestProfileBindings(profile, bindings);
			};

			suggestThumbstickProfile("/interaction_profiles/oculus/touch_controller", true, true);
			suggestThumbstickProfile("/interaction_profiles/microsoft/motion_controller", false, false);
			// Index maps its face buttons to a/b, so only triggers are bound there.
			suggestThumbstickProfile("/interaction_profiles/valve/index_controller", true, false);
			suggestThumbstickProfile("/interaction_profiles/bytedance/pico_touch_controller", true, true);
			suggestThumbstickProfile("/interaction_profiles/meta/touch_pro_controller", true, true);

			SuggestProfileBindings("/interaction_profiles/htc/vive_controller", {
				{moveAction, Path("/user/hand/right/input/trackpad")},
				{lookAction, Path("/user/hand/left/input/trackpad")},
				{leftTriggerAction, Path("/user/hand/left/input/trigger")},
				{rightTriggerAction, Path("/user/hand/right/input/trigger")},
				{leftGripAction, Path("/user/hand/left/input/squeeze/click")},
				{rightGripAction, Path("/user/hand/right/input/squeeze/click")},
				{leftGripPoseAction, Path("/user/hand/left/input/grip/pose")},
				{leftAimPoseAction, Path("/user/hand/left/input/aim/pose")},
				{rightGripPoseAction, Path("/user/hand/right/input/grip/pose")},
				{rightAimPoseAction, Path("/user/hand/right/input/aim/pose")},
				{leftHapticAction, Path("/user/hand/left/output/haptic")},
				{rightHapticAction, Path("/user/hand/right/output/haptic")},
			});

			XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
			attachInfo.actionSets = &actionSet;
			attachInfo.countActionSets = 1;
			if (!CheckResult(xrAttachSessionActionSets(session, &attachInfo), "xrAttachSessionActionSets"))
				return;

			auto createActionSpace = [&](XrAction action, XrSpace* space) {
				if (action == XR_NULL_HANDLE) return;
				XrActionSpaceCreateInfo spaceInfo{XR_TYPE_ACTION_SPACE_CREATE_INFO};
				spaceInfo.action = action;
				spaceInfo.subactionPath = XR_NULL_PATH;
				spaceInfo.poseInActionSpace.orientation = {0.0f, 0.0f, 0.0f, 1.0f};
				spaceInfo.poseInActionSpace.position = {0.0f, 0.0f, 0.0f};
				CheckResult(xrCreateActionSpace(session, &spaceInfo, space), "xrCreateActionSpace");
			};

			createActionSpace(leftGripPoseAction, &gripPoseSpace[0]);
			createActionSpace(leftAimPoseAction, &aimPoseSpace[0]);
			createActionSpace(rightGripPoseAction, &gripPoseSpace[1]);
			createActionSpace(rightAimPoseAction, &aimPoseSpace[1]);
		}

		bool ReadButton(XrAction action) {
			if (action == XR_NULL_HANDLE)
				return false;

			XrActionStateBoolean state{XR_TYPE_ACTION_STATE_BOOLEAN};
			XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			getInfo.action = action;
			if (!CheckResult(xrGetActionStateBoolean(session, &getInfo, &state), "xrGetActionStateBoolean"))
				return false;
			return state.isActive && state.changedSinceLastSync && state.currentState;
		}

		bool ReadButtonHeld(XrAction action) {
			if (action == XR_NULL_HANDLE)
				return false;

			XrActionStateBoolean state{XR_TYPE_ACTION_STATE_BOOLEAN};
			XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			getInfo.action = action;
			if (!CheckResult(xrGetActionStateBoolean(session, &getInfo, &state), "xrGetActionStateBoolean"))
				return false;
			return state.isActive && state.currentState;
		}

		float ReadFloat(XrAction action) {
			if (action == XR_NULL_HANDLE)
				return 0.0f;

			XrActionStateFloat state{XR_TYPE_ACTION_STATE_FLOAT};
			XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			getInfo.action = action;
			if (!CheckResult(xrGetActionStateFloat(session, &getInfo, &state), "xrGetActionStateFloat"))
				return 0.0f;
			return state.isActive ? state.currentState : 0.0f;
		}

		glm::vec3 LocateControllerPose(XrAction action, XrSpace space, XrTime displayTime, bool& active, glm::quat& orientation) {
			active = false;
			orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
			if (action == XR_NULL_HANDLE || space == XR_NULL_HANDLE)
				return glm::vec3(0.0f);

			XrActionStatePose poseState{XR_TYPE_ACTION_STATE_POSE};
			XrActionStateGetInfo poseGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			poseGetInfo.action = action;
			if (!CheckResult(xrGetActionStatePose(session, &poseGetInfo, &poseState), "xrGetActionStatePose") || !poseState.isActive)
				return glm::vec3(0.0f);

			XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
			if (!CheckResult(xrLocateSpace(space, appSpace, displayTime, &location), "xrLocateSpace"))
				return glm::vec3(0.0f);
			if (!(location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT))
				return glm::vec3(0.0f);

			if (location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) {
				const auto& o = location.pose.orientation;
				orientation = glm::quat(o.w, o.x, o.y, o.z);
			}

			active = true;
			return glm::vec3(location.pose.position.x, location.pose.position.y, location.pose.position.z);
		}

		void LoadPassthroughFunctions() {
			xrGetInstanceProcAddr(instance, "xrCreatePassthroughFB", reinterpret_cast<PFN_xrVoidFunction*>(&pfnCreatePassthroughFB));
			xrGetInstanceProcAddr(instance, "xrDestroyPassthroughFB", reinterpret_cast<PFN_xrVoidFunction*>(&pfnDestroyPassthroughFB));
			xrGetInstanceProcAddr(instance, "xrPassthroughStartFB", reinterpret_cast<PFN_xrVoidFunction*>(&pfnPassthroughStartFB));
			xrGetInstanceProcAddr(instance, "xrPassthroughPauseFB", reinterpret_cast<PFN_xrVoidFunction*>(&pfnPassthroughPauseFB));
			xrGetInstanceProcAddr(instance, "xrCreatePassthroughLayerFB", reinterpret_cast<PFN_xrVoidFunction*>(&pfnCreatePassthroughLayerFB));
			xrGetInstanceProcAddr(instance, "xrDestroyPassthroughLayerFB", reinterpret_cast<PFN_xrVoidFunction*>(&pfnDestroyPassthroughLayerFB));
			xrGetInstanceProcAddr(instance, "xrPassthroughLayerResumeFB", reinterpret_cast<PFN_xrVoidFunction*>(&pfnPassthroughLayerResumeFB));
			xrGetInstanceProcAddr(instance, "xrPassthroughLayerPauseFB", reinterpret_cast<PFN_xrVoidFunction*>(&pfnPassthroughLayerPauseFB));
		}

		void DestroyPassthrough() {
			passthroughEnabled = false;
			if (passthroughLayer != XR_NULL_HANDLE && pfnDestroyPassthroughLayerFB)
				OutputError(pfnDestroyPassthroughLayerFB(passthroughLayer));
			if (passthrough != XR_NULL_HANDLE && pfnDestroyPassthroughFB)
				OutputError(pfnDestroyPassthroughFB(passthrough));
			passthroughLayer = XR_NULL_HANDLE;
			passthrough = XR_NULL_HANDLE;
		}
#endif
	};

	OpenXR::OpenXR() : impl(std::make_unique<Impl>()) {}

	OpenXR::~OpenXR() {
		Shutdown();
	}

	bool OpenXR::Initialize(const GraphicsBinding& binding) {
#ifndef FE_EXCLUDE_OPENXR
		Shutdown();
		auto initializationFailed = [this]() {
			Shutdown();
			return false;
		};
		impl->useVulkan = binding.useVulkan;

#ifdef __ANDROID__
		PFN_xrInitializeLoaderKHR initializeLoader = nullptr;
		const XrResult loaderProcResult = xrGetInstanceProcAddr(
			XR_NULL_HANDLE, "xrInitializeLoaderKHR",
			reinterpret_cast<PFN_xrVoidFunction*>(&initializeLoader));
		if (XR_SUCCEEDED(loaderProcResult) && initializeLoader) {
			JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
			JavaVM* vm = nullptr;
			if (env)
				env->GetJavaVM(&vm);

			XrLoaderInitInfoAndroidKHR loaderInfo{XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
			loaderInfo.applicationVM = vm;
			loaderInfo.applicationContext = static_cast<jobject>(SDL_GetAndroidActivity());
			const XrResult result = initializeLoader(
				reinterpret_cast<const XrLoaderInitInfoBaseHeaderKHR*>(&loaderInfo));
			if (XR_FAILED(result)) {
				impl->OutputError(result);
				return initializationFailed();
			}
		}
#endif

		std::vector<const char*> enabledExtensions;
#ifdef __ANDROID__
		enabledExtensions.push_back(XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME);
		enabledExtensions.push_back(binding.useVulkan
			? XR_KHR_VULKAN_ENABLE_EXTENSION_NAME
			: XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME);
#else
		enabledExtensions.push_back(binding.useVulkan
			? XR_KHR_VULKAN_ENABLE_EXTENSION_NAME
			: XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);
#endif

		uint32_t extensionCount = 0;
		if (XR_SUCCEEDED(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr)) && extensionCount > 0) {
			std::vector<XrExtensionProperties> extensions(extensionCount, {XR_TYPE_EXTENSION_PROPERTIES});
			if (XR_SUCCEEDED(xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, extensions.data()))) {
				for (const auto& extension : extensions) {
					if (std::strcmp(extension.extensionName, XR_FB_PASSTHROUGH_EXTENSION_NAME) == 0) {
						impl->passthroughExtensionAvailable = true;
						enabledExtensions.push_back(XR_FB_PASSTHROUGH_EXTENSION_NAME);
						break;
					}
				}
			}
		}

		XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
		createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
		createInfo.enabledExtensionNames = enabledExtensions.data();
		createInfo.applicationInfo.apiVersion = XR_API_VERSION_1_0;
		createInfo.applicationInfo.applicationVersion = 1;
		createInfo.applicationInfo.engineVersion = 1;
		std::snprintf(createInfo.applicationInfo.engineName, sizeof(createInfo.applicationInfo.engineName), "%s", "FenixEngine");
		std::snprintf(createInfo.applicationInfo.applicationName, sizeof(createInfo.applicationInfo.applicationName), "%s", "Fenix Engine");

#ifdef __ANDROID__
		JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
		JavaVM* vm = nullptr;
		if (env)
			env->GetJavaVM(&vm);
		XrInstanceCreateInfoAndroidKHR androidCreateInfo{XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
		androidCreateInfo.applicationVM = vm;
		androidCreateInfo.applicationActivity = static_cast<jobject>(SDL_GetAndroidActivity());
		createInfo.next = &androidCreateInfo;
#endif

		if (!impl->CheckResult(xrCreateInstance(&createInfo, &impl->instance), "xrCreateInstance"))
			return initializationFailed();
		if (impl->passthroughExtensionAvailable)
			impl->LoadPassthroughFunctions();

		XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
		systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
		if (!impl->CheckResult(xrGetSystem(impl->instance, &systemInfo, &impl->systemId), "xrGetSystem"))
			return initializationFailed();

		XrSystemPassthroughPropertiesFB passthroughProperties{XR_TYPE_SYSTEM_PASSTHROUGH_PROPERTIES_FB};
		XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES};
		if (impl->passthroughExtensionAvailable)
			systemProperties.next = &passthroughProperties;
		if (!impl->CheckResult(xrGetSystemProperties(impl->instance, impl->systemId, &systemProperties), "xrGetSystemProperties"))
			return initializationFailed();
		impl->passthroughSupported = impl->passthroughExtensionAvailable &&
			passthroughProperties.supportsPassthrough == XR_TRUE;
		impl->Log("OpenXR system: " + std::string(systemProperties.systemName));

#ifdef XR_USE_GRAPHICS_API_VULKAN
		if (binding.useVulkan) {
			PFN_xrGetVulkanGraphicsRequirementsKHR getRequirements = nullptr;
			if (impl->CheckResult(xrGetInstanceProcAddr(impl->instance,
					"xrGetVulkanGraphicsRequirementsKHR",
					reinterpret_cast<PFN_xrVoidFunction*>(&getRequirements)),
					"xrGetInstanceProcAddr(xrGetVulkanGraphicsRequirementsKHR)") && getRequirements) {
				XrGraphicsRequirementsVulkanKHR requirements{XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR};
				if (!impl->CheckResult(getRequirements(impl->instance, impl->systemId, &requirements),
						"xrGetVulkanGraphicsRequirementsKHR"))
					return initializationFailed();
			}
		} else
#endif
		{
#ifdef __ANDROID__
			PFN_xrGetOpenGLESGraphicsRequirementsKHR getRequirements = nullptr;
			if (impl->CheckResult(xrGetInstanceProcAddr(impl->instance,
					"xrGetOpenGLESGraphicsRequirementsKHR",
					reinterpret_cast<PFN_xrVoidFunction*>(&getRequirements)),
					"xrGetInstanceProcAddr(xrGetOpenGLESGraphicsRequirementsKHR)") && getRequirements) {
				XrGraphicsRequirementsOpenGLESKHR requirements{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR};
				if (!impl->CheckResult(getRequirements(impl->instance, impl->systemId, &requirements),
						"xrGetOpenGLESGraphicsRequirementsKHR"))
					return initializationFailed();
			}
#else
			PFN_xrGetOpenGLGraphicsRequirementsKHR getRequirements = nullptr;
			if (impl->CheckResult(xrGetInstanceProcAddr(impl->instance,
					"xrGetOpenGLGraphicsRequirementsKHR",
					reinterpret_cast<PFN_xrVoidFunction*>(&getRequirements)),
					"xrGetInstanceProcAddr(xrGetOpenGLGraphicsRequirementsKHR)") && getRequirements) {
				XrGraphicsRequirementsOpenGLKHR requirements{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
				if (!impl->CheckResult(getRequirements(impl->instance, impl->systemId, &requirements),
						"xrGetOpenGLGraphicsRequirementsKHR"))
					return initializationFailed();
			}
#endif
		}

		XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
		sessionInfo.systemId = impl->systemId;
#ifdef XR_USE_GRAPHICS_API_VULKAN
		XrGraphicsBindingVulkanKHR vulkanBinding{XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR};
		if (binding.useVulkan) {
			vulkanBinding.instance = reinterpret_cast<VkInstance>(binding.instance);
			vulkanBinding.physicalDevice = reinterpret_cast<VkPhysicalDevice>(binding.physicalDevice);
			vulkanBinding.device = reinterpret_cast<VkDevice>(binding.device);
			vulkanBinding.queueFamilyIndex = binding.queueFamilyIndex;
			vulkanBinding.queueIndex = 0;

			PFN_xrGetVulkanGraphicsDeviceKHR getGraphicsDevice = nullptr;
			if (impl->CheckResult(xrGetInstanceProcAddr(impl->instance,
					"xrGetVulkanGraphicsDeviceKHR",
					reinterpret_cast<PFN_xrVoidFunction*>(&getGraphicsDevice)),
					"xrGetInstanceProcAddr(xrGetVulkanGraphicsDeviceKHR)") && getGraphicsDevice) {
				VkPhysicalDevice runtimeDevice = VK_NULL_HANDLE;
				if (XR_SUCCEEDED(getGraphicsDevice(impl->instance, impl->systemId,
						vulkanBinding.instance, &runtimeDevice)))
					vulkanBinding.physicalDevice = runtimeDevice;
			}
			sessionInfo.next = &vulkanBinding;
		} else
#endif
		{
#ifdef _WIN32
			XrGraphicsBindingOpenGLWin32KHR win32Binding{XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR};
			win32Binding.hDC = static_cast<HDC>(binding.display);
			win32Binding.hGLRC = static_cast<HGLRC>(binding.context);
			sessionInfo.next = &win32Binding;
#elif defined(__ANDROID__)
			XrGraphicsBindingOpenGLESAndroidKHR androidBinding{XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR};
			androidBinding.display = static_cast<EGLDisplay>(binding.display);
			androidBinding.config = static_cast<EGLConfig>(binding.config);
			androidBinding.context = static_cast<EGLContext>(binding.context);
			sessionInfo.next = &androidBinding;
#elif defined(__linux__)
			if (binding.windowSystem == GraphicsBinding::WindowSystem::Wayland) {
				XrGraphicsBindingOpenGLWaylandKHR waylandBinding{XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR};
				waylandBinding.display = static_cast<wl_display*>(binding.display);
				sessionInfo.next = &waylandBinding;
			} else {
				XrGraphicsBindingOpenGLXlibKHR xlibBinding{XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR};
				xlibBinding.xDisplay = static_cast<Display*>(binding.display);
				xlibBinding.glxDrawable = static_cast<GLXDrawable>(binding.drawable);
				xlibBinding.visualid = static_cast<VisualID>(binding.visualId);
				xlibBinding.glxFBConfig = reinterpret_cast<GLXFBConfig>(binding.framebufferConfig);
				xlibBinding.glxContext = reinterpret_cast<GLXContext>(binding.context);
				sessionInfo.next = &xlibBinding;
			}
#endif
		}

		if (!impl->CheckResult(xrCreateSession(impl->instance, &sessionInfo, &impl->session), "xrCreateSession"))
			return initializationFailed();

		XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
		spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
		spaceInfo.poseInReferenceSpace.orientation = {0.0f, 0.0f, 0.0f, 1.0f};
		if (!impl->CheckResult(xrCreateReferenceSpace(impl->session, &spaceInfo, &impl->appSpace), "xrCreateReferenceSpace"))
			return initializationFailed();

		impl->CreateActions();
		if (impl->actionSet == XR_NULL_HANDLE)
			return initializationFailed();
		return true;
#else
		(void)binding;
		return false;
#endif
	}

	bool OpenXR::IsInstanceValid() const {
#ifndef FE_EXCLUDE_OPENXR
		return impl->instance != XR_NULL_HANDLE;
#else
		return false;
#endif
	}

	bool OpenXR::IsSessionRunning() const {
#ifndef FE_EXCLUDE_OPENXR
		return impl->sessionRunning;
#else
		return false;
#endif
	}

	bool OpenXR::IsPassthroughSupported() const {
#ifndef FE_EXCLUDE_OPENXR
		return impl->passthroughSupported;
#else
		return false;
#endif
	}

	bool OpenXR::IsPassthroughEnabled() const {
#ifndef FE_EXCLUDE_OPENXR
		return impl->passthroughEnabled;
#else
		return false;
#endif
	}

	void OpenXR::SetPassthroughEnabled(bool enabled) {
#ifndef FE_EXCLUDE_OPENXR
		if (enabled == impl->passthroughEnabled || !impl->passthroughSupported)
			return;
		if (enabled) {
			if (impl->passthrough == XR_NULL_HANDLE && impl->pfnCreatePassthroughFB) {
				XrPassthroughCreateInfoFB info{XR_TYPE_PASSTHROUGH_CREATE_INFO_FB};
				info.flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB;
				impl->OutputError(impl->pfnCreatePassthroughFB(impl->session, &info, &impl->passthrough));
			}
			if (impl->passthrough == XR_NULL_HANDLE)
				return;
			if (impl->passthroughLayer == XR_NULL_HANDLE && impl->pfnCreatePassthroughLayerFB) {
				XrPassthroughLayerCreateInfoFB info{XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB};
				info.passthrough = impl->passthrough;
				info.flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB;
				info.purpose = XR_PASSTHROUGH_LAYER_PURPOSE_RECONSTRUCTION_FB;
				impl->OutputError(impl->pfnCreatePassthroughLayerFB(impl->session, &info, &impl->passthroughLayer));
			}
			if (impl->passthroughLayer == XR_NULL_HANDLE)
				return;
			if (impl->pfnPassthroughStartFB)
				impl->OutputError(impl->pfnPassthroughStartFB(impl->passthrough));
			if (impl->pfnPassthroughLayerResumeFB)
				impl->OutputError(impl->pfnPassthroughLayerResumeFB(impl->passthroughLayer));
			impl->passthroughEnabled = true;
		} else {
			if (impl->pfnPassthroughLayerPauseFB && impl->passthroughLayer != XR_NULL_HANDLE)
				impl->OutputError(impl->pfnPassthroughLayerPauseFB(impl->passthroughLayer));
			if (impl->pfnPassthroughPauseFB && impl->passthrough != XR_NULL_HANDLE)
				impl->OutputError(impl->pfnPassthroughPauseFB(impl->passthrough));
			impl->passthroughEnabled = false;
		}
#else
		(void)enabled;
#endif
	}

	void OpenXR::PollEvents() {
#ifndef FE_EXCLUDE_OPENXR
		if (impl->instance == XR_NULL_HANDLE)
			return;
		XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
		while (xrPollEvent(impl->instance, &event) == XR_SUCCESS) {
			if (event.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
				const auto& changed = *reinterpret_cast<const XrEventDataSessionStateChanged*>(&event);
				impl->sessionState = changed.state;
				switch (changed.state) {
					case XR_SESSION_STATE_READY: {
						XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
						beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
						impl->sessionRunning = impl->CheckResult(
							xrBeginSession(impl->session, &beginInfo), "xrBeginSession");
						break;
					}
					case XR_SESSION_STATE_STOPPING:
						impl->CheckResult(xrEndSession(impl->session), "xrEndSession");
						impl->sessionRunning = false;
						break;
					case XR_SESSION_STATE_EXITING:
					case XR_SESSION_STATE_LOSS_PENDING:
						impl->sessionRunning = false;
						break;
					default:
						break;
				}
			}
			event = {XR_TYPE_EVENT_DATA_BUFFER};
		}
#endif
	}

	bool OpenXR::CreateSwapchain(int64_t preferredFormat, SwapchainInfo& info) {
#ifndef FE_EXCLUDE_OPENXR
		DestroySwapchain();
		if (impl->session == XR_NULL_HANDLE)
			return false;

		uint32_t viewCount = 0;
		if (!impl->CheckResult(xrEnumerateViewConfigurationViews(impl->instance,
				impl->systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
				0, &viewCount, nullptr), "xrEnumerateViewConfigurationViews") || viewCount == 0)
			return false;
		std::vector<XrViewConfigurationView> configViews(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
		if (!impl->CheckResult(xrEnumerateViewConfigurationViews(impl->instance,
				impl->systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
				viewCount, &viewCount, configViews.data()), "xrEnumerateViewConfigurationViews"))
			return false;

		uint32_t formatCount = 0;
		if (!impl->CheckResult(xrEnumerateSwapchainFormats(impl->session, 0,
				&formatCount, nullptr), "xrEnumerateSwapchainFormats") || formatCount == 0)
			return false;
		std::vector<int64_t> formats(formatCount);
		if (!impl->CheckResult(xrEnumerateSwapchainFormats(impl->session, formatCount,
				&formatCount, formats.data()), "xrEnumerateSwapchainFormats"))
			return false;
		int64_t chosenFormat = formats.front();
		for (int64_t format : formats) {
			if (format == preferredFormat) {
				chosenFormat = format;
				break;
			}
		}

		impl->viewCount = viewCount;
		impl->swapchainWidth = static_cast<uint32_t>(configViews.front().recommendedImageRectWidth);
		impl->swapchainHeight = static_cast<uint32_t>(configViews.front().recommendedImageRectHeight);
		XrSwapchainCreateInfo swapchainInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
		swapchainInfo.arraySize = viewCount;
		swapchainInfo.format = chosenFormat;
		swapchainInfo.width = impl->swapchainWidth;
		swapchainInfo.height = impl->swapchainHeight;
		swapchainInfo.mipCount = 1;
		swapchainInfo.faceCount = 1;
		swapchainInfo.sampleCount = 1;
		swapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
			XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_SRC_BIT;
		if (!impl->CheckResult(xrCreateSwapchain(impl->session, &swapchainInfo,
				&impl->swapchain), "xrCreateSwapchain"))
			return false;

		uint32_t imageCount = 0;
		if (!impl->CheckResult(xrEnumerateSwapchainImages(impl->swapchain, 0,
				&imageCount, nullptr), "xrEnumerateSwapchainImages"))
			return false;
		info.images.clear();
		info.images.reserve(imageCount);
		if (impl->useVulkan) {
#ifdef XR_USE_GRAPHICS_API_VULKAN
			std::vector<XrSwapchainImageVulkanKHR> images(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR});
			if (!impl->CheckResult(xrEnumerateSwapchainImages(impl->swapchain, imageCount,
					&imageCount, reinterpret_cast<XrSwapchainImageBaseHeader*>(images.data())),
					"xrEnumerateSwapchainImages"))
				return false;
			for (const auto& image : images)
				info.images.push_back(static_cast<uint64_t>(reinterpret_cast<uintptr_t>(image.image)));
#endif
		} else {
#ifdef __ANDROID__
			std::vector<XrSwapchainImageOpenGLESKHR> images(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR});
#else
			std::vector<XrSwapchainImageOpenGLKHR> images(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR});
#endif
			if (!impl->CheckResult(xrEnumerateSwapchainImages(impl->swapchain, imageCount,
					&imageCount, reinterpret_cast<XrSwapchainImageBaseHeader*>(images.data())),
					"xrEnumerateSwapchainImages"))
				return false;
			for (const auto& image : images)
				info.images.push_back(static_cast<uint64_t>(image.image));
		}

		info.width = impl->swapchainWidth;
		info.height = impl->swapchainHeight;
		info.viewCount = impl->viewCount;
		info.format = chosenFormat;
		return !info.images.empty();
#else
		(void)preferredFormat;
		(void)info;
		return false;
#endif
	}

	void OpenXR::DestroySwapchain() {
#ifndef FE_EXCLUDE_OPENXR
		if (impl->swapchain != XR_NULL_HANDLE) {
			impl->OutputError(xrDestroySwapchain(impl->swapchain));
			impl->swapchain = XR_NULL_HANDLE;
		}
		impl->viewCount = 0;
		impl->swapchainWidth = 0;
		impl->swapchainHeight = 0;
#endif
	}

	bool OpenXR::BeginFrame(uint32_t& imageIndex, std::vector<View>& views) {
#ifndef FE_EXCLUDE_OPENXR
		views.clear();
		impl->locatedViews.clear();
		if (!impl->sessionRunning || impl->session == XR_NULL_HANDLE || impl->swapchain == XR_NULL_HANDLE)
			return false;
		if (!impl->CheckResult(xrWaitFrame(impl->session, &impl->waitInfo, &impl->frameState), "xrWaitFrame"))
			return false;
		UpdateInput(static_cast<int64_t>(impl->frameState.predictedDisplayTime));
		if (!impl->CheckResult(xrBeginFrame(impl->session, &impl->frameBegin), "xrBeginFrame"))
			return false;
		impl->frameBegun = true;
		if (!impl->frameState.shouldRender) {
			EndFrame();
			return false;
		}

		if (!impl->CheckResult(xrAcquireSwapchainImage(impl->swapchain, &impl->acquireInfo,
				&imageIndex), "xrAcquireSwapchainImage")) {
			EndFrame();
			return false;
		}
		impl->imageAcquired = true;
		XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
		waitInfo.timeout = XR_INFINITE_DURATION;
		if (!impl->CheckResult(xrWaitSwapchainImage(impl->swapchain, &waitInfo), "xrWaitSwapchainImage")) {
			EndFrame();
			return false;
		}

		XrViewLocateInfo locateInfo{XR_TYPE_VIEW_LOCATE_INFO};
		locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
		locateInfo.displayTime = impl->frameState.predictedDisplayTime;
		locateInfo.space = impl->appSpace;
		XrViewState viewState{XR_TYPE_VIEW_STATE};
		impl->locatedViews.assign(impl->viewCount, {XR_TYPE_VIEW});
		uint32_t locatedViewCount = 0;
		if (!impl->CheckResult(xrLocateViews(impl->session, &locateInfo, &viewState,
				impl->viewCount, &locatedViewCount, impl->locatedViews.data()), "xrLocateViews")) {
			EndFrame();
			return false;
		}
		impl->locatedViews.resize(locatedViewCount);
		views.reserve(locatedViewCount);
		for (const XrView& xrView : impl->locatedViews) {
			View view;
			view.position = glm::vec3(xrView.pose.position.x, xrView.pose.position.y, xrView.pose.position.z);
			view.orientation = glm::quat(xrView.pose.orientation.w, xrView.pose.orientation.x,
				xrView.pose.orientation.y, xrView.pose.orientation.z);
			view.fov = glm::vec4(xrView.fov.angleLeft, xrView.fov.angleRight,
				xrView.fov.angleDown, xrView.fov.angleUp);
			views.push_back(view);
		}
		return true;
#else
		(void)imageIndex;
		(void)views;
		return false;
#endif
	}

	void OpenXR::EndFrame() {
#ifndef FE_EXCLUDE_OPENXR
		if (!impl->frameBegun)
			return;
		if (impl->imageAcquired) {
			XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
			impl->OutputError(xrReleaseSwapchainImage(impl->swapchain, &releaseInfo));
			impl->imageAcquired = false;
		}

		std::vector<XrCompositionLayerProjectionView> projectionViews;
		projectionViews.reserve(impl->locatedViews.size());
		for (const XrView& view : impl->locatedViews) {
			XrCompositionLayerProjectionView projectionView{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
			projectionView.pose = view.pose;
			projectionView.fov = view.fov;
			projectionView.subImage.swapchain = impl->swapchain;
			projectionView.subImage.imageRect.offset = {0, 0};
			projectionView.subImage.imageRect.extent = {
				static_cast<int32_t>(impl->swapchainWidth),
				static_cast<int32_t>(impl->swapchainHeight)};
			projectionView.subImage.imageArrayIndex = static_cast<uint32_t>(projectionViews.size());
			projectionViews.push_back(projectionView);
		}

		XrCompositionLayerProjection projectionLayer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
		projectionLayer.space = impl->appSpace;
		projectionLayer.viewCount = static_cast<uint32_t>(projectionViews.size());
		projectionLayer.views = projectionViews.data();
		XrCompositionLayerPassthroughFB passthroughLayer{XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB};
		passthroughLayer.flags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
		passthroughLayer.space = XR_NULL_HANDLE;
		passthroughLayer.layerHandle = impl->passthroughLayer;

		const XrCompositionLayerBaseHeader* layers[2];
		uint32_t layerCount = 0;
		if (impl->passthroughEnabled && impl->passthroughLayer != XR_NULL_HANDLE)
			layers[layerCount++] = reinterpret_cast<const XrCompositionLayerBaseHeader*>(&passthroughLayer);
		if (!projectionViews.empty())
			layers[layerCount++] = reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projectionLayer);

		XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
		endInfo.displayTime = impl->frameState.predictedDisplayTime;
		endInfo.environmentBlendMode = impl->passthroughEnabled
			? XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND : XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
		endInfo.layerCount = layerCount;
		endInfo.layers = layerCount ? layers : nullptr;
		impl->OutputError(xrEndFrame(impl->session, &endInfo));
		impl->frameBegun = false;
		impl->locatedViews.clear();
#endif
	}

	void OpenXR::RequestExitSession() {
#ifndef FE_EXCLUDE_OPENXR
		if (impl->session != XR_NULL_HANDLE)
			impl->OutputError(xrRequestExitSession(impl->session));
#endif
	}

	void OpenXR::Shutdown() {
#ifndef FE_EXCLUDE_OPENXR
		DestroyInput();
		impl->DestroyPassthrough();
		DestroySwapchain();
		if (impl->appSpace != XR_NULL_HANDLE) {
			impl->OutputError(xrDestroySpace(impl->appSpace));
			impl->appSpace = XR_NULL_HANDLE;
		}
		if (impl->session != XR_NULL_HANDLE) {
			impl->OutputError(xrDestroySession(impl->session));
			impl->session = XR_NULL_HANDLE;
		}
		if (impl->instance != XR_NULL_HANDLE) {
			impl->OutputError(xrDestroyInstance(impl->instance));
			impl->instance = XR_NULL_HANDLE;
		}
		impl->systemId = XR_NULL_SYSTEM_ID;
		impl->sessionState = XR_SESSION_STATE_UNKNOWN;
		impl->sessionRunning = false;
		impl->passthroughSupported = false;
		impl->passthroughExtensionAvailable = false;
		impl->frameBegun = false;
		impl->imageAcquired = false;
#endif
	}

	void OpenXR::UpdateInput(int64_t predictedDisplayTime) {
#ifndef FE_EXCLUDE_OPENXR
		if (impl->session == XR_NULL_HANDLE || impl->actionSet == XR_NULL_HANDLE)
			return;
		const XrTime displayTime = static_cast<XrTime>(predictedDisplayTime);

		XrActiveActionSet activeActionSet{impl->actionSet, XR_NULL_PATH};
		XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
		syncInfo.activeActionSets = &activeActionSet;
		syncInfo.countActiveActionSets = 1;
		if (!impl->CheckResult(xrSyncActions(impl->session, &syncInfo), "xrSyncActions"))
			return;

		impl->ApplyPendingHaptics();

		XrActionStateVector2f moveState{XR_TYPE_ACTION_STATE_VECTOR2F};
		XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
		getInfo.action = impl->moveAction;
		if (impl->CheckResult(xrGetActionStateVector2f(impl->session, &getInfo, &moveState), "xrGetActionStateVector2f") && moveState.isActive)
			impl->inputState.moveInput = glm::vec2(moveState.currentState.x, moveState.currentState.y);
		else
			impl->inputState.moveInput = glm::vec2(0.0f);

		XrActionStateVector2f lookState{XR_TYPE_ACTION_STATE_VECTOR2F};
		getInfo.action = impl->lookAction;
		if (impl->CheckResult(xrGetActionStateVector2f(impl->session, &getInfo, &lookState), "xrGetActionStateVector2f") && lookState.isActive)
			impl->inputState.lookInput = glm::vec2(lookState.currentState.x, lookState.currentState.y);
		else
			impl->inputState.lookInput = glm::vec2(0.0f);

		impl->buttonPressed[static_cast<size_t>(Button::X)] |= impl->ReadButton(impl->buttonXAction);
		impl->buttonPressed[static_cast<size_t>(Button::Y)] |= impl->ReadButton(impl->buttonYAction);
		impl->buttonPressed[static_cast<size_t>(Button::A)] |= impl->ReadButton(impl->buttonAAction);
		impl->buttonPressed[static_cast<size_t>(Button::B)] |= impl->ReadButton(impl->buttonBAction);
		impl->buttonPressed[static_cast<size_t>(Button::LeftTrigger)] |= impl->ReadButton(impl->leftTriggerAction);
		impl->buttonPressed[static_cast<size_t>(Button::RightTrigger)] |= impl->ReadButton(impl->rightTriggerAction);

		impl->buttonHeld[static_cast<size_t>(Button::X)] = impl->ReadButtonHeld(impl->buttonXAction);
		impl->buttonHeld[static_cast<size_t>(Button::Y)] = impl->ReadButtonHeld(impl->buttonYAction);
		impl->buttonHeld[static_cast<size_t>(Button::A)] = impl->ReadButtonHeld(impl->buttonAAction);
		impl->buttonHeld[static_cast<size_t>(Button::B)] = impl->ReadButtonHeld(impl->buttonBAction);
		impl->buttonHeld[static_cast<size_t>(Button::LeftTrigger)] = impl->ReadButtonHeld(impl->leftTriggerAction);
		impl->buttonHeld[static_cast<size_t>(Button::RightTrigger)] = impl->ReadButtonHeld(impl->rightTriggerAction);

		XrViewLocateInfo locateInfo{XR_TYPE_VIEW_LOCATE_INFO};
		locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
		locateInfo.displayTime = displayTime;
		locateInfo.space = impl->appSpace;
		XrViewState locateViewState{XR_TYPE_VIEW_STATE};
		uint32_t hmdViewCount = 0;
		std::vector<XrView> hmdViews(2, {XR_TYPE_VIEW});
		if (impl->CheckResult(xrLocateViews(impl->session, &locateInfo, &locateViewState, 2, &hmdViewCount, hmdViews.data()), "xrLocateViews") && hmdViewCount > 0) {
			if (locateViewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) {
				const auto& orientation = hmdViews[0].pose.orientation;
				impl->inputState.headOrientation = glm::quat(orientation.w, orientation.x, orientation.y, orientation.z);
			}
			if (locateViewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) {
				const auto& position = hmdViews[0].pose.position;
				impl->inputState.headPosition = glm::vec3(position.x, position.y, position.z);
			}
		}

		auto& state = impl->inputState;
		state.leftGripPosition = impl->LocateControllerPose(impl->leftGripPoseAction, impl->gripPoseSpace[0], displayTime, state.leftGripPoseActive, state.leftGripOrientation);
		state.leftAimPosition = impl->LocateControllerPose(impl->leftAimPoseAction, impl->aimPoseSpace[0], displayTime, state.leftAimPoseActive, state.leftAimOrientation);
		state.rightGripPosition = impl->LocateControllerPose(impl->rightGripPoseAction, impl->gripPoseSpace[1], displayTime, state.rightGripPoseActive, state.rightGripOrientation);
		state.rightAimPosition = impl->LocateControllerPose(impl->rightAimPoseAction, impl->aimPoseSpace[1], displayTime, state.rightAimPoseActive, state.rightAimOrientation);

		// Analog squeeze (Touch/Index/Pico) is converted to the same pressed
		// state as the boolean squeeze click (Windows MR/Vive).
		state.leftGripPressed = impl->ReadFloat(impl->leftGripAction) > 0.5f;
		state.rightGripPressed = impl->ReadFloat(impl->rightGripAction) > 0.5f;

		const bool grips[2] = {state.leftGripPressed, state.rightGripPressed};
		for (size_t i = 0; i < 2; i++) {
			if (grips[i] && !impl->prevGripPressed[i])
				impl->gripPressedEdge[i] = true;
			impl->prevGripPressed[i] = grips[i];
		}
#else
		(void)predictedDisplayTime;
#endif
	}

#ifndef FE_EXCLUDE_OPENXR
	void OpenXR::DestroyInput() {
		for (XrSpace& space : impl->gripPoseSpace) {
			if (space != XR_NULL_HANDLE) xrDestroySpace(space);
			space = XR_NULL_HANDLE;
		}
		for (XrSpace& space : impl->aimPoseSpace) {
			if (space != XR_NULL_HANDLE) xrDestroySpace(space);
			space = XR_NULL_HANDLE;
		}
		if (impl->actionSet != XR_NULL_HANDLE) xrDestroyActionSet(impl->actionSet);
		impl->actionSet = XR_NULL_HANDLE;
		impl->moveAction = XR_NULL_HANDLE;
		impl->lookAction = XR_NULL_HANDLE;
		impl->leftGripPoseAction = XR_NULL_HANDLE;
		impl->leftAimPoseAction = XR_NULL_HANDLE;
		impl->rightGripPoseAction = XR_NULL_HANDLE;
		impl->rightAimPoseAction = XR_NULL_HANDLE;
		impl->buttonXAction = XR_NULL_HANDLE;
		impl->buttonYAction = XR_NULL_HANDLE;
		impl->buttonAAction = XR_NULL_HANDLE;
		impl->buttonBAction = XR_NULL_HANDLE;
		impl->leftTriggerAction = XR_NULL_HANDLE;
		impl->rightTriggerAction = XR_NULL_HANDLE;
		impl->leftHapticAction = XR_NULL_HANDLE;
		impl->rightHapticAction = XR_NULL_HANDLE;
		impl->leftGripAction = XR_NULL_HANDLE;
		impl->rightGripAction = XR_NULL_HANDLE;
		impl->pendingHapticAmplitude[0] = 0.0f;
		impl->pendingHapticAmplitude[1] = 0.0f;
		impl->pendingHapticDuration[0] = 0.0f;
		impl->pendingHapticDuration[1] = 0.0f;
		impl->inputState = InputState{};
		std::fill(std::begin(impl->buttonPressed), std::end(impl->buttonPressed), false);
		std::fill(std::begin(impl->buttonHeld), std::end(impl->buttonHeld), false);
		impl->gripPressedEdge[0] = impl->gripPressedEdge[1] = false;
		impl->prevGripPressed[0] = impl->prevGripPressed[1] = false;
	}
#endif

	const OpenXR::InputState& OpenXR::GetInputState() const {
		return impl->inputState;
	}

	OpenXR::InputState OpenXR::GetWorldInputState(const glm::vec3& worldOrigin, const glm::quat& worldRotation) const {
		InputState state = impl->inputState;
		state.headPosition = worldOrigin + worldRotation * state.headPosition;
		state.headOrientation = worldRotation * state.headOrientation;

		auto transformPose = [&](bool active, glm::vec3& position, glm::quat& orientation) {
			if (active) {
				position = worldOrigin + worldRotation * position;
				orientation = worldRotation * orientation;
			} else {
				position = glm::vec3(0.0f);
				orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
			}
		};

		transformPose(state.leftGripPoseActive, state.leftGripPosition, state.leftGripOrientation);
		transformPose(state.leftAimPoseActive, state.leftAimPosition, state.leftAimOrientation);
		transformPose(state.rightGripPoseActive, state.rightGripPosition, state.rightGripOrientation);
		transformPose(state.rightAimPoseActive, state.rightAimPosition, state.rightAimOrientation);
		return state;
	}

	glm::vec3 OpenXR::GetControllerPosition(Hand hand, ControllerPose pose) const {
		const InputState& state = impl->inputState;
		if (hand == Hand::Left)
			return pose == ControllerPose::Grip ? state.leftGripPosition : state.leftAimPosition;
		return pose == ControllerPose::Grip ? state.rightGripPosition : state.rightAimPosition;
	}

	bool OpenXR::IsControllerPoseActive(Hand hand, ControllerPose pose) const {
		const InputState& state = impl->inputState;
		if (hand == Hand::Left)
			return pose == ControllerPose::Grip ? state.leftGripPoseActive : state.leftAimPoseActive;
		return pose == ControllerPose::Grip ? state.rightGripPoseActive : state.rightAimPoseActive;
	}

	bool OpenXR::WasButtonPressed(Button button) const {
		const size_t index = static_cast<size_t>(button);
		return index < std::size(impl->buttonPressed) && impl->buttonPressed[index];
	}

	bool OpenXR::ConsumeButtonPress(Button button) {
		const size_t index = static_cast<size_t>(button);
		if (index >= std::size(impl->buttonPressed))
			return false;
		bool pressed = impl->buttonPressed[index];
		impl->buttonPressed[index] = false;
		return pressed;
	}

	bool OpenXR::IsButtonHeld(Button button) const {
		const size_t index = static_cast<size_t>(button);
		return index < std::size(impl->buttonHeld) && impl->buttonHeld[index];
	}

	bool OpenXR::IsGripPressed(Hand hand) const {
		return hand == Hand::Left ? impl->inputState.leftGripPressed : impl->inputState.rightGripPressed;
	}

	bool OpenXR::ConsumeGripPress(Hand hand) {
		const size_t index = hand == Hand::Left ? 0 : 1;
		const bool pressed = impl->gripPressedEdge[index];
		impl->gripPressedEdge[index] = false;
		return pressed;
	}

	void OpenXR::TriggerHaptic(Hand hand, float amplitude, float durationSeconds) {
#ifndef FE_EXCLUDE_OPENXR
		if (impl->session == XR_NULL_HANDLE || durationSeconds <= 0.0f)
			return;

		const size_t index = hand == Hand::Left ? 0 : 1;
		impl->pendingHapticAmplitude[index] = std::clamp(amplitude, 0.0f, 1.0f);
		impl->pendingHapticDuration[index] = durationSeconds;
#else
		(void)hand;
		(void)amplitude;
		(void)durationSeconds;
#endif
	}

}