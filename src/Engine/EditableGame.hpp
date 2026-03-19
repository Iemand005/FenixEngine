
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
      
      InitImGUI();
      InitUI();
    }

    // virtual void DrawUI();
    
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

    // void DrawUI() override {

    // }

    public:
    void DrawDebugUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Debug");
    {
      ImGui::Text("Hello, World!");
      ImGui::Text("FPS %.1f", fpsCounter.deltaTime > 0.0 ? 1.0 / fpsCounter.deltaTime : 0.0);
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
      ImGui::Text("Objects: %zu", this->scene->GetObjects().size());
      size_t totalVertices = 0;
      for (auto& obj : this->scene->GetObjects())
        for (auto& mesh : obj->meshes) totalVertices += mesh.GetVertices().size();
      ImGui::Text("Vertices: %zu", totalVertices);

      if (ImGui::Button("Enable VR!", ImVec2(100, 20))) {
        this->EnableXR ();
      }

      if (ImGui::Button("Disable VR :()", ImVec2(100, 20))) {
        this->DestroyXR();
      }

      if (ImGui::Button("Enable AA", ImVec2(70, 20))) {
        std::cout << "Button clicked!" << std::endl;
      }

      static bool wireframe = false;
      if (ImGui::Checkbox("Enable Wireframe", &wireframe)) {
        if (wireframe) this->EnableWireframe();
        else this->DisableWireframe();
      }

      fe::Object* model = this->player.get();
      ImGui::SliderFloat3("Position", &model->state.position.x, -10.0f, 10.0f);
      for (size_t i = 0; i < this->npcs.size(); ++i) {
        ImGui::Text("NPC %zu", i);
        ImGui::SliderFloat3(("Position##npc" + std::to_string(i)).c_str(), &this->npcs[i]->state.position.x, -10.0f, 10.0f);
        ImGui::SliderFloat3(("Rotation##npc" + std::to_string(i)).c_str(), &this->npcs[i]->state.rotation.x, -180.0f, 180.0f);
      }
    }
    ImGui::End();

    ImGui::Begin("Objects");
    {
      static char filenameBuffer[512] = "\0";
      static float newObjectScale = 1.0f;

      ImGui::InputText("Model file (.obj)", filenameBuffer, IM_ARRAYSIZE(filenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
      ImGui::DragFloat3("Scale##newObj", &newObjectScale, 0.001f);
      if (ImGui::Button("Load model")) {
        LoadObj(filenameBuffer, newObjectScale);
      }


      static char mapNameBuffer[512] = "level.fes\0";
      ImGui::InputText("Map file", mapNameBuffer, IM_ARRAYSIZE(mapNameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);

      if (ImGui::Button("Save map!"))
        this->SaveLevel();

      if (ImGui::Button("Load map!"))
        this->LoadLevel();

      if (ImGui::Button("Clear objects"))
        this->scene->ClearObjects();

      static bool snapToGrid = true;
      ImGui::Checkbox("Snap to grid", &snapToGrid);
      float step = snapToGrid ? 0.1f : 0.0001f;

      size_t i = 0;
      for (auto &object : scene->GetObjects()) {
        ImGui::Text("Object %zu", i);
        ImGui::DragFloat3(("Position##npc" + std::to_string(i)).c_str(), &object->state.position.x, step);
        ImGui::DragFloat3(("Rotation##npc" + std::to_string(i)).c_str(), &object->state.rotation.x, step);
        ImGui::DragFloat3(("Scale##npc" + std::to_string(i)).c_str(), &object->state.scale.x, step);
        ++i;
      }

      if (ImGui::Button("Add light"))
        this->scene->AddLight();

      auto lights = scene->GetLights();
      for (int i = 0; i < scene->GetLightCount(); ++i) {
        ImGui::Text("Light %zu", i);
        ImGui::DragFloat3(("Position##light" + std::to_string(i)).c_str(), &lights[i].position.x, step);
        ImGui::DragFloat3(("Colour##light" + std::to_string(i)).c_str(), &lights[i].color.x, step);
        ImGui::DragFloat(("Radius##light" + std::to_string(i)).c_str(), &lights[i].radius, step);
        ImGui::DragFloat(("Intensity##light" + std::to_string(i)).c_str(), &lights[i].intensity, step);
      }
    }
    ImGui::End();


    if (this->client) DrawNetworkDebugUI();
    

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void DrawNetworkDebugUI() {
ImGui::Begin("Multiplayer");
    {
      static char usernameBuffer[32] = "Bill\0";
      static char addressBuffer[256] = "127.0.0.1\0";
      int port = 2130;

      ImGui::InputText("Username", usernameBuffer, IM_ARRAYSIZE(usernameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
      ImGui::InputText("Address", addressBuffer, IM_ARRAYSIZE(addressBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
      ImGui::InputInt("Port", &port);

      if (ImGui::Button("Join", ImVec2(60, 0))) {
        std::cout << "Connecting to server... " << addressBuffer << std::endl;
        this->connectToServer(addressBuffer, port, usernameBuffer);
      }

      fe::Object* model = this->player.get();
      ImGui::SliderFloat3("Position", &model->state.position.x, -10.0f, 10.0f);

      ImGui::Text("Players:");
      for (auto& [id, client] : this->client->clientClients) {
        ImGui::Text("Player #%i username: %s", id, client.username.c_str());
      }
    }
    ImGui::End();

    

    ImGui::Begin("Chat");
    {
      static char inputBuffer[256] = "";
      ImGui::BeginChild("ChatHistory", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 10), true, ImGuiWindowFlags_HorizontalScrollbar);

      for (const auto& msg : messages) {
        ImGui::TextWrapped("%s", msg.c_str());
      }

      // Auto-scroll to bottom if new messages
      if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
      }

      ImGui::EndChild();

      ImGui::Separator();

      ImGui::PushItemWidth(-70);
      bool enter_pressed = ImGui::InputText("##Input", inputBuffer, IM_ARRAYSIZE(inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
      ImGui::PopItemWidth();

      ImGui::SameLine();

      bool send_clicked = ImGui::Button("Send", ImVec2(60, 0));

      if (send_clicked || enter_pressed) {
        if (inputBuffer[0] != '\0') {
          messages.push_back(std::string("You: ") + inputBuffer);

#ifdef FE_WIN32

          client->sendMessage(inputBuffer);
          #endif

          inputBuffer[0] = '\0';
          ImGui::SetKeyboardFocusHere(-1);
        }
      }
    }
    ImGui::End();
    }
  
  };
  
} // namespace fe
