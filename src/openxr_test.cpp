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
    std::cerr << "Cannot create XR instance. Is OpenXR runtime installed?\n";
}

int main()
{
  XrInstance instance;
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

  XrSystemId systemId;
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

  // 4. Begin Session
  XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
  beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
  outputError(xrBeginSession(session, &beginInfo));

  // 5. Run for 10 seconds of black screen
  for (int i = 0; i < 100000000; i++)
  {
    XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frameState{XR_TYPE_FRAME_STATE};
    xrWaitFrame(session, &waitInfo, &frameState);

    XrFrameBeginInfo frameBegin{XR_TYPE_FRAME_BEGIN_INFO};
    xrBeginFrame(session, &frameBegin);

    // Submit empty frame (black screen)
    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = frameState.predictedDisplayTime;
    endInfo.layerCount = 0; // ZERO layers = black screen
    xrEndFrame(session, &endInfo);
  }

  std::cout << "Done!\n";

  // Cleanup
  xrDestroySession(session);
  xrDestroyInstance(instance);

  return 0;
}