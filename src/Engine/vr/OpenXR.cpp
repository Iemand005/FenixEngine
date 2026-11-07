#include "OpenXR.hpp"

#include "../Log.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

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
#endif
	};

	OpenXR::OpenXR() : impl(std::make_unique<Impl>()) {}

	OpenXR::~OpenXR() {
#ifndef FE_EXCLUDE_OPENXR
		DestroyInput();
#endif
	}

#ifndef FE_EXCLUDE_OPENXR
	bool OpenXR::InitializeInput(XrInstance instance, XrSession session, XrSpace appSpace) {
		DestroyInput();
		impl->instance = instance;
		impl->session = session;
		impl->appSpace = appSpace;
		if (instance == XR_NULL_HANDLE || session == XR_NULL_HANDLE || appSpace == XR_NULL_HANDLE)
			return false;

		impl->CreateActions();
		return impl->actionSet != XR_NULL_HANDLE;
	}

	void OpenXR::UpdateInput(XrTime predictedDisplayTime) {
		if (impl->session == XR_NULL_HANDLE || impl->actionSet == XR_NULL_HANDLE)
			return;

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
		locateInfo.displayTime = predictedDisplayTime;
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
		state.leftGripPosition = impl->LocateControllerPose(impl->leftGripPoseAction, impl->gripPoseSpace[0], predictedDisplayTime, state.leftGripPoseActive, state.leftGripOrientation);
		state.leftAimPosition = impl->LocateControllerPose(impl->leftAimPoseAction, impl->aimPoseSpace[0], predictedDisplayTime, state.leftAimPoseActive, state.leftAimOrientation);
		state.rightGripPosition = impl->LocateControllerPose(impl->rightGripPoseAction, impl->gripPoseSpace[1], predictedDisplayTime, state.rightGripPoseActive, state.rightGripOrientation);
		state.rightAimPosition = impl->LocateControllerPose(impl->rightAimPoseAction, impl->aimPoseSpace[1], predictedDisplayTime, state.rightAimPoseActive, state.rightAimOrientation);

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
	}

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
		impl->instance = XR_NULL_HANDLE;
		impl->session = XR_NULL_HANDLE;
		impl->appSpace = XR_NULL_HANDLE;
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