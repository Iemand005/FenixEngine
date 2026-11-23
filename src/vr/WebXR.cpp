#include "WebXR.hpp"

#ifdef FE_INCLUDE_WEBXR
#include <algorithm>
#include <array>
#include <cmath>

extern "C" void FE_WebXR_Js_CheckSupport();
extern "C" void FE_WebXR_Js_RequestSession();
extern "C" void FE_WebXR_Js_ExitSession();
extern "C" void FE_WebXR_Js_SetHaptic(int hand, float amplitude, int durationMs);

namespace {

	int Bit(float mask, int index) {
		return (static_cast<int>(mask) >> index) & 1;
	}

	glm::vec3 ReadVec3(const float* data, int count, int base) {
		if (base + 2 >= count) return glm::vec3(0.0f);
		return glm::vec3(data[base], data[base + 1], data[base + 2]);
	}

	glm::quat ReadQuat(const float* data, int count, int base) {
		if (base + 3 >= count) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		return glm::quat(data[base + 3], data[base], data[base + 1], data[base + 2]);
	}

	glm::vec4 ReadFov(const float* data, int count, int base) {
		if (base + 3 >= count) return glm::vec4(0.0f);
		return glm::vec4(data[base], data[base + 1], data[base + 2], data[base + 3]);
	}

	float ReadFloat(const float* data, int count, int index) {
		return index >= 0 && index < count ? data[index] : 0.0f;
	}

}

namespace fe {

	struct WebXR::Impl {
		bool supported = false;
		bool requested = false;
		bool sessionActive = false;
		bool passthroughEnabled = false;
		uint32_t width = 0;
		uint32_t height = 0;
		std::vector<View> views;
		std::array<glm::ivec4, 2> viewports{};
		InputState input;
		float heldMask = 0.0f;
		std::array<bool, 6> buttonPressed{};
		std::array<bool, 2> gripHeld{};
		std::array<bool, 2> gripPressed{};
		HandState inactiveHand;
	};

	WebXR::WebXR() : impl(std::make_unique<Impl>()) {}
	WebXR::~WebXR() = default;

	bool WebXR::Initialize(const GraphicsBinding&) {
		FE_WebXR_Js_CheckSupport();
		return true;
	}

	bool WebXR::GetVulkanRequirements(VulkanRequirements&) { return false; }
	bool WebXR::GetVulkanGraphicsDevice(void*, void*&) { return false; }

	bool WebXR::IsInstanceValid() const {
		return impl->supported || impl->requested || impl->sessionActive;
	}

	bool WebXR::IsSessionRunning() const { return impl->sessionActive; }
	bool WebXR::IsPassthroughSupported() const { return false; }
	bool WebXR::IsPassthroughEnabled() const { return impl->passthroughEnabled; }

	void WebXR::SetPassthroughEnabled(bool enabled) {
		impl->passthroughEnabled = enabled;
	}

	bool WebXR::IsHandTrackingSupported() const { return false; }
	void WebXR::PollEvents() {}
	void WebXR::DestroySwapchain() {}
	void WebXR::UpdateInput(int64_t) {}

	bool WebXR::CreateSwapchain(int64_t, SwapchainInfo& info) {
		info.width = impl->width ? impl->width : 1024;
		info.height = impl->height ? impl->height : 1024;
		info.viewCount = impl->views.empty() ? 2 : static_cast<uint32_t>(impl->views.size());
		info.images.assign(1, 0);
		return true;
	}

	bool WebXR::BeginFrame(uint32_t& imageIndex, std::vector<View>& views) {
		imageIndex = 0;
		views = impl->views;
		return impl->sessionActive && !views.empty();
	}

	void WebXR::EndFrame() {}

	void WebXR::RequestExitSession() {
		if (!impl->sessionActive) return;
		FE_WebXR_Js_ExitSession();
	}

	void WebXR::Shutdown() {
		if (impl->sessionActive) FE_WebXR_Js_ExitSession();
		impl->sessionActive = false;
		impl->requested = false;
		impl->views.clear();
	}

	const WebXR::InputState& WebXR::GetInputState() const {
		return impl->input;
	}

	WebXR::InputState WebXR::GetWorldInputState(const glm::vec3& worldOrigin, const glm::quat& worldRotation) const {
		InputState result = impl->input;
		const auto position = [&](const glm::vec3& p) { return worldOrigin + worldRotation * p; };
		const auto orientation = [&](const glm::quat& q) { return worldRotation * q; };

		result.headPosition = position(result.headPosition);
		result.headOrientation = orientation(result.headOrientation);
		result.leftGripPosition = position(result.leftGripPosition);
		result.leftAimPosition = position(result.leftAimPosition);
		result.rightGripPosition = position(result.rightGripPosition);
		result.rightAimPosition = position(result.rightAimPosition);
		result.leftGripOrientation = orientation(result.leftGripOrientation);
		result.leftAimOrientation = orientation(result.leftAimOrientation);
		result.rightGripOrientation = orientation(result.rightGripOrientation);
		result.rightAimOrientation = orientation(result.rightAimOrientation);
		return result;
	}

	glm::vec3 WebXR::GetControllerPosition(Hand hand, ControllerPose pose) const {
		const bool left = hand == Hand::Left;
		if (pose == ControllerPose::Grip)
			return left ? impl->input.leftGripPosition : impl->input.rightGripPosition;
		return left ? impl->input.leftAimPosition : impl->input.rightAimPosition;
	}

	bool WebXR::IsControllerPoseActive(Hand hand, ControllerPose pose) const {
		const bool left = hand == Hand::Left;
		if (pose == ControllerPose::Grip)
			return left ? impl->input.leftGripPoseActive : impl->input.rightGripPoseActive;
		return left ? impl->input.leftAimPoseActive : impl->input.rightAimPoseActive;
	}

	bool WebXR::IsHandTrackingActive(Hand) const { return false; }
	const WebXR::HandState& WebXR::GetHandState(Hand) const { return impl->inactiveHand; }
	const WebXR::HandJointPose& WebXR::GetHandJointPose(Hand, HandJoint) const {
		static const HandJointPose inactive;
		return inactive;
	}

	bool WebXR::WasButtonPressed(Button button) const {
		return impl->buttonPressed[static_cast<size_t>(button)];
	}

	bool WebXR::ConsumeButtonPress(Button button) {
		auto& pressed = impl->buttonPressed[static_cast<size_t>(button)];
		const bool result = pressed;
		pressed = false;
		return result;
	}

	bool WebXR::IsButtonHeld(Button button) const {
		const int index = static_cast<int>(button);
		return Bit(impl->heldMask, index) != 0;
	}

	bool WebXR::IsGripPressed(Hand hand) const {
		return impl->gripHeld[hand == Hand::Left ? 0 : 1];
	}

	bool WebXR::ConsumeGripPress(Hand hand) {
		auto& pressed = impl->gripPressed[hand == Hand::Left ? 0 : 1];
		const bool result = pressed;
		pressed = false;
		return result;
	}

	void WebXR::TriggerHaptic(Hand hand, float amplitude, float durationSeconds) {
		FE_WebXR_Js_SetHaptic(hand == Hand::Left ? 0 : 1, amplitude,
			static_cast<int>(std::clamp(durationSeconds, 0.0f, 1.0f) * 1000.0f));
	}

	bool WebXR::IsSupported() const { return impl->supported; }
	bool WebXR::IsRequested() const { return impl->requested; }
	uint32_t WebXR::GetFramebufferWidth() const { return impl->width; }
	uint32_t WebXR::GetFramebufferHeight() const { return impl->height; }
	const glm::ivec4& WebXR::GetViewport(size_t eye) const {
		return impl->viewports[eye < 2 ? eye : 0];
	}

	void WebXR::SetSupported(bool supported) {
		impl->supported = supported;
	}

	void WebXR::RequestSession() {
		if (impl->requested) return;
		impl->requested = true;
		FE_WebXR_Js_RequestSession();
	}

	void WebXR::SetSessionActive(bool active, uint32_t width, uint32_t height) {
		impl->sessionActive = active;
		if (width && height) {
			impl->width = width;
			impl->height = height;
		}
		if (!active) {
			impl->requested = false;
			impl->views.clear();
		}
	}

	void WebXR::SetFrameData(const float* data, int count) {
		if (!data || count < 80) return;

		const int viewCount = static_cast<int>(data[0]) == 1 ? 1 : 2;
		impl->width = static_cast<uint32_t>(data[1]);
		impl->height = static_cast<uint32_t>(data[2]);
		impl->views.clear();
		impl->views.reserve(viewCount);

		View head;
		head.position = ReadVec3(data, count, 3);
		head.orientation = ReadQuat(data, count, 6);
		(void)head;

		impl->views.push_back({ReadVec3(data, count, 10), ReadQuat(data, count, 13), ReadFov(data, count, 17)});
		if (viewCount > 1)
			impl->views.push_back({ReadVec3(data, count, 21), ReadQuat(data, count, 24), ReadFov(data, count, 28)});

		InputState& input = impl->input;
		input.headPosition = head.position;
		input.headOrientation = head.orientation;
		input.leftGripPosition = ReadVec3(data, count, 32);
		input.leftGripOrientation = ReadQuat(data, count, 35);
		input.leftGripPoseActive = ReadFloat(data, count, 39) > 0.5f;
		input.leftAimPosition = ReadVec3(data, count, 40);
		input.leftAimOrientation = ReadQuat(data, count, 43);
		input.leftAimPoseActive = ReadFloat(data, count, 47) > 0.5f;
		input.rightGripPosition = ReadVec3(data, count, 48);
		input.rightGripOrientation = ReadQuat(data, count, 51);
		input.rightGripPoseActive = ReadFloat(data, count, 55) > 0.5f;
		input.rightAimPosition = ReadVec3(data, count, 56);
		input.rightAimOrientation = ReadQuat(data, count, 59);
		input.rightAimPoseActive = ReadFloat(data, count, 63) > 0.5f;
		input.moveInput = glm::vec2(ReadFloat(data, count, 64), ReadFloat(data, count, 65));
		input.lookInput = glm::vec2(ReadFloat(data, count, 66), ReadFloat(data, count, 67));

		impl->heldMask = ReadFloat(data, count, 68);
		const int pressedMask = static_cast<int>(ReadFloat(data, count, 69));
		for (size_t i = 0; i < impl->buttonPressed.size(); ++i)
			impl->buttonPressed[i] = ((pressedMask >> i) & 1) != 0;

		impl->gripHeld[0] = Bit(ReadFloat(data, count, 70), 0) != 0;
		impl->gripHeld[1] = Bit(ReadFloat(data, count, 70), 1) != 0;
		const int gripPressedMask = static_cast<int>(ReadFloat(data, count, 71));
		impl->gripPressed[0] = Bit(gripPressedMask, 0) != 0;
		impl->gripPressed[1] = Bit(gripPressedMask, 1) != 0;

		impl->viewports[0] = glm::ivec4(
			static_cast<int>(ReadFloat(data, count, 72)),
			static_cast<int>(ReadFloat(data, count, 73)),
			static_cast<int>(ReadFloat(data, count, 74)),
			static_cast<int>(ReadFloat(data, count, 75)));
		impl->viewports[1] = glm::ivec4(
			static_cast<int>(ReadFloat(data, count, 76)),
			static_cast<int>(ReadFloat(data, count, 77)),
			static_cast<int>(ReadFloat(data, count, 78)),
			static_cast<int>(ReadFloat(data, count, 79)));
	}

}
#endif
