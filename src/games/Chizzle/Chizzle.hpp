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
#include <imgui/imgui_impl_sdl3.h>
#include <imgui/imgui_impl_opengl3.h>

#include "../../engine/VRGame.hpp"

class Chizzle : public VRGame {
public:

  std::vector<std::string> messages;

  double lastUpdateTime = 0.0f;

  bool canJump = true;

  int mapIndex = 0;

  ImGuiIO io;

  Chizzle() : Chizzle(800, 640) {}

  Chizzle(int width, int height, bool vr = false) : VRGame(width, height, vr) {

    LoadModels();

    this->physicsEngine->DisableGravity();


    this->client = std::make_unique<Networker>(2130);

    this->client->receiveHandler = [this](PacketData data, PacketType type, const ClientData sender) {
      switch (type) {
        case PacketType::Position: {
          auto packet = data.As<PositionPacket>();
          if (!this->players.count(sender.id)) this->SpawnPlayer(sender.id);
          auto player = this->players.at(sender.id);
          player->state.position = packet.position;
          player->state.rotation = packet.rotation;
        } break;
        case PacketType::ClientList: {
          this->players.clear();
          for (auto& [id, client] : this->client->clientClients) {
            this->SpawnPlayer(id);
            isConnectedToServer = true;
          }
        } break;
      }
    };

    client->messageReceiveHandler = [this](std::string message, ClientData sender) {
      std::cout << "The server broadcasted a messageay: " << message << " Which came from  with username " << sender.username << std::endl;
      messages.push_back(sender.username + ": " + message);
    };

    InitUI();
  }

  void LoadModels() {
    auto map1 = loadStaticOBJ("resources/models/collisiontest.obj");
    this->scene->AddObject(map1);
    this->maps.push_back(map1);


    
    this->maps.push_back(loadStaticOBJ("resources/testmap/testmappy.obj", 5.0f));

    loadMap(0);

    this->player = std::make_shared<fe::Character>();
    this->scene->AddObject(player);

    this->player->SetPhysicsObject(physicsEngine->CreateObject(glm::vec3(1.0f, 1.0f, 1.0f)));

  }

  void ProcessInput() {
    SDL_Event event;
    while (window->PollSDLEvents(&event)) switch (event.type) {
      case SDL_EVENT_QUIT: window->PrepareClose(); break;
      case SDL_EVENT_WINDOW_RESIZED:
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                    // Get actual pixel dimensions
                    int w, h;
                    SDL_GetWindowSizeInPixels(window->GetSDLWindow(), &w, &h);
                    glViewport(0, 0, w, h);
                    // window_changed = 1;
                    break;

      // case SDL_EVENT_WINDOW_RESIZED:{
      //   Resize(event.window.data1, event.window.data2);
      // }

      // break;

      // case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:{
      //   Resize(event.window.data1, event.window.data2);
      // }

      // break;
       }

      //  const bool* keyboardState = SDL_GetKeyboardState(NULL);

      //  if (keyboardState[SDL_SCANCODE_W])KUKU
      //  if (keyboardState[SDL_SCANCODE_W]) this->player->Move(fe::Direction::Forwards, camera.get());
      //  if (keyboardState[SDL_SCANCODE_A]) this->player->Move(fe::Direction::Forwards, camera.get());
      //  if (keyboardState[SDL_SCANCODE_S]) this->player->Move(fe::Direction::Forwards, camera.get());
      //  if (keyboardState[SDL_SCANCODE_D]) this->player->Move(fe::Direction::Forwards, camera.get());
    
      if (window->IsKeyDown(SDL_SCANCODE_W)) this->player->Move(fe::Direction::Forwards, camera.get());
      if (window->IsKeyDown(SDL_SCANCODE_A)) this->player->Move(fe::Direction::Left, camera.get());
      if (window->IsKeyDown(SDL_SCANCODE_S)) this->player->Move(fe::Direction::Backwards, camera.get());
      if (window->IsKeyDown(SDL_SCANCODE_D)) this->player->Move(fe::Direction::Right, camera.get());

      if (window->IsKeyDown(SDL_SCANCODE_SPACE)) this->player->Move(fe::Direction::Up, camera.get());
      if (window->IsKeyDown(SDL_SCANCODE_LSHIFT)) this->player->Move(fe::Direction::Down, camera.get());

      if (window->IsKeyDown(SDL_SCANCODE_ESCAPE)) StopMouseCapture();


    // if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) this->player->Move(fe::Direction::Forwards, camera.get());
    // if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) this->player->Move(fe::Direction::Backwards, camera.get());
    // if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) this->player->Move(fe::Direction::Left, camera.get());
    // if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) this->player->Move(fe::Direction::Right, camera.get());

  // }
    // if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) StopMouseCapture();
    // if (ImGui::GetIO().WantCaptureMouse) {
    //   StopMouseCapture();
    // } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
    //   StartMouseCapture();
    // }

    // const float cameraSpeed = 0.0100f;
    // glm::vec3 horizontalFront = glm::normalize(glm::vec3(camera->front.x, 0.0f, camera->front.z));
    // glm::vec3 right = glm::normalize(glm::cross(horizontalFront, camera->up));

    // glm::vec3 velocity{};
    // glm::vec3 acceleration{};
    // // if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) this->player->physicsComponent->physicsSystem->GetBodyInterface().SetLinearVelocity() += cameraSpeed * horizontalFront;
    
    // if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) velocity -= glm::vec3(0, 1, 0);
    // if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && canJump) velocity -= glm::vec3(0, -1, 0);
    // if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
    //   std::shared_ptr<fe::Object> newObj = this->player->Clone();
    //   newObj->state.position = this->player->state.position + horizontalFront * 2.0f;
    //   glm::vec3 dir = glm::normalize(this->player->state.position - newObj->state.position);
    //   newObj->state.rotation.y = glm::degrees(atan2(dir.z, dir.x)) - 90.0f;
    //   newObj->state.rotation.x = 0.0f;
    //   this->scene->AddObject(newObj);
    // }
    // if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) EnableWireframeMode();
    // if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) DisableWireframeMode();
    // if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)  // Host server
    //   DisableWireframeMode();
    // if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)  // Join server
    //   DisableWireframeMode();

    // if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) glEnable(GL_MULTISAMPLE);
    // if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) glDisable(GL_MULTISAMPLE);
    // // if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
    // //   client->sendPing();

    // static bool ctrlWasDown = false;
    // if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
    //   if (!ctrlWasDown) this->nextMap();
    //   ctrlWasDown = true;
    // } else
    //   ctrlWasDown = false;

    //   this->player->physicsObject->AddPosition(velocity * cameraSpeed);
  }


  void Run() {
    window->DisableVSync();
  
    glm::vec3 cameraOffset = glm::vec3(0);

    SDL_Event event;
    while (!window->ShouldClose()) {

      if (player->touchedGround) {
        canJump = true;
      }
      // while (window->PollSDLEvents(&event))
      ProcessInput();
      player->state.rotation.y = -yaw + 90.0f;
      glm::vec3 pos = player->state.position + cameraOffset;
      camera->SetPos(pos - camera->front * 6.0f);

      camera->setFront(glm::normalize(pos - camera->GetPos()));

      if (isConnectedToServer) client->sendPosition(player->state.position, player->state.rotation);

      Update();
      Redraw();
    }

    Destroy();
  }

  void InitUI() override {
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

  void DrawUI() override {
    ImGui_ImplOpenGL3_NewFrame();
    // ImGui_ImplDSL3_NewFrame();
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
        for (auto& mesh : obj->meshes) totalVertices += mesh.getVertices().size();
      ImGui::Text("Vertices: %zu", totalVertices);

      if (ImGui::Button("Enable VR", ImVec2(50, 20))) {
        this->EnableXR ();
      }

      if (ImGui::Button("Disable VR", ImVec2(50, 20))) {
        this->DestroyXR();
      }

      if (ImGui::Button("Enable AA", ImVec2(50, 20))) {
        std::cout << "Button clicked!" << std::endl;
      }

      static bool wireframe = false;
      if (ImGui::Checkbox("Enable Wireframe", &wireframe)) {
        if (wireframe) this->EnableWireframeMode();
        else this->DisableWireframeMode();
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

      if (ImGui::Button("Save map!")) {
        this->SaveLevel();
      }

      if (ImGui::Button("Load map!")) {
        this->LoadLevel();
      }


      if (ImGui::Button("Clear objects")) {
        this->scene->ClearObjects();
      }

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
    }
    ImGui::End();



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

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  }
};