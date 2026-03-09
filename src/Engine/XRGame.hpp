#include "Game.hpp"

#pragma once
#define XR_USE_GRAPHICS_API_OPENGL
#define XR_USE_PLATFORM_WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
// #define XR_EXTENSION_PROTOTYPES
// #define XR_KHR_opengl_enable
#ifdef XR_USE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <unknwn.h>
#endif
// #include <openxr/openxr.h>
// #include <openxr/openxr_platform.h>
// #include <GL/glew.h>
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

#include "engine.h"

class XRGame : public fe::Game {
 private:
  bool drawVR = false;
  // bool vrInitialized = false;
  struct Impl;
  std::unique_ptr<Impl> impl;

 public:
  
  std::vector<GLuint> depthTextures;
  std::vector<GLuint> framebuffers;
  uint32_t viewCount = 2;  // Stereo
  int32_t swapchainWidth, swapchainHeight;

  

  float playerHeight = 1.7f;

  bool drawWindow = true;

  glm::vec3 positionOffset = glm::vec3(1.0f);

  bool running = true;
  
  uint32_t swapchainImageIndex;

  XRGame(bool launchVR = true) : XRGame(0, 0, false) { LaunchVR(); }

  XRGame(int width, int height, bool launchVR = true, bool drawWindow = true) : Game(width, height) {
    this->drawWindow = drawWindow;
    if (launchVR) LaunchVR();
  }

  void EnableXR() {
    if (!IsInstanceValid()) LaunchVR();
    if (IsInstanceValid()) drawVR = true;
  }

  bool IsInstanceValid();

  void DisableVR();

  void LaunchVR();

  void RedrawWindow();

  void Redraw() ;

  

  void CheckGLError(const char* location) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
      std::cerr << "OpenGL error at " << location << ": " << err << std::endl;
    }
  }

  void DestroyXR();

  void Destroy() {
    DestroyXR();
    this->window->Destroy();
  }
};