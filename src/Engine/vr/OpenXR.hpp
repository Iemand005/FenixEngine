#pragma once

#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#ifndef FE_EXCLUDE_OPENXR
#include <openxr/openxr.h>
#endif

namespace fe {

	class OpenXR {
	public:
		enum class Hand {
			Left,
			Right
		};

		enum class ControllerPose {
			Grip,
			Aim
		};

		// Generic physical inputs exposed by the runtime. Games map these to
		// their own actions (e.g. X -> spawn, grip -> grab).
		enum class Button {
			X,
			Y,
			A,
			B,
			LeftTrigger,
			RightTrigger
		};

		struct InputState {
			glm::vec2 moveInput{0.0f};
			glm::vec2 lookInput{0.0f};
			glm::vec3 headPosition{0.0f};
			glm::quat headOrientation{1.0f, 0.0f, 0.0f, 0.0f};
			glm::vec3 leftGripPosition{0.0f};
			glm::vec3 leftAimPosition{0.0f};
			glm::vec3 rightGripPosition{0.0f};
			glm::vec3 rightAimPosition{0.0f};
			glm::quat leftGripOrientation{1.0f, 0.0f, 0.0f, 0.0f};
			glm::quat leftAimOrientation{1.0f, 0.0f, 0.0f, 0.0f};
			glm::quat rightGripOrientation{1.0f, 0.0f, 0.0f, 0.0f};
			glm::quat rightAimOrientation{1.0f, 0.0f, 0.0f, 0.0f};
			bool leftGripPressed = false;
			bool rightGripPressed = false;
			bool leftGripPoseActive = false;
			bool leftAimPoseActive = false;
			bool rightGripPoseActive = false;
			bool rightAimPoseActive = false;
		};

	private:
		struct Impl;
		std::unique_ptr<Impl> impl;

	public:
		OpenXR();
		~OpenXR();

		OpenXR(const OpenXR&) = delete;
		OpenXR& operator=(const OpenXR&) = delete;

#ifndef FE_EXCLUDE_OPENXR
		bool InitializeInput(XrInstance instance, XrSession session, XrSpace appSpace);
		void UpdateInput(XrTime predictedDisplayTime);
		void DestroyInput();
#endif

		const InputState& GetInputState() const;
		glm::vec3 GetControllerPosition(Hand hand, ControllerPose pose) const;
		bool IsControllerPoseActive(Hand hand, ControllerPose pose) const;
		bool WasButtonPressed(Button button) const;
		bool ConsumeButtonPress(Button button);
		bool IsButtonHeld(Button button) const;

		// True while the squeeze/grip button is held on the given hand.
		bool IsGripPressed(Hand hand) const;

		// Edge-triggered grip press; clears the pending press when consumed.
		bool ConsumeGripPress(Hand hand);

		// Pulses the haptic actuator of the given controller.
		// amplitude is clamped to [0, 1], durationSeconds is in seconds.
		void TriggerHaptic(Hand hand, float amplitude = 0.5f, float durationSeconds = 0.1f);
	};

}