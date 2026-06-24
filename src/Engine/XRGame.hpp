#include "Game.hpp"

#pragma once
#define XR_USE_GRAPHICS_API_OPENGL
#ifdef WIN32
#define XR_USE_PLATFORM_WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
// #define XR_EXTENSION_PROTOTYPES
// #define XR_KHR_opengl_enable
#ifdef XR_USE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <unknwn.h>
#endif
#else
#define XR_USE_PLATFORM_WAYLAND
#endif
// #include <openxr/openxr.h>
// #include <openxr/openxr_platform.h>
// #include <GL/glew.h>
// #define GLFW_INCLUDE_NONE
// #include <GLFW/glfw3.h>
// #include <GLFW/glfw3native.h>

#include <cstring>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>
#include <vector>

#include "engine.h"

typedef int64_t XrTime;

namespace fe {
  class XRGame : public fe::Game {
  private:
    struct Impl;
    std::unique_ptr<Impl> impl;

  public:

    float playerHeight = 1.7f;

    bool drawWindow = true;

    glm::vec3 positionOffset = glm::vec3(1.0f);

    bool running = true;
    
    uint32_t swapchainImageIndex;

    XRGame(bool launchVR = true);
    XRGame(int width, int height, bool launchVR = true, bool drawWindow = true);
    XRGame(GLADloadproc loadProc);
    ~XRGame();

    bool IsInstanceValid();

    void initOpenXR();
    void initOpenXR(void *next);
#ifdef WIN32
    void initOpenXR(HDC hDC, HGLRC hGLRC);
#endif
    
    void EnableXR();
    void DisableVR();
    void LaunchVR();

    void Redraw(GLuint fbo = 0) ;
    void RedrawVR();
    void RedrawWindow(GLuint fbo = 0);

    void PollActionsAndUpdateMovement(XrTime predictedDisplayTime);

    void DestroyXR();

    void Destroy() {
      DestroyXR();
      if (window) window->Destroy();
    }
  };
}
