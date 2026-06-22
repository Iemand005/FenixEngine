
#include "XRGame.hpp"

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#ifdef WIN32
#define XR_USE_PLATFORM_WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <unknwn.h>
#endif

using namespace fe;

void CheckGLError(const char* location) {
  GLenum err;
  while ((err = glGetError()) != GL_NO_ERROR) {
    std::cerr << "OpenGL error at " << location << ": " << err << std::endl;
  }
}

struct fe::XRGame::Impl {

  XrInstance instance = XR_NULL_HANDLE;
  XrSession session = XR_NULL_HANDLE;

  XrSystemId systemId;

  XrSpace appSpace = XR_NULL_HANDLE;

  XrSwapchain swapchain;
  std::vector<XrSwapchainImageOpenGLKHR> swapchainImages;

  std::vector<GLuint> depthTextures;
  std::vector<GLuint> framebuffers;
  uint32_t viewCount = 2;  // Stereo
  int32_t swapchainWidth, swapchainHeight;

  bool drawVR = false;


  XrActionSet actionSet = XR_NULL_HANDLE;
  XrAction moveAction = XR_NULL_HANDLE;
  XrAction orientAction = XR_NULL_HANDLE;
  XrAction poseAction = XR_NULL_HANDLE;                           // For controller pose, if needed
  XrSpace controllerSpace[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE};  // For each hand
  XrSpace headSpace = XR_NULL_HANDLE;

  XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
  XrFrameState frameState{XR_TYPE_FRAME_STATE};
  XrFrameBeginInfo frameBegin{XR_TYPE_FRAME_BEGIN_INFO};
  XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};

  
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

    // Left thumbstick for movement
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


  void initSwapchain() {
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

    // Try to find a supported format - GL_RGBA8 is usually 0x8058
    int64_t chosenFormat = formats[1];  // Try RGBA8 first
    for (auto format : formats) {
        if (format == GL_RGBA8) {
            chosenFormat = format;
            std::cout << "Using format: " << chosenFormat << std::endl;
            break;
        }
    }
    // // {
    // //   formatFound = true;
    // //   chosenFormat = ;
    // // }

    // if (!formats.empty()) {
    //   chosenFormat = formats[0];
    //   std::cout << "Falling back to format: " << chosenFormat << std::endl;
    // }

    XrSwapchainCreateInfo swapchainInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
    swapchainInfo.arraySize = viewCount;  // Stereo = 2 layers
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
    swapchainImages.resize(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR});
    outputError(xrEnumerateSwapchainImages(swapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)swapchainImages.data()));

    depthTextures = std::vector<GLuint>(imageCount);

    glGenTextures(imageCount, depthTextures.data());

    framebuffers.resize(imageCount);
    glGenFramebuffers(imageCount, framebuffers.data());

    for (int i = 0; i < imageCount; i++) {
      glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[i]);

      // Initialize with no color attachment for now
      // We'll attach layers dynamically in RedrawVR()

      // Create depth texture as 2D array for stereo
      glBindTexture(GL_TEXTURE_2D_ARRAY, depthTextures[i]);
      glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, swapchainWidth, swapchainHeight, 2, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

      // Set texture parameters
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  

  void Log(const std::string& message) { std::cout << message << std::endl; }

  

  void BeginSession() {
    XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
    beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

    outputError(xrBeginSession(session, &beginInfo));
  }

  

  // void BindFrameBuffer(int bufferIndex = 0) {
  //   glBindFramebuffer(GL_FRAMEBUFFER, bufferIndex);
  // }

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
        // m_canSubmitFrames = true;  // Set a flag for your Render loop
        break;
      case XR_SESSION_STATE_STOPPING:
        Log("Session state: STOPPING - Should call xrEndSession");
        xrEndSession(session);
        drawVR = false;

        // m_canSubmitFrames = false;
        break;
    }
  }

  void PollEvents() {
    XrEventDataBuffer event = {XR_TYPE_EVENT_DATA_BUFFER};

    while (xrPollEvent(instance, &event) == XR_SUCCESS) {
      switch (event.type) {
        case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
          XrEventDataSessionStateChanged* stateChanged = (XrEventDataSessionStateChanged*)&event;

          XrSessionState currentState = stateChanged->state;
          XrTime time = stateChanged->time;

          // Handle the state transition
          HandleSessionStateChange(currentState, time);

        } break;
          // Handle other event types...
      }
      event = {XR_TYPE_EVENT_DATA_BUFFER};  // Reset for next poll
    }
  }

  void outputError(XrResult result) {
    if (XR_SUCCEEDED(result)) return;
    // std::cerr << "Error code: " << result << "\n";

    char buf[XR_MAX_RESULT_STRING_SIZE];
    if (xrResultToString(nullptr, result, buf) == XR_SUCCESS) std::cerr << "Error: " << buf << " (" << result << ")" << std::endl;
  }
}; // Impl

XRGame::XRGame(bool launchVR) : XRGame(0, 0, launchVR, true) {}

XRGame::XRGame(int width, int height, bool launchVR, bool drawWindow) : Game(width, height), impl(std::make_unique<Impl>()) {
  this->drawWindow = drawWindow;
  if (launchVR) LaunchVR();
}

XRGame::XRGame(GLADloadproc loadProc) : Game(loadProc), impl(std::make_unique<Impl>()) {}

XRGame::~XRGame() {
  Destroy();
};

#ifdef XR_USE_PLATFORM_WIN32

void XRGame::initOpenXR(HDC hDC, HGLRC hGLRC) {
    XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};

    const char* enabledExtensions[] = {XR_KHR_OPENGL_ENABLE_EXTENSION_NAME};
    createInfo.enabledExtensionCount = 1;
    createInfo.enabledExtensionNames = enabledExtensions;

    createInfo.applicationInfo.apiVersion = XR_API_VERSION_1_0;
    createInfo.applicationInfo.applicationVersion = 1;
    createInfo.applicationInfo.engineVersion = 1;
    strcpy(createInfo.applicationInfo.engineName, "FoxEngine");
    strcpy(createInfo.applicationInfo.applicationName, "FoxEngineTest");

    impl->outputError(xrCreateInstance(&createInfo, &impl->instance));

    XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    impl->outputError(xrGetSystem(impl->instance, &systemInfo, &impl->systemId));

    XrSystemProperties systemProps{XR_TYPE_SYSTEM_PROPERTIES};
    impl->outputError(xrGetSystemProperties(impl->instance, impl->systemId, &systemProps));
    impl->Log("System Name: " + std::string(systemProps.systemName));
    impl->Log("Vendor ID: " + std::to_string(systemProps.vendorId));

    impl->Log("OpenXR Session Created");

    impl->Log("Current OpenGL Renderer: " + std::string((char*)glGetString(GL_RENDERER)));

    XrGraphicsBindingOpenGLWin32KHR gfx{};
    gfx.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR;
    gfx.hDC = hDC;
    gfx.hGLRC = hGLRC;

    XrSessionCreateInfo sci{XR_TYPE_SESSION_CREATE_INFO};
    sci.systemId = impl->systemId;
    sci.next = &gfx;

    XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.systemId = impl->systemId;
    sessionInfo.next = &gfx;

    XrGraphicsRequirementsOpenGLKHR glReqs{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
    PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;

    impl->outputError(xrGetInstanceProcAddr(impl->instance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction*)(&pfnGetOpenGLGraphicsRequirementsKHR)));
    impl->outputError(pfnGetOpenGLGraphicsRequirementsKHR(impl->instance, impl->systemId, &glReqs));
    impl->outputError(xrCreateSession(impl->instance, &sessionInfo, &impl->session));

    impl->BeginSession();

    XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    spaceInfo.poseInReferenceSpace.position = {0, 0, 0};
    spaceInfo.poseInReferenceSpace.orientation = {0, 0, 0, 1};

    impl->outputError(xrCreateReferenceSpace(impl->session, &spaceInfo, &impl->appSpace));
  }

#endif

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

      // Normalize and apply joystick input
      forward = glm::normalize(forward);
      right = glm::normalize(right);

      XrVector3f movement;
      float moveSpeed = 0.1f;  // meters per second
      movement.x = .3f * forward.x * joystickInput.y + right.x * joystickInput.x * moveSpeed;
      movement.y = 0.0f;  // Typically no vertical movement from joystick
      movement.z = -.3f * forward.z * joystickInput.y + right.z * joystickInput.x * moveSpeed;

      // Apply movement speed and delta time
      // float deltaTime = GetFrameDeltaTime(); // Implement this
      // movement = movement //ScaleVector(movement, moveSpeed * 1);
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
  if (impl->session != XR_NULL_HANDLE) xrDestroySession(impl->session);
  if (impl->instance != XR_NULL_HANDLE) xrDestroyInstance(impl->instance);
  impl->session = XR_NULL_HANDLE;
  impl->instance = XR_NULL_HANDLE;
}

void XRGame::LaunchVR() {
  initOpenXR();
  impl->initSwapchain();
  CheckGLError("after framebuffer setup");
  impl->CreateActions();
  window->StopMouseCapture();
}

void XRGame::RedrawWindow(GLuint fbo) {
  // if (window && window.get())
  BindFrameBuffer(fbo);
  CheckErrors();
  Game::Redraw(fbo);
}

void XRGame::EnableXR() {
  if (!IsInstanceValid()) LaunchVR();
  if (IsInstanceValid()) impl->drawVR = true;
}

void XRGame::Redraw(GLuint fbo) {
  {
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

    glBindFramebuffer(GL_FRAMEBUFFER, impl->framebuffers[swapchainImageIndex]);
    static float angle = 0.0f;

    bool render2D = false;

    for (uint32_t eye = 0; eye < viewCount; eye++) {
      XrPosef pose = views[eye].pose;
      XrFovf xrFov = views[eye].fov;

      glm::vec3 position(pose.position.x, pose.position.y, pose.position.z);
      glm::quat orientation(pose.orientation.w, pose.orientation.x, pose.orientation.y, pose.orientation.z);
      glm::vec4 fov(xrFov.angleLeft, xrFov.angleRight, xrFov.angleDown, xrFov.angleUp);

      if (!render2D) {
        glBindFramebuffer(GL_FRAMEBUFFER, impl->framebuffers[swapchainImageIndex]);
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, impl->swapchainImages[swapchainImageIndex].image, 0, eye);
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, impl->depthTextures[swapchainImageIndex], 0, eye);

        // Check framebuffer status
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
          std::cerr << "Framebuffer incomplete for eye " << eye << ": " << status << std::endl;
        }

        // glViewport(0, 0, swapchainWidth, swapchainHeight);
        // glScissor(0, 0, swapchainWidth, swapchainHeight);

        camera->update(position + positionOffset, orientation, fov);
        scene->Render(*shader, *camera, impl->swapchainWidth, impl->swapchainHeight);
      } else {
        glBindTexture(GL_TEXTURE_2D_ARRAY, impl->swapchainImages[swapchainImageIndex].image);
        glCopyTexSubImage3D(GL_TEXTURE_2D_ARRAY,  // Target is a texture array
                            0,                    // Mipmap level 0
                            0, 0, 1,              // Destination (x, y, layer) -> layer 1
                            0, 0,                 // Source (x, y) in framebuffer
                            impl->swapchainWidth, impl->swapchainHeight);
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
