
#include "XRGame.hpp"
#include "Graphics/Renderer.hpp"

#ifndef WIN32
#define XR_USE_PLATFORM_XLIB
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#ifdef WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <unknwn.h>

#else

#include <X11/Xlib.h>
#include <GL/glx.h>

#endif

#ifdef XR_USE_PLATFORM_WAYLAND
#include <wayland-client.h>
#endif

#include "Graphics/VulkanDevice.hpp"

using namespace fe;

void CheckGLError(const char* location) {
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		std::cerr << "OpenGL error at " << location << ": " << err << std::endl;
	}
}

struct fe::XRGame::Impl {

	bool useVulkan = false;

	XrInstance instance = XR_NULL_HANDLE;
	XrSession session = XR_NULL_HANDLE;

	XrSystemId systemId;

	XrSpace appSpace = XR_NULL_HANDLE;

	XrSwapchain swapchain;

	// API-specific swapchain image storage
	std::vector<XrSwapchainImageOpenGLKHR> swapchainImagesGL;
	std::vector<XrSwapchainImageVulkanKHR> swapchainImagesVK;

	// Framebuffer handles from renderDevice, indexed [eye][swapchainImage]
	std::vector<std::vector<uint64_t>> framebuffers;

	// OpenGL-only depth textures (2D array, one per swapchain image)
	std::vector<GLuint> depthTextures;

	uint32_t viewCount = 2;
	int32_t swapchainWidth, swapchainHeight;

	bool drawVR = false;

	XrActionSet actionSet = XR_NULL_HANDLE;
	XrAction moveAction = XR_NULL_HANDLE;
	XrAction orientAction = XR_NULL_HANDLE;
	XrAction poseAction = XR_NULL_HANDLE;
	XrSpace controllerSpace[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE};
	XrSpace headSpace = XR_NULL_HANDLE;

	XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
	XrFrameState frameState{XR_TYPE_FRAME_STATE};
	XrFrameBeginInfo frameBegin{XR_TYPE_FRAME_BEGIN_INFO};
	XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};

	void initSwapchain(IRenderDevice* renderDevice) {
		uint32_t configCount;
		xrEnumerateViewConfigurationViews(instance, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &configCount, nullptr);
		std::vector<XrViewConfigurationView> configViews(configCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
		xrEnumerateViewConfigurationViews(instance, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, configCount, &configCount, configViews.data());

		if (!configViews.size()) {
			std::cerr << "No headset??" << std::endl;
		}

		swapchainWidth = configViews[0].recommendedImageRectWidth;
		swapchainHeight = configViews[0].recommendedImageRectHeight;

		uint32_t formatCount;
		xrEnumerateSwapchainFormats(session, 0, &formatCount, nullptr);
		std::vector<int64_t> formats(formatCount);
		xrEnumerateSwapchainFormats(session, formatCount, &formatCount, formats.data());

		std::cout << "Available swapchain formats:" << std::endl;
		for (auto format : formats) {
			std::cout << "  " << format << std::endl;
		}

		int64_t preferredFormat = static_cast<int64_t>(renderDevice->GetSwapchainFormat());
		int64_t chosenFormat = formats[0];
		for (auto format : formats) {
			if (format == preferredFormat) {
				chosenFormat = format;
				break;
			}
		}
		std::cout << "Using format: " << chosenFormat << std::endl;

		XrSwapchainCreateInfo swapchainInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
		swapchainInfo.arraySize = viewCount;
		swapchainInfo.format = chosenFormat;
		swapchainInfo.width = swapchainWidth;
		swapchainInfo.height = swapchainHeight;
		swapchainInfo.mipCount = 1;
		swapchainInfo.faceCount = 1;
		swapchainInfo.sampleCount = 1;
		swapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

		outputError(xrCreateSwapchain(session, &swapchainInfo, &swapchain));

		uint32_t imageCount;
		outputError(xrEnumerateSwapchainImages(swapchain, 0, &imageCount, nullptr));

		if (useVulkan) {
			swapchainImagesVK.resize(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR});
			outputError(xrEnumerateSwapchainImages(swapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)swapchainImagesVK.data()));
		} else {
			swapchainImagesGL.resize(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR});
			outputError(xrEnumerateSwapchainImages(swapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)swapchainImagesGL.data()));
		}

		// Create framebuffers via renderDevice (one per eye per swapchain image)
		framebuffers.resize(viewCount);
		for (uint32_t eye = 0; eye < viewCount; eye++) {
			framebuffers[eye].resize(imageCount);
			for (uint32_t i = 0; i < imageCount; i++) {
				uint64_t nativeImage = useVulkan
					? reinterpret_cast<uint64_t>(swapchainImagesVK[i].image)
					: static_cast<uint64_t>(swapchainImagesGL[i].image);
				framebuffers[eye][i] = renderDevice->CreateFramebuffer(nativeImage, swapchainWidth, swapchainHeight, eye);
			}
		}

		// OpenGL-only depth textures (2D array for stereo)
		if (!useVulkan) {
			depthTextures.resize(imageCount);
			glGenTextures(imageCount, depthTextures.data());
			for (uint32_t i = 0; i < imageCount; i++) {
				glBindTexture(GL_TEXTURE_2D_ARRAY, depthTextures[i]);
				glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F,
					swapchainWidth, swapchainHeight, 2, 0,
					GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		}
	}

	void CreateAction(XrActionType type, std::string name, XrAction* action) {
		XrActionCreateInfo actionInfo{XR_TYPE_ACTION_CREATE_INFO};
		actionInfo.actionType = XR_ACTION_TYPE_VECTOR2F_INPUT;
		strcpy(actionInfo.actionName, name.c_str());
		strcpy(actionInfo.localizedActionName, name.c_str());
		outputError(xrCreateAction(actionSet, &actionInfo, action));
	}

	void CreateActions() {
		XrActionSetCreateInfo actionSetInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
		strcpy(actionSetInfo.actionSetName, "gameplay");
		strcpy(actionSetInfo.localizedActionSetName, "Gameplay");
		xrCreateActionSet(instance, &actionSetInfo, &actionSet);

		CreateAction(XR_ACTION_TYPE_VECTOR2F_INPUT, "move", &moveAction);

		std::vector<XrActionSuggestedBinding> bindings;

		XrPath leftThumbstickPath;
		xrStringToPath(instance, "/user/hand/right/input/thumbstick", &leftThumbstickPath);
		bindings.push_back({moveAction, leftThumbstickPath});

		XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
		XrPath oculusProfilePath;
		xrStringToPath(instance, "/interaction_profiles/oculus/touch_controller", &oculusProfilePath);
		suggestedBindings.interactionProfile = oculusProfilePath;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		xrSuggestInteractionProfileBindings(instance, &suggestedBindings);

		XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
		attachInfo.actionSets = &actionSet;
		attachInfo.countActionSets = 1;
		xrAttachSessionActionSets(session, &attachInfo);
	}

	void Log(const std::string& message) { std::cout << message << std::endl; }

	void BeginSession() {
		XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
		beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

		outputError(xrBeginSession(session, &beginInfo));
	}

	void HandleSessionStateChange(XrSessionState state, XrTime time) {
		switch (state) {
			case XR_SESSION_STATE_IDLE:
				Log("Session state: IDLE");
				break;
			case XR_SESSION_STATE_READY:
				Log("Session state: READY - Should call xrBeginSession");
				BeginSession();
				break;
			case XR_SESSION_STATE_SYNCHRONIZED:
				Log("Session state: SYNCHRONIZED");
				break;
			case XR_SESSION_STATE_VISIBLE:
				Log("Session state: VISIBLE - Can Render but shouldn't submit");
				break;
			case XR_SESSION_STATE_FOCUSED:
				Log("Session state: FOCUSED - Can Render AND submit frames");
				break;
			case XR_SESSION_STATE_STOPPING:
				Log("Session state: STOPPING - Should call xrEndSession");
				xrEndSession(session);
				drawVR = false;
				break;
		}
	}

	void PollEvents() {
		if (instance == XR_NULL_HANDLE) return;

		XrEventDataBuffer event = {XR_TYPE_EVENT_DATA_BUFFER};

		while (xrPollEvent(instance, &event) == XR_SUCCESS) {
			switch (event.type) {
				case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
					XrEventDataSessionStateChanged* stateChanged = (XrEventDataSessionStateChanged*)&event;
					XrSessionState currentState = stateChanged->state;
					XrTime time = stateChanged->time;
					HandleSessionStateChange(currentState, time);
				} break;
			}
			event = {XR_TYPE_EVENT_DATA_BUFFER};
		}
	}

	void outputError(XrResult result) {
		if (XR_SUCCEEDED(result)) return;
		char buf[XR_MAX_RESULT_STRING_SIZE];
		if (xrResultToString(nullptr, result, buf) == XR_SUCCESS) std::cerr << "Error: " << buf << " (" << result << ")" << std::endl;
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

void XRGame::initOpenXR() {
	impl->useVulkan = useVulkan;

	if (useVulkan) {
		auto* vk = static_cast<VulkanDevice*>(renderDevice.get());
		XrGraphicsBindingVulkanKHR vkBinding{XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR};
		vkBinding.next = nullptr;
		vkBinding.instance = vk->GetInstance();
		vkBinding.physicalDevice = vk->GetPhysicalDevice();
		vkBinding.device = vk->GetDevice();
		vkBinding.queueFamilyIndex = vk->GetGraphicsQueueFamily();
		vkBinding.queueIndex = 0;
		initOpenXR(&vkBinding);
	} else {
		auto window = GetWindow<fe::SDLWindow>();

#ifdef WIN32
		HDC hDC = window->GetDrawingContext();
		HGLRC hGLRC = window->GetOpenGLRenderingContext();
		XrGraphicsBindingOpenGLWin32KHR gfx{XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR};
		gfx.hDC = hDC;
		gfx.hGLRC = hGLRC;
		initOpenXR(&gfx);
#else
		const char *video_driver = SDL_GetCurrentVideoDriver();
		if (video_driver != NULL) {
			std::cout << "Video Driver: " << video_driver << std::endl;
			if (SDL_strcmp(video_driver, "wayland") == 0) {
				XrGraphicsBindingOpenGLWaylandKHR gfx{XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR};
				gfx.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR;
				gfx.display = (wl_display *)window->GetWaylandDisplay();
				initOpenXR(&gfx);
			} else if (SDL_strcmp(video_driver, "x11") == 0) {
				Display *xDisplay = (Display *)window->GetX11Display();
				GLXContext glxContext = glXGetCurrentContext();
				int fb_config_id = 0;
				glXQueryContext(xDisplay, glxContext, GLX_FBCONFIG_ID, &fb_config_id);
				int attribs[] = { GLX_FBCONFIG_ID, fb_config_id, None };
				int num_configs = 0;
				GLXFBConfig* fb_configs = glXChooseFBConfig(xDisplay, DefaultScreen(xDisplay), attribs, &num_configs);
				GLXFBConfig glxFBConfig = fb_configs[0];
				XFree(fb_configs);
				int visual_id_val = 0;
				glXGetFBConfigAttrib(xDisplay, glxFBConfig, GLX_VISUAL_ID, &visual_id_val);
				VisualID visualid = (VisualID)visual_id_val;
				XrGraphicsBindingOpenGLXlibKHR gfx{XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR};
				gfx.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR;
				gfx.next = NULL;
				gfx.xDisplay = xDisplay;
				gfx.glxDrawable = window->GetGLXDrawable();
				gfx.visualid = visualid;
				gfx.glxFBConfig = glxFBConfig;
				gfx.glxContext = glxContext;
				initOpenXR(&gfx);
			}
		}
#endif
	}
}

void XRGame::initOpenXR(void *next) {
	XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};

	const char* extension = useVulkan
		? XR_KHR_VULKAN_ENABLE_EXTENSION_NAME
		: XR_KHR_OPENGL_ENABLE_EXTENSION_NAME;
	const char* enabledExtensions[] = {extension};
	createInfo.enabledExtensionCount = 1;
	createInfo.enabledExtensionNames = enabledExtensions;

	createInfo.applicationInfo.apiVersion = XR_API_VERSION_1_0;
	createInfo.applicationInfo.applicationVersion = 1;
	createInfo.applicationInfo.engineVersion = 1;
	strcpy(createInfo.applicationInfo.engineName, "FenixEngine");
	strcpy(createInfo.applicationInfo.applicationName, "Fenix Engine");

	impl->outputError(xrCreateInstance(&createInfo, &impl->instance));

	XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
	systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

	impl->outputError(xrGetSystem(impl->instance, &systemInfo, &impl->systemId));

	XrSystemProperties systemProps{XR_TYPE_SYSTEM_PROPERTIES};
	impl->outputError(xrGetSystemProperties(impl->instance, impl->systemId, &systemProps));
	impl->Log("System Name: " + std::string(systemProps.systemName));
	impl->Log("Vendor ID: " + std::to_string(systemProps.vendorId));

	if (!useVulkan) {
		impl->Log("Current OpenGL Renderer: " + std::string((char*)glGetString(GL_RENDERER)));
	}

	// Graphics requirements for the selected API
	if (useVulkan) {
		XrGraphicsRequirementsVulkanKHR vkReqs{XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR};
		PFN_xrGetVulkanGraphicsRequirementsKHR pfnGetVulkanReqs = nullptr;
		xrGetInstanceProcAddr(impl->instance, "xrGetVulkanGraphicsRequirementsKHR", (PFN_xrVoidFunction*)(&pfnGetVulkanReqs));
		if (pfnGetVulkanReqs) {
			impl->outputError(pfnGetVulkanReqs(impl->instance, impl->systemId, &vkReqs));
		}
	} else {
		XrGraphicsRequirementsOpenGLKHR glReqs{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
		PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLReqs = nullptr;
		xrGetInstanceProcAddr(impl->instance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction*)(&pfnGetOpenGLReqs));
		if (pfnGetOpenGLReqs) {
			impl->outputError(pfnGetOpenGLReqs(impl->instance, impl->systemId, &glReqs));
		}
	}

	if (impl->useVulkan) {
		PFN_xrGetVulkanGraphicsDeviceKHR pfn = nullptr;
		xrGetInstanceProcAddr(impl->instance, "xrGetVulkanGraphicsDeviceKHR",
			(PFN_xrVoidFunction*)(&pfn));
		if (pfn) {
			auto* vk = static_cast<VulkanDevice*>(renderDevice.get());
			VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
			impl->outputError(pfn(impl->instance, impl->systemId,
				vk->GetInstance(), &vkPhysicalDevice));
		}
	}

	XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
	sessionInfo.systemId = impl->systemId;
	sessionInfo.next = next;

	impl->outputError(xrCreateSession(impl->instance, &sessionInfo, &impl->session));

	impl->Log("OpenXR Session Created");

	impl->BeginSession();

	XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
	spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
	spaceInfo.poseInReferenceSpace.position = {0, 0, 0};
	spaceInfo.poseInReferenceSpace.orientation = {0, 0, 0, 1};

	impl->outputError(xrCreateReferenceSpace(impl->session, &spaceInfo, &impl->appSpace));
}

void XRGame::PollActionsAndUpdateMovement(XrTime predictedDisplayTime) {
	XrVector2f joystickInput = {0.0f, 0.0f};
	XrPosef headPose = {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}};

	XrActiveActionSet activeActionSet{impl->actionSet, XR_NULL_PATH};
	XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
	syncInfo.activeActionSets = &activeActionSet;
	syncInfo.countActiveActionSets = 1;
	xrSyncActions(impl->session, &syncInfo);

	XrActionStateVector2f moveState{XR_TYPE_ACTION_STATE_VECTOR2F};
	XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
	getInfo.action = impl->moveAction;
	xrGetActionStateVector2f(impl->session, &getInfo, &moveState);

	if (moveState.isActive) {
		joystickInput = moveState.currentState;
	} else {
		joystickInput = {0.0f, 0.0f};
	}

	XrSpaceLocation headLocation{XR_TYPE_SPACE_LOCATION};
	impl->headSpace = impl->appSpace;
	xrLocateSpace(impl->headSpace, impl->appSpace, predictedDisplayTime, &headLocation);

	if (headLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) {
		headPose = headLocation.pose;
	}

	auto ori = headPose.orientation;

	if (fabsf(joystickInput.x) > 0.1f || fabsf(joystickInput.y) > 0.1f) {
		glm::vec3 forward = glm::vec3(-2.0f * (ori.x * ori.z + ori.w * ori.y), -2.0f * (ori.y * ori.z - ori.w * ori.x), -1.0f + 2.0f * (ori.x * ori.x + ori.y * ori.y));
		glm::vec3 right = glm::vec3(1.0f - 2.0f * (ori.y * ori.y + ori.z * ori.z), 2.0f * (ori.x * ori.y + ori.w * ori.z), 2.0f * (ori.x * ori.z - ori.w * ori.y));

		forward = glm::normalize(forward);
		right = glm::normalize(right);

		XrVector3f movement;
		float moveSpeed = 0.1f;
		movement.x = .3f * forward.x * joystickInput.y + right.x * joystickInput.x * moveSpeed;
		movement.y = 0.0f;
		movement.z = -.3f * forward.z * joystickInput.y + right.z * joystickInput.x * moveSpeed;

		player->state.position.x += movement.x;
		player->state.position.z += movement.z;

		positionOffset.x += movement.x;
		positionOffset.z += movement.z;
	}
}

bool XRGame::IsInstanceValid() { return impl->instance != XR_NULL_HANDLE; }

void XRGame::DisableVR() {
	impl->outputError(xrRequestExitSession(impl->session));
}

void XRGame::DestroyXR() {
	impl->drawVR = false;

	// Destroy framebuffers via renderDevice
	for (auto& eyeFBs : impl->framebuffers) {
		for (auto fb : eyeFBs) {
			renderDevice->DestroyFramebuffer(fb);
		}
	}
	impl->framebuffers.clear();

	// OpenGL-only depth cleanup
	if (!useVulkan && !impl->depthTextures.empty()) {
		glDeleteTextures((GLsizei)impl->depthTextures.size(), impl->depthTextures.data());
		impl->depthTextures.clear();
	}

	impl->swapchainImagesGL.clear();
	impl->swapchainImagesVK.clear();

	if (impl->session != XR_NULL_HANDLE) xrDestroySession(impl->session);
	if (impl->instance != XR_NULL_HANDLE) xrDestroyInstance(impl->instance);
	impl->session = XR_NULL_HANDLE;
	impl->instance = XR_NULL_HANDLE;
}

void XRGame::LaunchVR() {
	initOpenXR();
	impl->useVulkan = useVulkan;
	impl->initSwapchain(renderDevice.get());
	if (!useVulkan) CheckGLError("after framebuffer setup");
	impl->CreateActions();
	window->StopMouseCapture();
}

void XRGame::RedrawWindow(uint64_t fbo) {
	BindFrameBuffer((int)fbo);
	CheckErrors();
	Renderer::Redraw();
}

void XRGame::EnableXR() {
	if (!IsInstanceValid()) LaunchVR();
	if (IsInstanceValid()) impl->drawVR = true;
}

void XRGame::Redraw(uint64_t fbo) {
	{
		impl->PollEvents();
		if (impl->drawVR) RedrawVR();
		if (drawWindow) RedrawWindow(fbo);
		CheckErrors();
	}
}

void XRGame::RedrawVR() {
	impl->outputError(xrWaitFrame(impl->session, &impl->waitInfo, &impl->frameState));

	PollActionsAndUpdateMovement(impl->frameState.predictedDisplayTime);

	impl->outputError(xrBeginFrame(impl->session, &impl->frameBegin));
	impl->outputError(xrAcquireSwapchainImage(impl->swapchain, &impl->acquireInfo, &swapchainImageIndex));

	XrSwapchainImageWaitInfo waitImageInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
	waitImageInfo.timeout = XR_INFINITE_DURATION;
	impl->outputError(xrWaitSwapchainImage(impl->swapchain, &waitImageInfo));

	XrViewState viewState{XR_TYPE_VIEW_STATE};
	XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
	viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
	viewLocateInfo.displayTime = impl->frameState.predictedDisplayTime;
	viewLocateInfo.space = impl->appSpace;

	uint32_t viewCount = 0;
	impl->outputError(xrEnumerateViewConfigurationViews(impl->instance, impl->systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, nullptr));

	std::vector<XrView> views(viewCount, {XR_TYPE_VIEW});
	std::vector<XrViewConfigurationView> viewConfigs(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
	impl->outputError(xrEnumerateViewConfigurationViews(impl->instance, impl->systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, viewCount, &viewCount, viewConfigs.data()));

	impl->outputError(xrLocateViews(impl->session, &viewLocateInfo, &viewState, viewCount, &viewCount, views.data()));

	std::vector<XrCompositionLayerProjectionView> projectionViews(viewCount);

	bool render2D = false;

	for (uint32_t eye = 0; eye < viewCount; eye++) {
		XrPosef pose = views[eye].pose;
		XrFovf xrFov = views[eye].fov;

		glm::vec3 position(pose.position.x, pose.position.y, pose.position.z);
		glm::quat orientation(pose.orientation.w, pose.orientation.x, pose.orientation.y, pose.orientation.z);
		glm::vec4 fov(xrFov.angleLeft, xrFov.angleRight, xrFov.angleDown, xrFov.angleUp);

		if (!render2D) {
			camera->update(position + positionOffset, orientation, fov);

			// OpenGL-specific: attach depth layer per eye (color is baked in CreateFramebuffer)
			if (!useVulkan) {
				GLuint fbo = (GLuint)impl->framebuffers[eye][swapchainImageIndex];
				glBindFramebuffer(GL_FRAMEBUFFER, fbo);
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
					impl->depthTextures[swapchainImageIndex], 0, eye);
			}

			// Set per-eye view/projection
			renderDevice->SetMat4("view", camera->GetViewMatrix());
			renderDevice->SetMat4("projection", camera->GetProjectionMatrix());

			// Render to the eye framebuffer
			renderDevice->BeginExternalFrame(impl->framebuffers[eye][swapchainImageIndex],
				impl->swapchainWidth, impl->swapchainHeight);
			RenderScene();
			renderDevice->EndExternalFrame();
		} else {
			if (!useVulkan) {
				glBindTexture(GL_TEXTURE_2D_ARRAY, impl->swapchainImagesGL[swapchainImageIndex].image);
				glCopyTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, 0, 0,
					impl->swapchainWidth, impl->swapchainHeight);
			}
		}

		projectionViews[eye] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
		projectionViews[eye].pose = views[eye].pose;
		projectionViews[eye].fov = views[eye].fov;
		projectionViews[eye].subImage.swapchain = impl->swapchain;
		projectionViews[eye].subImage.imageRect.offset = {0, 0};
		projectionViews[eye].subImage.imageRect.extent = {impl->swapchainWidth, impl->swapchainHeight};
		projectionViews[eye].subImage.imageArrayIndex = eye;
	}

	XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
	impl->outputError(xrReleaseSwapchainImage(impl->swapchain, &releaseInfo));

	XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
	layer.space = impl->appSpace;
	layer.viewCount = viewCount;
	layer.views = projectionViews.data();

	const XrCompositionLayerBaseHeader* layers[] = {(XrCompositionLayerBaseHeader*)&layer};

	XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
	endInfo.displayTime = impl->frameState.predictedDisplayTime;
	endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;

	if (viewCount > 0 && projectionViews[0].subImage.swapchain != XR_NULL_HANDLE) {
		XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
		layer.space = impl->appSpace;
		layer.viewCount = viewCount;
		layer.views = projectionViews.data();

		const XrCompositionLayerBaseHeader* layers[] = {(XrCompositionLayerBaseHeader*)&layer};
		endInfo.layerCount = 1;
		endInfo.layers = layers;
	} else {
		endInfo.layerCount = 0;
		endInfo.layers = nullptr;
		std::cerr << "Warning: No valid layers to submit" << std::endl;
	}

	impl->outputError(xrEndFrame(impl->session, &endInfo));
}
