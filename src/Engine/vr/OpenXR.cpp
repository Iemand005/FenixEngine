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
		bool buttonPressed[5] = {};

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
		XrAction toggleXrAction = XR_NULL_HANDLE;
		XrAction breakBlockAction = XR_NULL_HANDLE;
		XrAction placeBlockAction = XR_NULL_HANDLE;
		XrAction prevBlockAction = XR_NULL_HANDLE;
		XrAction nextBlockAction = XR_NULL_HANDLE;

		XrSpace gripPoseSpace[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE};
		XrSpace aimPoseSpace[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE};

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

		void CreateActions() {
			XrActionSetCreateInfo actionSetInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
			std::snprintf(actionSetInfo.actionSetName, sizeof(actionSetInfo.actionSetName), "%s", "gameplay");
			std::snprintf(actionSetInfo.localizedActionSetName, sizeof(actionSetInfo.localizedActionSetName), "%s", "Gameplay");
			if (!CheckResult(xrCreateActionSet(instance, &actionSetInfo, &actionSet), "xrCreateActionSet"))
				return;

			CreateAction(XR_ACTION_TYPE_VECTOR2F_INPUT, "move", &moveAction);
			CreateAction(XR_ACTION_TYPE_VECTOR2F_INPUT, "look", &lookAction);
			CreateAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "toggle_xr", &toggleXrAction);
			CreateAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "break_block", &breakBlockAction);
			CreateAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "place_block", &placeBlockAction);
			CreateAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "prev_block", &prevBlockAction);
			CreateAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "next_block", &nextBlockAction);
			CreateAction(XR_ACTION_TYPE_POSE_INPUT, "left_grip_pose", &leftGripPoseAction);
			CreateAction(XR_ACTION_TYPE_POSE_INPUT, "left_aim_pose", &leftAimPoseAction);
			CreateAction(XR_ACTION_TYPE_POSE_INPUT, "right_grip_pose", &rightGripPoseAction);
			CreateAction(XR_ACTION_TYPE_POSE_INPUT, "right_aim_pose", &rightAimPoseAction);

			const char* thumbstickProfiles[] = {
				"/interaction_profiles/oculus/touch_controller",
				"/interaction_profiles/microsoft/motion_controller",
				"/interaction_profiles/valve/index_controller",
				"/interaction_profiles/bytedance/pico_touch_controller",
				"/interaction_profiles/meta/touch_pro_controller",
			};

			for (const char* profile : thumbstickProfiles) {
				SuggestProfileBindings(profile, {
					{moveAction, Path("/user/hand/right/input/thumbstick")},
					{lookAction, Path("/user/hand/left/input/thumbstick")},
					{toggleXrAction, Path("/user/hand/left/input/x/click")},
					{toggleXrAction, Path("/user/hand/right/input/a/click")},
					{prevBlockAction, Path("/user/hand/left/input/trigger")},
					{nextBlockAction, Path("/user/hand/right/input/trigger")},
					{breakBlockAction, Path("/user/hand/left/input/squeeze")},
					{placeBlockAction, Path("/user/hand/right/input/squeeze")},
					{leftGripPoseAction, Path("/user/hand/left/input/grip/pose")},
					{leftAimPoseAction, Path("/user/hand/left/input/aim/pose")},
					{rightGripPoseAction, Path("/user/hand/right/input/grip/pose")},
					{rightAimPoseAction, Path("/user/hand/right/input/aim/pose")},
				});
			}

			SuggestProfileBindings("/interaction_profiles/htc/vive_controller", {
				{moveAction, Path("/user/hand/right/input/trackpad")},
				{lookAction, Path("/user/hand/left/input/trackpad")},
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

		glm::vec3 LocateControllerPose(XrAction action, XrSpace space, XrTime displayTime, bool& active) {
			active = false;
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

		impl->buttonPressed[static_cast<size_t>(Button::ToggleXR)] |= impl->ReadButton(impl->toggleXrAction);
		impl->buttonPressed[static_cast<size_t>(Button::BreakBlock)] |= impl->ReadButton(impl->breakBlockAction);
		impl->buttonPressed[static_cast<size_t>(Button::PlaceBlock)] |= impl->ReadButton(impl->placeBlockAction);
		impl->buttonPressed[static_cast<size_t>(Button::PreviousBlock)] |= impl->ReadButton(impl->prevBlockAction);
		impl->buttonPressed[static_cast<size_t>(Button::NextBlock)] |= impl->ReadButton(impl->nextBlockAction);

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
		state.leftGripPosition = impl->LocateControllerPose(impl->leftGripPoseAction, impl->gripPoseSpace[0], predictedDisplayTime, state.leftGripPoseActive);
		state.leftAimPosition = impl->LocateControllerPose(impl->leftAimPoseAction, impl->aimPoseSpace[0], predictedDisplayTime, state.leftAimPoseActive);
		state.rightGripPosition = impl->LocateControllerPose(impl->rightGripPoseAction, impl->gripPoseSpace[1], predictedDisplayTime, state.rightGripPoseActive);
		state.rightAimPosition = impl->LocateControllerPose(impl->rightAimPoseAction, impl->aimPoseSpace[1], predictedDisplayTime, state.rightAimPoseActive);
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
		impl->toggleXrAction = XR_NULL_HANDLE;
		impl->breakBlockAction = XR_NULL_HANDLE;
		impl->placeBlockAction = XR_NULL_HANDLE;
		impl->prevBlockAction = XR_NULL_HANDLE;
		impl->nextBlockAction = XR_NULL_HANDLE;
		impl->instance = XR_NULL_HANDLE;
		impl->session = XR_NULL_HANDLE;
		impl->appSpace = XR_NULL_HANDLE;
		impl->inputState = InputState{};
		std::fill(std::begin(impl->buttonPressed), std::end(impl->buttonPressed), false);
	}
#endif

	const OpenXR::InputState& OpenXR::GetInputState() const {
		return impl->inputState;
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

}