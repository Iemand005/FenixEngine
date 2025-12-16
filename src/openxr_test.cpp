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

void outputError(XrResult result)
{
  if (XR_SUCCEEDED(result))
    return;
  std::cerr << "Failed to create instance: " << result << "\n";

  // Try to get error info
  char buf[XR_MAX_RESULT_STRING_SIZE];
  if (xrResultToString(nullptr, result, buf) == XR_SUCCESS)
    std::cerr << "Error: " << buf << "\n";
}

float cubeVertices[] = {
    // positions          // colors
    -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    -0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,

    -0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f,

    -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,

    0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f,

    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 1.0f,
    0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 1.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 1.0f};

// Shader source code
const char *vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aColor;
    
    out vec3 ourColor;
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        ourColor = aColor;
    }
)";

const char *fragmentShaderSource = R"(
    #version 330 core
    in vec3 ourColor;
    out vec4 FragColor;
    
    void main() {
        FragColor = vec4(ourColor, 1.0);
    }
)";

// Global OpenGL objects
GLuint cubeVAO, cubeVBO, cubeShaderProgram;
GLint modelLoc, viewLoc, projLoc;

void initCube()
{
  // Create and compile shaders

  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);

  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);

  // Create shader program
  cubeShaderProgram = glCreateProgram();
  glAttachShader(cubeShaderProgram, vertexShader);
  glAttachShader(cubeShaderProgram, fragmentShader);
  glLinkProgram(cubeShaderProgram);

  // Delete shaders (already linked)
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  // Get uniform locations
  modelLoc = glGetUniformLocation(cubeShaderProgram, "model");
  viewLoc = glGetUniformLocation(cubeShaderProgram, "view");
  projLoc = glGetUniformLocation(cubeShaderProgram, "projection");

  // Set up vertex data and buffers
  glGenVertexArrays(1, &cubeVAO);
  glGenBuffers(1, &cubeVBO);

  glBindVertexArray(cubeVAO);
  glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

  // Position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // Color attribute
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  std::cout << "Cube initialized\n";
}

void drawCube(const glm::mat4 &view, const glm::mat4 &projection, const glm::mat4 &model = glm::mat4(1.0f))
{
  // Use shader program
  glUseProgram(cubeShaderProgram);

  // Pass matrices to shader
  glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
  glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

  // Draw cube
  glBindVertexArray(cubeVAO);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);
}

XrInstance instance;
XrSystemId systemId;

XrSpace appSpace = XR_NULL_HANDLE;

XrSwapchain swapchain;
std::vector<XrSwapchainImageOpenGLKHR> swapchainImages;
std::vector<GLuint> framebuffers;
uint32_t viewCount = 2; // Stereo
int32_t swapchainWidth, swapchainHeight;

// Initialize swapchain (call after session creation)
void initSwapchain(XrSession session)
{
  // Get recommended view configuration
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

  // Create swapchain
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

  // Get swapchain images
  uint32_t imageCount;
  xrEnumerateSwapchainImages(swapchain, 0, &imageCount, nullptr);
  swapchainImages.resize(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR});
  xrEnumerateSwapchainImages(swapchain, imageCount, &imageCount,
                             (XrSwapchainImageBaseHeader *)swapchainImages.data());

  // Create framebuffers for each swapchain image
  framebuffers.resize(imageCount);
  glGenFramebuffers(imageCount, framebuffers.data());

  for (int i = 0; i < imageCount; i++)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[i]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D_ARRAY, swapchainImages[i].image, 0);
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

  initCube();

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
    outputError(xrEnumerateViewConfigurationViews(instance, systemId,
                                                  XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, nullptr));

    std::vector<XrView> views(viewCount, {XR_TYPE_VIEW}); // Initialize with correct type
    std::vector<XrViewConfigurationView> viewConfigs(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
    outputError(xrEnumerateViewConfigurationViews(instance, systemId,
                                                  XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, viewCount, &viewCount, viewConfigs.data()));

    outputError(xrLocateViews(session, &viewLocateInfo, &viewState, viewCount, &viewCount, views.data()));

    std::vector<XrCompositionLayerProjectionView> projectionViews(viewCount);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[swapchainImageIndex]);

    for (uint32_t eye = 0; eye < viewCount; eye++)
    {
      glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                swapchainImages[swapchainImageIndex].image, 0, eye);

      glViewport(0, 0, swapchainWidth, swapchainHeight);

      glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      XrPosef pose = views[eye].pose;
      XrFovf fov = views[eye].fov;

      glm::mat4 view = glm::inverse(
          glm::translate(glm::mat4(1.0f),
                         glm::vec3(pose.position.x, pose.position.y, pose.position.z)) *
          glm::mat4_cast(glm::quat(pose.orientation.w,
                                   pose.orientation.x, pose.orientation.y, pose.orientation.z)));

      float tanLeft = tanf(fov.angleLeft);
      float tanRight = tanf(fov.angleRight);
      float tanDown = tanf(fov.angleDown);
      float tanUp = tanf(fov.angleUp);

      glm::mat4 projection = glm::mat4(0.0f);
      projection[0][0] = 2.0f / (tanRight - tanLeft);
      projection[1][1] = 2.0f / (tanUp - tanDown);
      projection[2][0] = (tanRight + tanLeft) / (tanRight - tanLeft);
      projection[2][1] = (tanUp + tanDown) / (tanUp - tanDown);
      projection[2][2] = -0.1f - 100.0f / (100.0f - 0.1f);
      projection[2][3] = -1.0f;
      projection[3][2] = -(0.1f * 100.0f) / (100.0f - 0.1f);

      static float angle = 0.0f;
      angle += 0.01f;
      glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle,
                                    glm::vec3(0.5f, 1.0f, 0.0f));

      drawCube(view, projection, model);

      // Store projection view for layer submission
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

    const XrCompositionLayerBaseHeader *layers[] = {
        (XrCompositionLayerBaseHeader *)&layer};

    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = frameState.predictedDisplayTime;
    endInfo.layerCount = 1; // MUST be > 0 to see in headset!
    endInfo.layers = layers;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    outputError(xrEndFrame(session, &endInfo));

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, 800, 600);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw cube to window (optional)
    glm::mat4 windowView = glm::lookAt(
        glm::vec3(2.0f, 2.0f, 2.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 windowProj = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);
    drawCube(windowView, windowProj);

    glfwSwapBuffers(window);
    glfwPollEvents();

    if (glfwWindowShouldClose(window))
    {
      running = false;
    }
  }

  std::cout << "Done!\n";

  // Cleanup
  xrDestroySession(session);
  xrDestroyInstance(instance);

  return 0;
}