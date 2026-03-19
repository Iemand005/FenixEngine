
#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <string>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include "XRGame.hpp"

namespace fe
{
  
  class EditableGame : public XRGame {

    
    public:
    EditableGame(int width, int height, bool vr = false) : XRGame(width, height, vr) {
      this->physicsEngine->DisableGravity();
      
      InitUI();
    }
    
  private:
    ImGuiIO io;

  void InitImGUI() {
    fe::SDLWindow *window = (fe::SDLWindow*)this->window.get();
    const char* glsl_version = "#version 330 core";
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    // ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplSDL3_InitForOpenGL(window->GetSDLWindow(), window->GetSDLGLContext());
    ImGui_ImplOpenGL3_Init(glsl_version);
  }
  
  };
  
} // namespace fe
