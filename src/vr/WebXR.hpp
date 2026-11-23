#pragma once
#include "OpenXR.hpp"

#ifdef FE_INCLUDE_WEBXR
namespace fe {

	class WebXR {
	public:
		using Hand = OpenXR::Hand;
		using ControllerPose = OpenXR::ControllerPose;
		using HandJoint = OpenXR::HandJoint;
		using HandJointPose = OpenXR::HandJointPose;
		using HandState = OpenXR::HandState;
		using Button = OpenXR::Button;
		using InputState = OpenXR::InputState;
		using GraphicsBinding = OpenXR::GraphicsBinding;
		using SwapchainInfo = OpenXR::SwapchainInfo;
		using View = OpenXR::View;
		using VulkanRequirements = OpenXR::VulkanRequirements;

		WebXR();
		~WebXR();

		WebXR(const WebXR&) = delete;
		WebXR& operator=(const WebXR&) = delete;

		bool Initialize(const GraphicsBinding& binding);
		static bool GetVulkanRequirements(VulkanRequirements& requirements);
		static bool GetVulkanGraphicsDevice(void* vulkanInstance, void*& physicalDevice);
		bool IsInstanceValid() const;
		bool IsSessionRunning() const;
		bool IsPassthroughSupported() const;
		bool IsPassthroughEnabled() const;
		void SetPassthroughEnabled(bool enabled);
		bool IsHandTrackingSupported() const;
		void PollEvents();
		bool CreateSwapchain(int64_t preferredFormat, SwapchainInfo& info);
		void DestroySwapchain();
		bool BeginFrame(uint32_t& imageIndex, std::vector<View>& views);
		void EndFrame();
		void RequestExitSession();
		void Shutdown();
		void UpdateInput(int64_t predictedDisplayTime);

		const InputState& GetInputState() const;
		InputState GetWorldInputState(const glm::vec3& worldOrigin, const glm::quat& worldRotation) const;
		glm::vec3 GetControllerPosition(Hand hand, ControllerPose pose) const;
		bool IsControllerPoseActive(Hand hand, ControllerPose pose) const;
		bool IsHandTrackingActive(Hand hand) const;
		const HandState& GetHandState(Hand hand) const;
		const HandJointPose& GetHandJointPose(Hand hand, HandJoint joint) const;
		bool WasButtonPressed(Button button) const;
		bool ConsumeButtonPress(Button button);
		bool IsButtonHeld(Button button) const;
		bool IsGripPressed(Hand hand) const;
		bool ConsumeGripPress(Hand hand);
		void TriggerHaptic(Hand hand, float amplitude = 0.5f, float durationSeconds = 0.1f);

		bool IsSupported() const;
		bool IsRequested() const;
		uint32_t GetFramebufferWidth() const;
		uint32_t GetFramebufferHeight() const;
		const glm::ivec4& GetViewport(size_t eye) const;

		void SetSupported(bool supported);
		void RequestSession();
		void SetSessionActive(bool active, uint32_t width = 0, uint32_t height = 0);
		void SetFrameData(const float* data, int count);

	private:
		struct Impl;
		std::unique_ptr<Impl> impl;
	};

}
#endif
