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
