#pragma once

#include <memory>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#ifndef XR_EXTENSION_PROTOTYPES
#define XR_EXTENSION_PROTOTYPES
#endif

#if defined(_WIN32)
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_OPENGL
#define XR_USE_GRAPHICS_API_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <unknwn.h>
#elif defined(__ANDROID__) || defined(ANDROID)
#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <EGL/egl.h>
#include <jni.h>
#elif defined(__linux__) && !defined(__ANDROID__) && !defined(ANDROID)
#define XR_USE_PLATFORM_WAYLAND
#define XR_USE_PLATFORM_XLIB
#define XR_USE_GRAPHICS_API_OPENGL
#define XR_USE_GRAPHICS_API_VULKAN
#endif

#ifndef FE_EXCLUDE_OPENXR
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
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

		// Generic physical inputs exposed by the runtime. Games map these to  their own actions (e.g. X -> spawn, grip -> grab).
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

		struct GraphicsBinding {
			enum class WindowSystem {
				Default,
				Wayland,
				X11
			};

			bool useVulkan = false;
			WindowSystem windowSystem = WindowSystem::Default;
			void* instance = nullptr;
			void* physicalDevice = nullptr;
			void* device = nullptr;
			uint32_t queueFamilyIndex = 0;
			void* display = nullptr;
			void* config = nullptr;
			void* context = nullptr;
			uintptr_t drawable = 0;
			uintptr_t framebufferConfig = 0;
			uintptr_t visualId = 0;
		};

		struct SwapchainInfo {
			uint32_t width = 0;
			uint32_t height = 0;
			uint32_t viewCount = 0;
			int64_t format = 0;
			std::vector<uint64_t> images;
		};

		struct View {
			glm::vec3 position{0.0f};
			glm::quat orientation{1.0f, 0.0f, 0.0f, 0.0f};
			glm::vec4 fov{0.0f};
		};

		struct VulkanRequirements {
			std::vector<std::string> instanceExtensions;
			std::vector<std::string> deviceExtensions;
			uint64_t minApiVersion = 0;
			uint64_t maxApiVersion = 0;
		};

	private:
		struct Impl;
		std::unique_ptr<Impl> impl;
		void DestroyInput();

	public:
		OpenXR();
		~OpenXR();

		OpenXR(const OpenXR&) = delete;
		OpenXR& operator=(const OpenXR&) = delete;

		bool Initialize(const GraphicsBinding& binding);
		static bool GetVulkanRequirements(VulkanRequirements& requirements);
		static bool GetVulkanGraphicsDevice(void* vulkanInstance, void*& physicalDevice);
		bool IsInstanceValid() const;
		bool IsSessionRunning() const;
		bool IsPassthroughSupported() const;
		bool IsPassthroughEnabled() const;
		void SetPassthroughEnabled(bool enabled);
		void PollEvents();
		bool CreateSwapchain(int64_t preferredFormat, SwapchainInfo& info);
		void DestroySwapchain();
		bool BeginFrame(uint32_t& imageIndex, std::vector<View>& views);
		void EndFrame();
		void RequestExitSession();
		void Shutdown();
		void UpdateInput(int64_t predictedDisplayTime);

		// Latest raw input; tracked poses are in OpenXR reference space.
		const InputState& GetInputState() const;
		// Returns the latest input with tracked poses transformed from reference
		// space into world space. Button and movement inputs are unchanged.
		InputState GetWorldInputState(const glm::vec3& worldOrigin, const glm::quat& worldRotation) const;
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