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
  std::cerr << "Cannot create XR instance. Is OpenXR runtime installed?\n";
}

float cubeVertices[] = {
    // positions          // colors
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,

    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,

     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f
};

// Shader source code
const char* vertexShaderSource = R"(
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

const char* fragmentShaderSource = R"(
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

void initCube() {
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    std::cout << "Cube initialized\n";
}

void drawCube(const glm::mat4& view, const glm::mat4& projection, const glm::mat4& model = glm::mat4(1.0f)) {
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

  XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
  beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
  outputError(xrBeginSession(session, &beginInfo));

  // for (int i = 0; i < 100000000; i++)
  // {
  //   XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
  //   XrFrameState frameState{XR_TYPE_FRAME_STATE};
  //   xrWaitFrame(session, &waitInfo, &frameState);

  //   XrFrameBeginInfo frameBegin{XR_TYPE_FRAME_BEGIN_INFO};
  //   xrBeginFrame(session, &frameBegin);

  //   // Submit empty frame (black screen)
  //   XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
  //   endInfo.displayTime = frameState.predictedDisplayTime;
  //   endInfo.layerCount = 0; // ZERO layers = black screen
  //   xrEndFrame(session, &endInfo);
  // }

  bool running = true;

  while (running)
  {
    // XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
    // while (XR_SUCCEEDED(xrPollEvent(instance, &event)))
    // {
    //   if (event.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)
    //   {
    //     auto *stateChange = reinterpret_cast<XrEventDataSessionStateChanged *>(&event);
    //     if (stateChange->state == XR_SESSION_STATE_STOPPING)
    //     {
    //       running = false;
    //     }
    //   }
    //   event = {XR_TYPE_EVENT_DATA_BUFFER};
    // }

    // if (!running)
    //   break;

    // XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
    // XrFrameState frameState{XR_TYPE_FRAME_STATE};
    // xrWaitFrame(session, &waitInfo, &frameState);

    // XrFrameBeginInfo frameBegin{XR_TYPE_FRAME_BEGIN_INFO};
    // xrBeginFrame(session, &frameBegin);

    // XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    // uint32_t swapchainImageIndex;
    // xrAcquireSwapchainImage(swapchain, &acquireInfo, &swapchainImageIndex);

    // // Wait until image is ready
    // XrSwapchainImageWaitInfo waitImageInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    // waitImageInfo.timeout = XR_INFINITE_DURATION;
    // xrWaitSwapchainImage(swapchain, &waitImageInfo);

    // // 5. Locate views
    // XrViewState viewState{XR_TYPE_VIEW_STATE};
    // XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
    // viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    // viewLocateInfo.displayTime = frameState.predictedDisplayTime;
    // viewLocateInfo.space = appSpace;

    // uint32_t viewCount;
    // xrLocateViews(session, &viewLocateInfo, &viewState, views.size(), &viewCount, views.data());

    // // 6. Render to each eye
    // std::vector<XrCompositionLayerProjectionView> projectionViews(viewCount);

    XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frameState{XR_TYPE_FRAME_STATE};
    xrWaitFrame(session, &waitInfo, &frameState);

    XrFrameBeginInfo frameBegin{XR_TYPE_FRAME_BEGIN_INFO};
    xrBeginFrame(session, &frameBegin);

    // SIMPLE: Render cube to default framebuffer (for testing)
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Simple rotating cube
    static float angle = 0.0f;
    angle += 0.01f;

    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 3.0f), // Camera position
        glm::vec3(0.0f, 0.0f, 0.0f), // Look at origin
        glm::vec3(0.0f, 1.0f, 0.0f)  // Up vector
    );

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f), // FOV
        16.0f / 9.0f,        // Aspect ratio
        0.1f, 100.0f         // Near/far
    );

    glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.5f, 1.0f, 0.0f));
    glm::mat4 mvp = projection * view * model;

    // Render cube (using your cube rendering code)
    renderCube(mvp);

    // End frame with NO layers (still shows something in desktop window)
    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = frameState.predictedDisplayTime;
    endInfo.layerCount = 0; // No XR layers = black in headset
    xrEndFrame(session, &endInfo);

    // Update GLFW window
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