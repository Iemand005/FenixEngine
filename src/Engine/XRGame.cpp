#include "XRGame.hpp"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include <glm/gtc/quaternion.hpp>

#ifndef FE_EXCLUDE_OPENXR
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#endif

#ifndef FE_EXCLUDE_OPENXR
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <unknwn.h>

#elif defined(__ANDROID__)

#include <EGL/egl.h>
#include <jni.h>
#include <SDL3/SDL_system.h>
#include "Log.hpp"

#else

#include <X11/Xlib.h>
#include <GL/glx.h>

#endif
#endif

#if defined(XR_USE_PLATFORM_WAYLAND) && !defined(__ANDROID__)
#ifndef FE_EXCLUDE_OPENXR
#include <wayland-client.h>
#endif
#endif

// #include "Graphics/VulkanDevice.hpp"

#include <imgui.h>

using namespace fe;

void CheckGLError(const char* location) {
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		std::cerr << "OpenGL error at " << location << ": " << err << std::endl;
	}
}

struct fe::XRGame::Impl {
		bool useVulkan = false;

	#ifndef FE_EXCLUDE_OPENXR

		XrInstance instance = XR_NULL_HANDLE;
		XrSession session = XR_NULL_HANDLE;

		XrSystemId systemId;

		XrSpace appSpace = XR_NULL_HANDLE;

		XrSwapchain swapchain;

		// API-specific swapchain image storage
#ifdef __ANDROID__
		std::vector<XrSwapchainImageOpenGLESKHR> swapchainImagesGL;
#else
		std::vector<XrSwapchainImageOpenGLKHR> swapchainImagesGL;
#endif
#ifdef XR_USE_GRAPHICS_API_VULKAN
		std::vector<XrSwapchainImageVulkanKHR> swapchainImagesVK;
#endif

		// Framebuffer handles from renderDevice, indexed [eye][swapchainImage]
		std::vector<std::vector<uint64_t>> framebuffers;

		// OpenGL-only depth textures (2D array, one per swapchain image)
		std::vector<GLuint> depthTextures;

		uint32_t viewCount = 2;
		int32_t swapchainWidth, swapchainHeight;

		bool drawVR = false;
		bool drawOpenVR = false;

		XrActionSet actionSet = XR_NULL_HANDLE;
		XrAction moveAction = XR_NULL_HANDLE;
		XrAction lookAction = XR_NULL_HANDLE;
		XrAction poseAction = XR_NULL_HANDLE;
		XrAction toggleXrAction = XR_NULL_HANDLE;
		XrAction breakBlockAction = XR_NULL_HANDLE;
		XrAction placeBlockAction = XR_NULL_HANDLE;
		XrAction prevBlockAction = XR_NULL_HANDLE;
		XrAction nextBlockAction = XR_NULL_HANDLE;
		XrSpace controllerSpace[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE};
		XrSpace headSpace = XR_NULL_HANDLE;

		XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
		XrFrameState frameState{XR_TYPE_FRAME_STATE};
		XrFrameBeginInfo frameBegin{XR_TYPE_FRAME_BEGIN_INFO};
		XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
		
	#endif

		void initSwapchain(IRenderDevice* renderDevice) {

		#ifndef FE_EXCLUDE_OPENXR

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
		swapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
			XR_SWAPCHAIN_USAGE_SAMPLED_BIT |
			XR_SWAPCHAIN_USAGE_TRANSFER_SRC_BIT;

		outputError(xrCreateSwapchain(session, &swapchainInfo, &swapchain));

		uint32_t imageCount;
		outputError(xrEnumerateSwapchainImages(swapchain, 0, &imageCount, nullptr));

		if (useVulkan) {
#ifdef XR_USE_GRAPHICS_API_VULKAN
			swapchainImagesVK.resize(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR});
			outputError(xrEnumerateSwapchainImages(swapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)swapchainImagesVK.data()));
#endif
		} else {
#ifdef __ANDROID__
			swapchainImagesGL.resize(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR});
#else
			swapchainImagesGL.resize(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR});
#endif
			outputError(xrEnumerateSwapchainImages(swapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)swapchainImagesGL.data()));
		}

		// Create framebuffers via renderDevice (one per eye per swapchain image)
		framebuffers.resize(viewCount);
		for (uint32_t eye = 0; eye < viewCount; eye++) {
			framebuffers[eye].resize(imageCount);
			for (uint32_t i = 0; i < imageCount; i++) {
				uint64_t nativeImage = 0;
				if (useVulkan) {
#ifdef XR_USE_GRAPHICS_API_VULKAN
					nativeImage = reinterpret_cast<uint64_t>(swapchainImagesVK[i].image);
#endif
				} else {
					nativeImage = static_cast<uint64_t>(swapchainImagesGL[i].image);
				}
				framebuffers[eye][i] = renderDevice->CreateFramebuffer(nativeImage, swapchainWidth, swapchainHeight, eye, 0, chosenFormat);
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
				// Triggers: L2 breaks, R2 places.
				{breakBlockAction, Path("/user/hand/left/input/trigger")},
				{placeBlockAction, Path("/user/hand/right/input/trigger")},
				// Grips: L1 switches block left, R1 switches block right.
				{prevBlockAction, Path("/user/hand/left/input/squeeze")},
				{nextBlockAction, Path("/user/hand/right/input/squeeze")},
			});
		}

		// HTC Vive only has a trackpad on each controller.
		SuggestProfileBindings("/interaction_profiles/htc/vive_controller", {
			{moveAction, Path("/user/hand/right/input/trackpad")},
			{lookAction, Path("/user/hand/left/input/trackpad")},
			// L2/R2 = trigger, L1/R1 = squeeze.
			{breakBlockAction, Path("/user/hand/left/input/trigger")},
			{placeBlockAction, Path("/user/hand/right/input/trigger")},
			{prevBlockAction, Path("/user/hand/left/input/squeeze")},
			{nextBlockAction, Path("/user/hand/right/input/squeeze")},
		});

		XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
		attachInfo.actionSets = &actionSet;
		attachInfo.countActionSets = 1;
		xrAttachSessionActionSets(session, &attachInfo);
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

void XRGame::initOpenXR() {
	#ifndef FE_EXCLUDE_OPENXR
	impl->Log("XRGame::initOpenXR() starting");
	impl->useVulkan = useVulkan;

#ifdef XR_USE_GRAPHICS_API_VULKAN
		auto* vk = static_cast<VulkanDevice*>(renderDevice.get());
		XrGraphicsBindingVulkanKHR vkBinding{XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR};
		vkBinding.next = nullptr;
		vkBinding.instance = vk->GetInstance();
		vkBinding.physicalDevice = vk->GetPhysicalDevice();
		vkBinding.device = vk->GetDevice();
		vkBinding.queueFamilyIndex = vk->GetGraphicsQueueFamily();
		vkBinding.queueIndex = 0;
		initOpenXR(&vkBinding);
#else
		impl->Log("Vulkan support not compiled in");
#endif
		auto window = GetWindow<fe::SDLWindow>();

#ifdef _WIN32
		HDC hDC = window->GetDrawingContext();
		HGLRC hGLRC = window->GetOpenGLRenderingContext();
		XrGraphicsBindingOpenGLWin32KHR gfx{XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR};
		gfx.hDC = hDC;
		gfx.hGLRC = hGLRC;
		initOpenXR(&gfx);
#elif defined(__ANDROID__)
		XrGraphicsBindingOpenGLESAndroidKHR gfx{XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR};
		gfx.next = nullptr;

		EGLDisplay display = (EGLDisplay)window->GetEGLDisplay();
		if (!display) display = eglGetCurrentDisplay();

		EGLContext context = (EGLContext)window->GetEGLContext();
		if (!context) context = eglGetCurrentContext();

		impl->Log("Android EGL Display: " + std::to_string((uint64_t)display));
		impl->Log("Android EGL Context: " + std::to_string((uint64_t)context));

		EGLConfig config = (EGLConfig)window->GetEGLConfig();
		if (!config && display && context) {
			EGLint configId = 0;
			if (eglQueryContext(display, context, EGL_CONFIG_ID, &configId)) {
				EGLint attribs[] = { EGL_CONFIG_ID, configId, EGL_NONE };
				int numConfigs = 0;
				eglChooseConfig(display, attribs, &config, 1, &numConfigs);
			}
		}
		impl->Log("Android EGL Config: " + std::to_string((uint64_t)config));

		gfx.display = display;
		gfx.config = config;
		gfx.context = context;

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
		#endif
#endif
}

void XRGame::initOpenXR(void *next) {
	#ifndef FE_EXCLUDE_OPENXR
	impl->Log("XRGame::initOpenXR(void *next) starting");

#ifdef __ANDROID__
	impl->Log("Initializing OpenXR Android loader...");
	// Initialize the OpenXR Android loader before calling any other OpenXR APIs
	PFN_xrInitializeLoaderKHR pfnInitializeLoaderKHR = nullptr;
	XrResult loaderProcRes = xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR", (PFN_xrVoidFunction*)(&pfnInitializeLoaderKHR));
	impl->Log("xrGetInstanceProcAddr(xrInitializeLoaderKHR) returned: " + std::to_string(loaderProcRes));
	if (XR_SUCCEEDED(loaderProcRes) && pfnInitializeLoaderKHR) {
		JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
		JavaVM* vm = nullptr;
		if (env) {
			env->GetJavaVM(&vm);
		}
		jobject activity = (jobject)SDL_GetAndroidActivity();

		XrLoaderInitInfoAndroidKHR loaderInitInfo{XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
		loaderInitInfo.next = nullptr;
		loaderInitInfo.applicationVM = vm;
		loaderInitInfo.applicationContext = activity;
		XrResult res = pfnInitializeLoaderKHR((const XrLoaderInitInfoBaseHeaderKHR*)&loaderInitInfo);
		if (XR_FAILED(res)) {
			impl->Log("xrInitializeLoaderKHR failed with error: " + std::to_string(res));
		} else {
			impl->Log("xrInitializeLoaderKHR succeeded");
		}
	} else {
		impl->Log("xrInitializeLoaderKHR not found or not required");
	}
#endif

	XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};

	std::vector<const char*> enabledExtensions;

#ifdef __ANDROID__
	enabledExtensions.push_back(XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME);
	#ifdef XR_USE_GRAPHICS_API_VULKAN
	if (useVulkan) {
		enabledExtensions.push_back(XR_KHR_VULKAN_ENABLE_EXTENSION_NAME);
	} else {
		enabledExtensions.push_back(XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME);
	}
	#else
	enabledExtensions.push_back(XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME);
	#endif
#else
	#ifdef XR_USE_GRAPHICS_API_VULKAN
	if (useVulkan) {
		enabledExtensions.push_back(XR_KHR_VULKAN_ENABLE_EXTENSION_NAME);
	} else {
		enabledExtensions.push_back(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);
	}
	#else
	enabledExtensions.push_back(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);
	#endif
#endif

	createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
	createInfo.enabledExtensionNames = enabledExtensions.data();

	createInfo.applicationInfo.apiVersion = XR_API_VERSION_1_0;
	createInfo.applicationInfo.applicationVersion = 1;
	createInfo.applicationInfo.engineVersion = 1;
	strcpy(createInfo.applicationInfo.engineName, "FenixEngine");
	strcpy(createInfo.applicationInfo.applicationName, "Fenix Engine");

#ifdef __ANDROID__
	JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
	JavaVM* vm = nullptr;
	if (env) {
		env->GetJavaVM(&vm);
	}
	jobject activity = (jobject)SDL_GetAndroidActivity();

	XrInstanceCreateInfoAndroidKHR androidCreateInfo{XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
	androidCreateInfo.next = nullptr;
	androidCreateInfo.applicationVM = vm;
	androidCreateInfo.applicationActivity = activity;

	createInfo.next = &androidCreateInfo;
#endif

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

#ifdef XR_USE_GRAPHICS_API_VULKAN
	if (useVulkan) {
		XrGraphicsRequirementsVulkanKHR vkReqs{XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR};
		PFN_xrGetVulkanGraphicsRequirementsKHR pfnGetVulkanReqs = nullptr;
		xrGetInstanceProcAddr(impl->instance, "xrGetVulkanGraphicsRequirementsKHR", (PFN_xrVoidFunction*)(&pfnGetVulkanReqs));
		if (pfnGetVulkanReqs) {
			impl->outputError(pfnGetVulkanReqs(impl->instance, impl->systemId, &vkReqs));
		}
	} else {
#else
	if (true) {
#endif
#ifdef __ANDROID__
		XrGraphicsRequirementsOpenGLESKHR glesReqs{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR};
		PFN_xrGetOpenGLESGraphicsRequirementsKHR pfnGetGLESReqs = nullptr;
		xrGetInstanceProcAddr(impl->instance, "xrGetOpenGLESGraphicsRequirementsKHR", (PFN_xrVoidFunction*)(&pfnGetGLESReqs));
		if (pfnGetGLESReqs) {
			impl->outputError(pfnGetGLESReqs(impl->instance, impl->systemId, &glesReqs));
		}
#else
		XrGraphicsRequirementsOpenGLKHR glReqs{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
		PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLReqs = nullptr;
		xrGetInstanceProcAddr(impl->instance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction*)(&pfnGetOpenGLReqs));
		if (pfnGetOpenGLReqs) {
			impl->outputError(pfnGetOpenGLReqs(impl->instance, impl->systemId, &glReqs));
		}
#endif
	}

#ifdef XR_USE_GRAPHICS_API_VULKAN
	if (impl->useVulkan) {
		PFN_xrGetVulkanGraphicsDeviceKHR pfn = nullptr;
		xrGetInstanceProcAddr(impl->instance, "xrGetVulkanGraphicsDeviceKHR",
			(PFN_xrVoidFunction*)(&pfn));
		if (pfn) {
			auto* vkBinding = static_cast<XrGraphicsBindingVulkanKHR*>(next);
			VkPhysicalDevice runtimeDevice = VK_NULL_HANDLE;
			XrResult result = pfn(impl->instance, impl->systemId,
				vkBinding->instance, &runtimeDevice);
			if (XR_SUCCEEDED(result)) {
				vkBinding->physicalDevice = runtimeDevice;
			}
		}
	}
#endif

	XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
	sessionInfo.systemId = impl->systemId;
	sessionInfo.next = next;

	impl->outputError(xrCreateSession(impl->instance, &sessionInfo, &impl->session));

	if (impl->session != XR_NULL_HANDLE) {
		impl->Log("OpenXR Session Created Successfully");
	} else {
		impl->Log("OpenXR Session Creation FAILED");
	}

	XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
	spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
	spaceInfo.poseInReferenceSpace.position = {0, 0, 0};
	spaceInfo.poseInReferenceSpace.orientation = {0, 0, 0, 1};

	if (impl->session != XR_NULL_HANDLE) {
		impl->outputError(xrCreateReferenceSpace(impl->session, &spaceInfo, &impl->appSpace));
	}
	#endif
}

void XRGame::PollActionsAndUpdateMovement(XrTime predictedDisplayTime) {
	#ifndef FE_EXCLUDE_OPENXR
	XrVector2f rightJoystickInput = {0.0f, 0.0f};
	XrVector2f leftJoystickInput = {0.0f, 0.0f};
	XrPosef headPose = {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}};

	XrActiveActionSet activeActionSet{impl->actionSet, XR_NULL_PATH};
	XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
	syncInfo.activeActionSets = &activeActionSet;
	syncInfo.countActiveActionSets = 1;
	xrSyncActions(impl->session, &syncInfo);

	// Right stick = movement.
	XrActionStateVector2f moveState{XR_TYPE_ACTION_STATE_VECTOR2F};
	XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
	getInfo.action = impl->moveAction;
	xrGetActionStateVector2f(impl->session, &getInfo, &moveState);
	rightJoystickInput = moveState.isActive ? moveState.currentState : XrVector2f{0.0f, 0.0f};

	// Left stick = look (turn).
	XrActionStateVector2f lookState{XR_TYPE_ACTION_STATE_VECTOR2F};
	getInfo.action = impl->lookAction;
	xrGetActionStateVector2f(impl->session, &getInfo, &lookState);
	leftJoystickInput = lookState.isActive ? lookState.currentState : XrVector2f{0.0f, 0.0f};

	// Face buttons (X / A) toggle the XR session.
	XrActionStateBoolean toggleState{XR_TYPE_ACTION_STATE_BOOLEAN};
	getInfo.action = impl->toggleXrAction;
	xrGetActionStateBoolean(impl->session, &getInfo, &toggleState);
	if (toggleState.isActive && toggleState.changedSinceLastSync && toggleState.currentState)
		xrToggleRequested = true;

	// Block break/place + hotbar switching. Edge-triggered (changedSinceLastSync
	// + currentState) so a single press raises each request exactly once.
	XrActionStateBoolean blockState{XR_TYPE_ACTION_STATE_BOOLEAN};
	auto raiseOnPress = [&](XrAction action, bool& requested) {
		if (action == XR_NULL_HANDLE) return;
		getInfo.action = action;
		xrGetActionStateBoolean(impl->session, &getInfo, &blockState);
		if (blockState.isActive && blockState.changedSinceLastSync && blockState.currentState)
			requested = true;
	};
	raiseOnPress(impl->breakBlockAction, xrBreakBlockRequested);
	raiseOnPress(impl->placeBlockAction, xrPlaceBlockRequested);
	raiseOnPress(impl->prevBlockAction, xrPrevBlockRequested);
	raiseOnPress(impl->nextBlockAction, xrNextBlockRequested);

	XrSpaceLocation headLocation{XR_TYPE_SPACE_LOCATION};
	impl->headSpace = impl->appSpace;
	xrLocateSpace(impl->headSpace, impl->appSpace, predictedDisplayTime, &headLocation);

	if (headLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) {
		headPose = headLocation.pose;
	}

	auto ori = headPose.orientation;
	glm::quat headOrientation(ori.w, ori.x, ori.y, ori.z);

	// Apply look input (left stick) as smooth yaw turning. Keep the previous
	// heading applied on top of the HMD orientation elsewhere via playerYaw.
	const float lookDeadzone = 0.1f;
	const float turnSpeed = 1.2f; // radians-ish per detached tap; tuned for 60fps, multiplied by dt below
	if (fabsf(leftJoystickInput.x) > lookDeadzone) {
		double dt = std::max(fpsCounter.deltaTime, 0.0001);
		playerYaw -= static_cast<float>(leftJoystickInput.x) * turnSpeed * static_cast<float>(dt);
	}

	// Movement direction should follow where the player is now looking after
	// the turn above, i.e. head orientation rotated by playerYaw.
	glm::quat turnQ = glm::angleAxis(playerYaw, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::quat viewOrientation = turnQ * headOrientation;

	const float moveDeadzone = 0.1f;
	if (fabsf(rightJoystickInput.x) > moveDeadzone || fabsf(rightJoystickInput.y) > moveDeadzone) {
		glm::vec3 forward = viewOrientation * glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3 right = viewOrientation * glm::vec3(1.0f, 0.0f, 0.0f);

		forward = glm::normalize(forward);
		right = glm::normalize(right);

		XrVector3f movement;
		const float moveSpeed = 0.1f;
		movement.x = moveSpeed * forward.x * -rightJoystickInput.y + moveSpeed * right.x * rightJoystickInput.x;
		movement.y = moveSpeed * forward.y * -rightJoystickInput.y;
		movement.z = moveSpeed * forward.z * -rightJoystickInput.y + moveSpeed * right.z * rightJoystickInput.x;

		player->state.position.x += movement.x;
		player->state.position.z += movement.z;

		positionOffset.x += movement.x;
		positionOffset.z += movement.z;
	}
	#endif
}

bool XRGame::IsInstanceValid() {
#ifndef FE_EXCLUDE_OPENXR
	return impl->instance != XR_NULL_HANDLE;
#else
	return false;
#endif
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
	#ifndef FE_EXCLUDE_OPENXR
	impl->outputError(xrRequestExitSession(impl->session));
	#endif
}

void XRGame::ToggleXR() {
	if (IsInstanceValid()) {
		DestroyXR();
	} else {
		EnableXR();
	}
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
	#ifndef FE_EXCLUDE_OPENXR

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
	#ifdef XR_USE_GRAPHICS_API_VULKAN
	impl->swapchainImagesVK.clear();
	#endif

	if (impl->session != XR_NULL_HANDLE) xrDestroySession(impl->session);
	if (impl->instance != XR_NULL_HANDLE) xrDestroyInstance(impl->instance);
	impl->session = XR_NULL_HANDLE;
	impl->instance = XR_NULL_HANDLE;
	#endif

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
	impl->useVulkan = useVulkan;
	impl->initSwapchain(renderDevice.get());
	if (!useVulkan) CheckGLError("after framebuffer setup");
	impl->CreateActions();
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
		impl->PollEvents();

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
	impl->outputError(xrWaitFrame(impl->session, &impl->waitInfo, &impl->frameState));

	// On Oculus (HorizonOS) xrWaitFrame can return shouldRender=false while the
	// session transitions through SYNCHRONIZED/VISIBLE. Keep calling xrWaitFrame
	// every frame so the compositor reaches FOCUSED and drops the loading screen;
	// only begin/submit a real frame when the runtime asks for one.
	if (!impl->frameState.shouldRender) {
		return;
	}

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

	renderDevice->BeginVRFrame();

	// Set up lighting once, shared by all eyes
	if (shader) {
		int count = scene->GetLightCount();
		auto pointLights = scene->GetLights();
		shader->SetInt("lightCount", count);
		for (int i = 0; i < count; ++i) {
			const auto& l = pointLights[i];
			shader->SetVec3("pointLights[" + std::to_string(i) + "].position", l.position);
			shader->SetVec3("pointLights[" + std::to_string(i) + "].color", l.color);
			shader->SetFloat("pointLights[" + std::to_string(i) + "].intensity", l.intensity);
			shader->SetFloat("pointLights[" + std::to_string(i) + "].radius", std::max(0.001f, l.radius));
		}
	}

	for (uint32_t eye = 0; eye < viewCount; eye++) {
		XrPosef pose = views[eye].pose;
		XrFovf xrFov = views[eye].fov;

		glm::vec3 hmdPosition(pose.position.x, pose.position.y, pose.position.z);
		glm::quat hmdOrientation(pose.orientation.w, pose.orientation.x, pose.orientation.y, pose.orientation.z);
		glm::quat turnedOrientation = glm::angleAxis(playerYaw, glm::vec3(0.0f, 1.0f, 0.0f)) * hmdOrientation;
		glm::vec4 fov(xrFov.angleLeft, xrFov.angleRight, xrFov.angleDown, xrFov.angleUp);

		if (!render2D) {
			// Camera position = player position + HMD offset + user offset
			glm::vec3 cameraPos = player->state.position + hmdPosition + positionOffset;
			camera->update(cameraPos, turnedOrientation, fov);

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
			renderDevice->BeginEyeFrame(impl->framebuffers[eye][swapchainImageIndex],
				impl->swapchainWidth, impl->swapchainHeight);
			for (auto& object : scene->GetObjects()) {
				RenderObject(*object);
			}
			renderDevice->EndEyeFrame();
		} else {
			if (!useVulkan) {
				glBindTexture(GL_TEXTURE_2D_ARRAY, impl->swapchainImagesGL[swapchainImageIndex].image);
				glCopyTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, 0, 0,
					impl->swapchainWidth, impl->swapchainHeight);
			}
		}

		projectionViews[eye] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
		projectionViews[eye].pose = views[eye].pose;
		projectionViews[eye].pose.orientation = {turnedOrientation.x, turnedOrientation.y, turnedOrientation.z, turnedOrientation.w};
		projectionViews[eye].fov = views[eye].fov;
		projectionViews[eye].subImage.swapchain = impl->swapchain;
		projectionViews[eye].subImage.imageRect.offset = {0, 0};
		projectionViews[eye].subImage.imageRect.extent = {impl->swapchainWidth, impl->swapchainHeight};
		projectionViews[eye].subImage.imageArrayIndex = eye;
	}

	renderDevice->EndVRFrame();

	XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
	impl->outputError(xrReleaseSwapchainImage(impl->swapchain, &releaseInfo));

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
	#endif
}