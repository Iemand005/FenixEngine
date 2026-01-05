#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <cstdio>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <map>
#include <string>

#include "../../engine/VRGame.hpp"

class Chizzle : public VRGame {
public:

  std::vector<std::string> messages;

  double lastUpdateTime = 0.0f;

  bool canJump = true;

  int mapIndex = 0;


  Chizzle(int width, int height) : VRGame(width, height) {

    loadModels();


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
  }

  void loadModels() {
    auto map1 = loadStaticOBJ("resources/models/collisiontest.obj");
    this->scene->AddModel(map1);
    this->maps.push_back(map1);

    // map1->physicsComponent = new fe::PhysicsObject(physicsEngine->physicsSystem);

    for (auto &mesh : map1->meshes) {
      auto vertices = std::vector<glm::vec3>(mesh.vertices.size());
      for (auto &vertex : mesh.vertices)
        vertices.push_back(vertex.position);
      mesh.SetPhysicsObject(physicsEngine->CreateObject(vertices, mesh.indices));
    }

    // this->maps.push_back(loadStaticOBJ("resources/testmap/testmappy.obj", 5.0f));

    loadMap(0);

    this->player = std::static_pointer_cast<fe::Character>(loadOBJ("resources/models/citizen.obj", 0.1f));

    this->player->SetPhysicsObject(physicsEngine->CreateObject());


    // spawnZombies(10);
  }

  void spawnZombies(int count = 10) {
    const float minDistance = 20.0f;
    const float minDistanceSq = minDistance * minDistance;

    auto zombieTemplate = std::static_pointer_cast<fe::Character>(this->player->Clone());
    if (zombieTemplate->meshes.size() < 2) return;

    zombieTemplate->meshes[0].loadTexture("resources/textures/chau_zombfacemap.png");
    zombieTemplate->meshes[1].loadTexture("resources/textures/citizenzomb_sheet_reference.png");
    for (int i = 0; i < count; i++) {
      float x = static_cast<float>(rand() % 100 - 50);
      float z = static_cast<float>(rand() % 100 - 50);

      float dx = x - player->state.position.x;
      float dz = z - player->state.position.z;
      float distanceSq = dx * dx + dz * dz;

      if (distanceSq < minDistanceSq) {
        x += minDistanceSq;
        z += minDistanceSq;
      }

      auto npc = std::static_pointer_cast<fe::Character>(zombieTemplate->Clone());
      npc->state.position = glm::vec3(x, 0.0f, z);

      this->scene->AddModel(npc);

      npcs.push_back(npc);
    }
  }

  void ProcessInput() {
    // double deltaTime = scene->getDeltaTime()

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) StopMouseCapture();
    if (ImGui::GetIO().WantCaptureMouse) {
      StopMouseCapture();
    } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
      StartMouseCapture();
    }

    const float cameraSpeed = 0.0100f;
    glm::vec3 horizontalFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
    glm::vec3 right = glm::normalize(glm::cross(horizontalFront, cameraUp));

    glm::vec3 velocity{};
    glm::vec3 acceleration{};
    // if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) this->player->physicsComponent->physicsSystem->GetBodyInterface().SetLinearVelocity() += cameraSpeed * horizontalFront;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) velocity += horizontalFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) velocity -= horizontalFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) velocity -= right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) velocity += right;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) velocity += cameraUp;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) velocity -= cameraUp;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && canJump) {
      // this->player->acceleration.y = 10.0f;
      // this->player->applyForce(glm::vec3(0.0f, 10.0f, 0.0f));
      acceleration.y = 10;
      canJump = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
      std::shared_ptr<fe::Object> newObj = this->player->Clone();
      newObj->state.position = this->player->state.position + horizontalFront * 2.0f;
      glm::vec3 dir = glm::normalize(this->player->state.position - newObj->state.position);
      newObj->state.rotation.y = glm::degrees(atan2(dir.z, dir.x)) - 90.0f;
      newObj->state.rotation.x = 0.0f;
      this->scene->AddModel(newObj);
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) EnableWireframeMode();
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) DisableWireframeMode();
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)  // Host server
      DisableWireframeMode();
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)  // Join server
      DisableWireframeMode();

    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) glEnable(GL_MULTISAMPLE);
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) glDisable(GL_MULTISAMPLE);
    // if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
    //   client->sendPing();

    static bool ctrlWasDown = false;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
      if (!ctrlWasDown) this->nextMap();
      ctrlWasDown = true;
    } else
      ctrlWasDown = false;

    static bool pWasDown = false;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
      // if (!pWasDown) client->sendPing();
      pWasDown = true;
    } else
      pWasDown = false;

      // this->player->physicsObject->AddLinearVelocity(acceleration);
      this->player->physicsObject->AddLinearVelocity(velocity * cameraSpeed);
  }

  bool ShouldClose() { return glfwWindowShouldClose(this->window); }

  void Run() {
    DisableVSync();
  
    glm::vec3 cameraOffset = glm::vec3(0.0f, 6.5f, 0.0f);

    while (!ShouldClose()) {
  //     std::cout << "DEBUG: Before glfwPollEvents()" << std::endl;
  // std::cout << "DEBUG: Window pointer: " << window << std::endl;
      glfwPollEvents();

      if (player->touchedGround) {
        canJump = true;
      }
      ProcessInput();
      player->state.rotation.y = -yaw + 90.0f;
      glm::vec3 pos = player->state.position + cameraOffset;
      cameraPos = pos - cameraFront * 5.0f;
      camera->SetPos(cameraPos);

      camera->setFront(glm::normalize(pos - cameraPos));

      if (isConnectedToServer) client->sendPosition(player->state.position, player->state.rotation);

      Update();
      Redraw();
      RedrawVR();
    }

    destroy();
  }

  int drawImGui() {
    // glDisable(GL_DEPTH_TEST);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Debug info");
    {
      ImGui::Text("Hello, World!");
      ImGui::Text("FPS %.1f", fpsCounter.deltaTime > 0.0 ? 1.0 / fpsCounter.deltaTime : 0.0);
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
      ImGui::Text("Objects: %zu", this->scene->getModels().size());
      size_t totalVertices = 0;
      for (auto& obj : this->scene->getModels())
        for (auto& mesh : obj->meshes) totalVertices += mesh.getVertices().size();
      ImGui::Text("Vertices: %zu", totalVertices);

      if (ImGui::Button("Enable AA", ImVec2(50, 20))) {
        std::cout << "Button clicked!" << std::endl;
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
    // glEnable(GL_DEPTH_TEST);
    return 0;
  }
};