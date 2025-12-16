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

int main()
{
  XrInstance instance;
  XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};

  const char* enabledExtensions[] = { XR_KHR_OPENGL_ENABLE_EXTENSION_NAME };
    createInfo.enabledExtensionCount = 1;
    createInfo.enabledExtensionNames = enabledExtensions;

  createInfo.applicationInfo.apiVersion = XR_API_VERSION_1_0;
  createInfo.applicationInfo.applicationVersion = 1;
  createInfo.applicationInfo.engineVersion = 1;
  strcpy(createInfo.applicationInfo.engineName, "FoxEngine");
  strcpy(createInfo.applicationInfo.applicationName, "FoxEngineTest");

  XrResult result = xrCreateInstance(&createInfo, &instance);
  if (result != XR_SUCCESS)
  {
    if (XR_FAILED(result))
    {
      std::cerr << "Failed to create instance: " << result << "\n";

      // Try to get error info
      char buf[XR_MAX_RESULT_STRING_SIZE];
      if (xrResultToString(nullptr, result, buf) == XR_SUCCESS)
      {
        std::cerr << "Error: " << buf << "\n";
      }
      std::cerr << "Cannot create XR instance. Is OpenXR runtime installed?\n";
      return 1;
    }
  }

  // 2. Get System
  XrSystemId systemId;
  XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
  systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

  if (xrGetSystem(instance, &systemInfo, &systemId) != XR_SUCCESS)
  {
    std::cerr << "No VR headset found\n";
    xrDestroyInstance(instance);
    return 1;
  }

//   glfwInit();

//   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
//   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

//   GLFWwindow *window = glfwCreateWindow(800, 600, "Hi", NULL, NULL);
//   glfwMakeContextCurrent(window);

//   if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
//   {
//     std::cout << "Failed to initialize GLAD" << std::endl;
//     return false;
//   }

//   glClearColor(1, 0, 0, 1);
//   glClear(GL_COLOR_BUFFER_BIT);
// glfwSwapBuffers(window);

//   XrGraphicsBindingOpenGLWin32KHR gfxBinding{};
//   gfxBinding.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR;
//   gfxBinding.hDC = GetDC(glfwGetWin32Window(window));
//   gfxBinding.hGLRC = glfwWGL();
// glfwGetWin32Window

//      HWND hwnd = CreateWindowA(
//         "EDIT", 
//         "OpenXR Binding", 
//         WS_OVERLAPPEDWINDOW,
//         0, 0, 1, 1, 
//         NULL, NULL, 
//         GetModuleHandle(NULL), 
//         NULL
//     );
//     ShowWindow(hwnd, SW_HIDE);
//     HDC hdc = GetDC(hwnd);
    
//     // Setup pixel format
//     // PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR), 1, ... };
//     // int format = ChoosePixelFormat(hdc, &pfd);
//     // SetPixelFormat(hdc, format, &pfd);

//     PIXELFORMATDESCRIPTOR pfd = {
//     sizeof(PIXELFORMATDESCRIPTOR),
//     1,
//     PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
//     PFD_TYPE_RGBA,
//     32, // Color depth
//     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//     24, // Depth buffer
//     8,  // Stencil buffer
    
// };

// int pixelFormat = ChoosePixelFormat(hdc, &pfd);
// if (pixelFormat == 0) {
//     // Handle error
// }

// if (!SetPixelFormat(hdc, pixelFormat, &pfd)) {
//     // Handle error
// }
    
//     // Create REAL WGL context
//     HGLRC hglrc = wglCreateContext(hdc);
//     DWORD error = GetLastError();
//     wglMakeCurrent(hdc, hglrc);
    
//     // Load GLAD
//     gladLoadGL();

glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    auto window = glfwCreateWindow(800, 600, "FoxEngine", NULL, NULL);
    glfwMakeContextCurrent(window);

    HGLRC hglrc = wglGetCurrentContext();
    
    // Now use for OpenXR
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

  // XrGraphicsRequirementsOpenGLKHR vulkanReqs = { XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
  XrGraphicsRequirementsOpenGLKHR glReqs = { XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
// XrResult result = PFN_xrGetOpenGLGraphicsRequirementsKHR(instance, systemId, &vulkanReqs);
// if (xrGetOpenGLGraphicsRequirementsKHR(instance, systemId, &glReqs) != XR_SUCCESS) {
//         // Handle error: Runtime may not support your OpenGL version
//         printf("OpenGL version not supported. Min: %d.%d\n",
//                XR_VERSION_MAJOR(glReqs.minApiVersionSupported),
//                XR_VERSION_MINOR(glReqs.minApiVersionSupported));
//         return -1;
//     }
PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;
    
    result = xrGetInstanceProcAddr(instance,
                                             "xrGetOpenGLGraphicsRequirementsKHR",
                                             (PFN_xrVoidFunction*)(&pfnGetOpenGLGraphicsRequirementsKHR));
    
    if (result != XR_SUCCESS || pfnGetOpenGLGraphicsRequirementsKHR == nullptr) {
        std::cerr << "Failed to get xrGetOpenGLGraphicsRequirementsKHR function pointer. Result: " << result << std::endl;
        xrDestroyInstance(instance);
        return -1;
    }
    std::cout << "Successfully retrieved function pointer." << std::endl;

    // 5. NOW call the function using your pointer
    
    if (pfnGetOpenGLGraphicsRequirementsKHR(instance, systemId, &glReqs) != XR_SUCCESS) {
        std::cerr << "Failed to get OpenGL graphics requirements." << std::endl;
        xrDestroyInstance(instance);
        return -1;
    }
    std::cout << "Successfully got OpenGL requirements." << std::endl;


if (XR_FAILED(result)) { /* Handle error */ }
  
  result = xrCreateSession(instance, &sessionInfo, &session);
  if (result != XR_SUCCESS)
  {
    std::cerr << "Cannot create session (needs graphics binding)\n";
    xrDestroyInstance(instance);
    return 1;
  }

  std::cout << "OpenXR initialized! Starting session (you should see black in headset)...\n";

  // 4. Begin Session
  XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
  beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
  xrBeginSession(session, &beginInfo);

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