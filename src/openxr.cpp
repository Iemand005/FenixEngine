#pragma once
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_OPENGL
#define GLFW_EXPOSE_NATIVE_WIN32
// #define XR_EXTENSION_PROTOTYPES
// #define XR_KHR_opengl_enable

#include <Windows.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
// #include <GL/glew.h>
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <iostream>
#include <cstring>

#include "engine/engine.h"

void outputError(XrResult result)
{
  if (XR_SUCCEEDED(result))
    return;
  std::cerr << "Failed to create instance: " << result << "\n";

  char buf[XR_MAX_RESULT_STRING_SIZE];
  if (xrResultToString(nullptr, result, buf) == XR_SUCCESS)
    std::cerr << "Error: " << buf << "\n";
}

XrInstance instance;
XrSystemId systemId;
XrSession session;

XrSpace appSpace = XR_NULL_HANDLE;

XrSwapchain swapchain;
std::vector<XrSwapchainImageOpenGLKHR> swapchainImages;
std::vector<GLuint> depthTextures;
std::vector<GLuint> framebuffers;
uint32_t viewCount = 2; // Stereo
int32_t swapchainWidth, swapchainHeight;

XrActionSet m_actionSet = XR_NULL_HANDLE;
XrAction m_moveAction = XR_NULL_HANDLE;
XrAction m_poseAction = XR_NULL_HANDLE;                          // For controller pose, if needed
XrSpace m_controllerSpace[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE}; // For each hand
XrSpace m_headSpace = XR_NULL_HANDLE;

// Movement State
XrVector2f m_joystickInput = {0.0f, 0.0f};
XrPosef m_headPose = {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}};
float m_playerHeight = 1.7f;

glm::vec3 positionOffset(1.0f);

std::shared_ptr<fe::Character> player;

void CreateActions()
{
  // 1. Create Action Set
  XrActionSetCreateInfo actionSetInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
  strcpy(actionSetInfo.actionSetName, "gameplay");
  strcpy(actionSetInfo.localizedActionSetName, "Gameplay");
  xrCreateActionSet(instance, &actionSetInfo, &m_actionSet);

  // 2. Create Actions
  XrActionCreateInfo actionInfo{XR_TYPE_ACTION_CREATE_INFO};
  actionInfo.actionType = XR_ACTION_TYPE_VECTOR2F_INPUT;
  strcpy(actionInfo.actionName, "move");
  strcpy(actionInfo.localizedActionName, "Move");
  xrCreateAction(m_actionSet, &actionInfo, &m_moveAction);

  // 3. Suggest Bindings (e.g., for Oculus Touch)
  std::vector<XrActionSuggestedBinding> bindings;

  // Left thumbstick for movement
  XrPath leftThumbstickPath;
  xrStringToPath(instance, "/user/hand/right/input/thumbstick", &leftThumbstickPath);
  bindings.push_back({m_moveAction, leftThumbstickPath});

  XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
  XrPath oculusProfilePath;
  xrStringToPath(instance, "/interaction_profiles/oculus/touch_controller", &oculusProfilePath);
  suggestedBindings.interactionProfile = oculusProfilePath;
  suggestedBindings.suggestedBindings = bindings.data();
  suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
  xrSuggestInteractionProfileBindings(instance, &suggestedBindings);

  // 4. Attach Action Set to Session
  XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
  attachInfo.actionSets = &m_actionSet;
  attachInfo.countActionSets = 1;
  xrAttachSessionActionSets(session, &attachInfo);
}

void initSwapchain(XrSession session)
{
  uint32_t configCount;
  xrEnumerateViewConfigurationViews(instance, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &configCount, nullptr);
  std::vector<XrViewConfigurationView> configViews(configCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
  xrEnumerateViewConfigurationViews(instance, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, configCount, &configCount, configViews.data());

  swapchainWidth = configViews[0].recommendedImageRectWidth;
  swapchainHeight = configViews[0].recommendedImageRectHeight;

  XrSwapchainCreateInfo swapchainInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
  swapchainInfo.arraySize = viewCount; // Stereo = 2 layers
  swapchainInfo.format = GL_SRGB8_ALPHA8;
  swapchainInfo.width = swapchainWidth;
  swapchainInfo.height = swapchainHeight;
  swapchainInfo.mipCount = 1;
  swapchainInfo.faceCount = 1;
  swapchainInfo.sampleCount = 1;
  swapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

  xrCreateSwapchain(session, &swapchainInfo, &swapchain);

  uint32_t imageCount;
  xrEnumerateSwapchainImages(swapchain, 0, &imageCount, nullptr);
  swapchainImages.resize(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR});
  xrEnumerateSwapchainImages(swapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader *)swapchainImages.data());

  depthTextures = std::vector<GLuint>(imageCount);

  glGenTextures(imageCount, depthTextures.data());

  framebuffers.resize(imageCount);
  glGenFramebuffers(imageCount, framebuffers.data());

  for (int i = 0; i < imageCount; i++)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[i]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_ARRAY, swapchainImages[i].image, 0);

    glBindTexture(GL_TEXTURE_2D_ARRAY, depthTextures[i]);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, swapchainWidth, swapchainHeight, 2, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PollActionsAndUpdateMovement(XrTime predictedDisplayTime)
{
  // 1. Sync Actions
  XrActiveActionSet activeActionSet{m_actionSet, XR_NULL_PATH};
  XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
  syncInfo.activeActionSets = &activeActionSet;
  syncInfo.countActiveActionSets = 1;
  xrSyncActions(session, &syncInfo);

  // 2. Get Joystick State
  XrActionStateVector2f moveState{XR_TYPE_ACTION_STATE_VECTOR2F};
  XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
  getInfo.action = m_moveAction;
  xrGetActionStateVector2f(session, &getInfo, &moveState);

  if (moveState.isActive)
  {
    m_joystickInput = moveState.currentState;
  }
  else
  {
    m_joystickInput = {0.0f, 0.0f};
  }

  // 3. Get Head Pose (CRUCIAL for view-relative movement)
  XrSpaceLocation headLocation{XR_TYPE_SPACE_LOCATION};
  m_headSpace = appSpace;
  xrLocateSpace(m_headSpace, appSpace, predictedDisplayTime, &headLocation);

  if (headLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)
  {
    m_headPose = headLocation.pose;
  }

  // 4. Calculate Movement (Transform joystick by head orientation)
  if (fabsf(m_joystickInput.x) > 0.1f || fabsf(m_joystickInput.y) > 0.1f)
  {
    // Extract forward and right vectors from head orientation
    XrVector3f forward = {
        -2.0f * (m_headPose.orientation.x * m_headPose.orientation.z + m_headPose.orientation.w * m_headPose.orientation.y),
        -2.0f * (m_headPose.orientation.y * m_headPose.orientation.z - m_headPose.orientation.w * m_headPose.orientation.x),
        -1.0f + 2.0f * (m_headPose.orientation.x * m_headPose.orientation.x + m_headPose.orientation.y * m_headPose.orientation.y)};

    XrVector3f right = {
        1.0f - 2.0f * (m_headPose.orientation.y * m_headPose.orientation.y + m_headPose.orientation.z * m_headPose.orientation.z),
        2.0f * (m_headPose.orientation.x * m_headPose.orientation.y + m_headPose.orientation.w * m_headPose.orientation.z),
        2.0f * (m_headPose.orientation.x * m_headPose.orientation.z - m_headPose.orientation.w * m_headPose.orientation.y)};

    // Normalize and apply joystick input
    // forward = glm::normalize(forward);
    // right = glm::normalize(right);

    XrVector3f movement;
    float moveSpeed = 0.1f; // meters per second
    movement.x = forward.x * m_joystickInput.y + right.x * m_joystickInput.x * moveSpeed;
    movement.y = 0.0f; // Typically no vertical movement from joystick
    movement.z = forward.z * m_joystickInput.y + right.z * m_joystickInput.x * moveSpeed;

    // Apply movement speed and delta time
    // float deltaTime = GetFrameDeltaTime(); // Implement this
    // movement = movement //ScaleVector(movement, moveSpeed * 1);
    player->position.x += movement.x;
    player->position.z += movement.z;

    positionOffset.x += movement.x;
    positionOffset.z += movement.z;
  }
}

void initOpenXR(HDC hDC, HGLRC hGLRC)
{
  XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};

  const char *enabledExtensions[] = {XR_KHR_OPENGL_ENABLE_EXTENSION_NAME};
  createInfo.enabledExtensionCount = 1;
  createInfo.enabledExtensionNames = enabledExtensions;

  createInfo.applicationInfo.apiVersion = XR_API_VERSION_1_0;
  createInfo.applicationInfo.applicationVersion = 1;
  createInfo.applicationInfo.engineVersion = 1;
  strcpy(createInfo.applicationInfo.engineName, "FoxEngine");
  strcpy(createInfo.applicationInfo.applicationName, "FoxEngineTest");

  outputError(xrCreateInstance(&createInfo, &instance));

  XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
  systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

  outputError(xrGetSystem(instance, &systemInfo, &systemId));

  XrGraphicsBindingOpenGLWin32KHR gfx{};
  gfx.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR;
  gfx.hDC = hDC;
  gfx.hGLRC = hGLRC;

  XrSessionCreateInfo sci{XR_TYPE_SESSION_CREATE_INFO};
  sci.systemId = systemId;
  sci.next = &gfx;

  XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
  sessionInfo.systemId = systemId;
  sessionInfo.next = &gfx;

  XrGraphicsRequirementsOpenGLKHR glReqs{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
  PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;

  outputError(xrGetInstanceProcAddr(instance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction *)(&pfnGetOpenGLGraphicsRequirementsKHR)));
  outputError(pfnGetOpenGLGraphicsRequirementsKHR(instance, systemId, &glReqs));
  outputError(xrCreateSession(instance, &sessionInfo, &session));

  XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
  beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

  outputError(xrBeginSession(session, &beginInfo));

  XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
  spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
  spaceInfo.poseInReferenceSpace.position = {0, 0, 0};
  spaceInfo.poseInReferenceSpace.orientation = {0, 0, 0, 1};

  outputError(xrCreateReferenceSpace(session, &spaceInfo, &appSpace));
}

int main()
{
  bool drawWindow = true;

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  auto window = glfwCreateWindow(800, 600, "FoxEngine", NULL, NULL);
  glfwMakeContextCurrent(window);

  if (!gladLoadGL())
  {
    std::cerr << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  initOpenXR(GetDC(glfwGetWin32Window(window)), wglGetCurrentContext());
  initSwapchain(session);
  CreateActions();

  auto scene = std::make_unique<fe::Scene>();
  auto shader = std::make_unique<fe::ShaderProgram>("VertexShader.glsl", "FragmentShader.glsl");

  std::shared_ptr<fe::Object> model = std::make_shared<fe::Object>("resources/models/collisiontest.obj");
  model->isStatic = true;
  model->needsUpdate = false;
  scene->addModel(model);

  auto playerObject = std::make_shared<fe::Object>("resources/models/citizen.obj", 0.1f);
  scene->addModel(playerObject);
  player = std::static_pointer_cast<fe::Character>(playerObject);

  bool running = true;
  while (running)
  {
    XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frameState{XR_TYPE_FRAME_STATE};
    outputError(xrWaitFrame(session, &waitInfo, &frameState));

    PollActionsAndUpdateMovement(frameState.predictedDisplayTime);

    XrFrameBeginInfo frameBegin{XR_TYPE_FRAME_BEGIN_INFO};
    outputError(xrBeginFrame(session, &frameBegin));

    XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    uint32_t swapchainImageIndex;
    outputError(xrAcquireSwapchainImage(swapchain, &acquireInfo, &swapchainImageIndex));

    XrSwapchainImageWaitInfo waitImageInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    waitImageInfo.timeout = XR_INFINITE_DURATION;
    outputError(xrWaitSwapchainImage(swapchain, &waitImageInfo));

    XrViewState viewState{XR_TYPE_VIEW_STATE};
    XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
    viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    viewLocateInfo.displayTime = frameState.predictedDisplayTime;
    viewLocateInfo.space = appSpace;

    uint32_t viewCount = 0;
    outputError(xrEnumerateViewConfigurationViews(instance, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, nullptr));

    std::vector<XrView> views(viewCount, {XR_TYPE_VIEW});
    std::vector<XrViewConfigurationView> viewConfigs(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
    outputError(xrEnumerateViewConfigurationViews(instance, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, viewCount, &viewCount, viewConfigs.data()));

    outputError(xrLocateViews(session, &viewLocateInfo, &viewState, viewCount, &viewCount, views.data()));

    std::vector<XrCompositionLayerProjectionView> projectionViews(viewCount);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[swapchainImageIndex]);
    static float angle = 0.0f;
    static fe::Camera camera = fe::Camera(0.1f, 100.0f);

    for (uint32_t eye = 0; eye < viewCount; eye++)
    {
      glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, swapchainImages[swapchainImageIndex].image, 0, eye);
      glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTextures[swapchainImageIndex], 0, eye);

      XrPosef xrPose = views[eye].pose;
      XrFovf xrFov = views[eye].fov;

      glm::vec3 position(xrPose.position.x, xrPose.position.y, xrPose.position.z);
      glm::quat orientation(xrPose.orientation.w, xrPose.orientation.x, xrPose.orientation.y, xrPose.orientation.z);
      glm::vec4 fov(xrFov.angleLeft, xrFov.angleRight, xrFov.angleDown, xrFov.angleUp);

      auto front = orientation * glm::vec3(0.0f, 0.0f, -1.0f);
      auto up = orientation * glm::vec3(0.0f, 1.0f, 0.0f);

      // float nearDist = 0.10f;
      // float farDist = 100.0f;

      // float left = tan(fov.angleLeft) * camera.nearDist;
      // float right = tan(fov.angleRight) * camera.nearDist;
      // float bottom = tan(fov.angleDown) * camera.nearDist;
      // float top = tan(fov.angleUp) * camera.nearDist;

      // position = position + positionOffset;

      // camera = fe::Camera(position, front, up, 45.0f, 1.0f, nearDist, farDist);

      // camera.setPos(position);
      camera.updateView(position + positionOffset, orientation);
      camera.updateProjection(fov);

      // camera.projectionMatrix = glm::frustum(left, right, bottom, top, nearDist, farDist);

      scene->render(*shader, camera, swapchainWidth, swapchainHeight);

      projectionViews[eye] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
      projectionViews[eye].pose = views[eye].pose;
      projectionViews[eye].fov = views[eye].fov;
      projectionViews[eye].subImage.swapchain = swapchain;
      projectionViews[eye].subImage.imageRect.offset = {0, 0};
      projectionViews[eye].subImage.imageRect.extent = {swapchainWidth, swapchainHeight};
      projectionViews[eye].subImage.imageArrayIndex = eye;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    outputError(xrReleaseSwapchainImage(swapchain, &releaseInfo));

    XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    layer.space = appSpace;
    layer.viewCount = viewCount;
    layer.views = projectionViews.data();

    const XrCompositionLayerBaseHeader *layers[] = {(XrCompositionLayerBaseHeader *)&layer};

    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = frameState.predictedDisplayTime;
    endInfo.layerCount = 1;
    endInfo.layers = layers;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    outputError(xrEndFrame(session, &endInfo));

    if (drawWindow)
    {
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      
      scene->render(*(shader), (camera), 800, 600);

      glfwSwapBuffers(window);
      glfwPollEvents();
    }

    if (glfwWindowShouldClose(window))
      running = false;
  }

  std::cout << "Done!\n";

  xrDestroySession(session);
  xrDestroyInstance(instance);

  return 0;
}