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

#include "engine.h"

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

XrSpace appSpace = XR_NULL_HANDLE;

XrSwapchain swapchain;
std::vector<XrSwapchainImageOpenGLKHR> swapchainImages;
std::vector<GLuint> depthTextures;
std::vector<GLuint> framebuffers;
uint32_t viewCount = 2; // Stereo
int32_t swapchainWidth, swapchainHeight;

void initSwapchain(XrSession session)
{
  uint32_t configCount;
  xrEnumerateViewConfigurationViews(instance, systemId,
                                    XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &configCount, nullptr);
  std::vector<XrViewConfigurationView> configViews(configCount,
                                                   {XR_TYPE_VIEW_CONFIGURATION_VIEW});
  xrEnumerateViewConfigurationViews(instance, systemId,
                                    XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, configCount, &configCount,
                                    configViews.data());

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

int main()
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

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  auto window = glfwCreateWindow(800, 600, "FoxEngine", NULL, NULL);
  glfwMakeContextCurrent(window);

  HGLRC hglrc = wglGetCurrentContext();

  if (!gladLoadGL())
  {
    std::cerr << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  XrGraphicsBindingOpenGLWin32KHR gfx{};
  gfx.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR;
  gfx.hDC = GetDC(glfwGetWin32Window(window));
  gfx.hGLRC = hglrc;

  XrSessionCreateInfo sci{XR_TYPE_SESSION_CREATE_INFO};
  sci.systemId = systemId;
  sci.next = &gfx;

  XrSession session;
  XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
  sessionInfo.systemId = systemId;
  sessionInfo.next = &gfx;

  XrGraphicsRequirementsOpenGLKHR glReqs = {XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
  PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;

  outputError(xrGetInstanceProcAddr(instance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction *)(&pfnGetOpenGLGraphicsRequirementsKHR)));

  outputError(pfnGetOpenGLGraphicsRequirementsKHR(instance, systemId, &glReqs));
  std::cout << "Successfully got OpenGL requirements." << std::endl;

  outputError(xrCreateSession(instance, &sessionInfo, &session));

  std::cout << "OpenXR initialized! Starting session (you should see black in headset)...\n";

  XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
  beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
  XrEnvironmentBlendMode envBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
  // xrEnviro

  uint32_t count = 0;
  xrEnumerateEnvironmentBlendModes(instance, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &count, nullptr);

  outputError(xrBeginSession(session, &beginInfo));

  XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
  spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL; // Or STAGE
  spaceInfo.poseInReferenceSpace.position = {0, 0, 0};          // Origin
  spaceInfo.poseInReferenceSpace.orientation = {0, 0, 0, 1};    // No rotation

  XrResult result = xrCreateReferenceSpace(session, &spaceInfo, &appSpace);
  if (XR_FAILED(result))
  {
    std::cerr << "Failed to create reference space\n";
    return 1;
  }

  uint32_t blendModeCount = 0;
  xrEnumerateEnvironmentBlendModes(
      instance,
      systemId,
      XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
      0,
      &blendModeCount,
      nullptr);

  std::vector<XrEnvironmentBlendMode> blendModes(blendModeCount);
  xrEnumerateEnvironmentBlendModes(
      instance,
      systemId,
      XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
      blendModeCount,
      &blendModeCount,
      blendModes.data());

  bool running = true;

  initSwapchain(session);


  auto scene = std::make_unique<fe::Scene>();
  auto shader = std::make_unique<fe::ShaderProgram>("VertexShader.glsl", "FragmentShader.glsl");
  glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
  glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
  float fov = 45.0f;
  auto playerCamera = std::make_unique<fe::Camera>(cameraPos, cameraFront, cameraUp, fov, (float)800 / (float)600, 0.1f, 100.0f);

  std::shared_ptr<fe::Object> model = std::make_shared<fe::Object>("resources/models/collisiontest.obj");
  model->isStatic = true;
  model->needsUpdate = false;
  scene->addModel(model);

  glEnable(GL_DEPTH_TEST);

  while (running)
  {
    XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frameState{XR_TYPE_FRAME_STATE};
    outputError(xrWaitFrame(session, &waitInfo, &frameState));

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
    static fe::Camera camera;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

    // glDepthRangef(0.0f, 1.0f);

    for (uint32_t eye = 0; eye < viewCount; eye++)
    {
      glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, swapchainImages[swapchainImageIndex].image, 0, eye);

      glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTextures[swapchainImageIndex], 0, eye);

      glViewport(0, 0, swapchainWidth, swapchainHeight);

      glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      XrPosef pose = views[eye].pose;
      XrFovf fov = views[eye].fov;

      glm::vec3 position(pose.position.x, pose.position.y, pose.position.z);
      glm::quat orientation(pose.orientation.w, pose.orientation.x, pose.orientation.y, pose.orientation.z);

      auto front = orientation * glm::vec3(0.0f, 0.0f, -1.0f);
      auto up = orientation * glm::vec3(0.0f, 1.0f, 0.0f);

      float nearDist = 0.10f;
      float farDist = 100.0f;

      float left = tan(fov.angleLeft) * nearDist;
      float right = tan(fov.angleRight) * nearDist;
      float bottom = tan(fov.angleDown) * nearDist;
      float top = tan(fov.angleUp) * nearDist;


      camera = fe::Camera(glm::vec3(pose.position.x, pose.position.y, pose.position.z), front, up, 45.0f, 1.0f, nearDist, farDist);

      camera.projectionMatrix = glm::frustum(left, right, bottom, top, nearDist, farDist);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      scene->render(*shader, camera);

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

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, 800, 600);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    scene->render(*(shader), (camera));

    glfwSwapBuffers(window);
    glfwPollEvents();

    if (glfwWindowShouldClose(window))
      running = false;
  }

  std::cout << "Done!\n";

  xrDestroySession(session);
  xrDestroyInstance(instance);

  return 0;
}