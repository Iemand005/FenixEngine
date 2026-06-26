#pragma once
#define GLFW_INCLUDE_NONE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <cstdio>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <map>
#include <string>

#include <EditableGame.hpp>

class Foxcraft : public fe::EditableGame {
public:
  int mapIndex = 0;


  Foxcraft(int width, int height) : fe::EditableGame(width, height) {

  }

  void LoadModels() {
    auto map1 = loadStaticOBJ("resources/models/collisiontest.obj");
    this->scene->AddObject(map1);
    this->maps.push_back(map1);

    this->maps.push_back(loadStaticOBJ("resources/testmap/testmappy.obj", 5.0f));

    loadMap(0);

    this->player = std::static_pointer_cast<fe::Character>(LoadObj("resources/models/citizen.obj", 0.1f));

    // spawnZombies(10);
  }

  void loadMap(int index) { scene->GetObjects()[0] = maps.at(index); }

  void nextMap() {
    loadMap(mapIndex);
    mapIndex++;
    if (mapIndex >= maps.size()) mapIndex = 0;
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

      float dx = x - player->position.x;
      float dz = z - player->position.z;
      float distanceSq = dx * dx + dz * dz;

      if (distanceSq < minDistanceSq) {
        x += minDistanceSq;
        z += minDistanceSq;
      }

      auto npc = std::static_pointer_cast<fe::Character>(zombieTemplate->Clone());
      npc->position = glm::vec3(x, 0.0f, z);

      this->scene->AddObject(npc);

      npcs.push_back(npc);
    }
  }

  void spawnPlayer(u_char playerId) {
    auto newPlayer = std::static_pointer_cast<fe::Character>(this->player->Clone());

    this->players.insert_or_assign(playerId, newPlayer);
    this->scene->AddObject(newPlayer);
  }

  std::shared_ptr<fe::Object> LoadObj(std::string path, float scale = 1.0f) {
    std::shared_ptr<fe::Object> model = std::make_shared<fe::Object>(path, scale);
    this->scene->AddObject(model);
    return model;
  }

  std::shared_ptr<fe::Object> loadOBJButDontAdd(std::string path, float scale = 1.0f) { return std::make_shared<fe::Object>(path, scale); }

  std::shared_ptr<fe::Object> loadStaticOBJ(std::string path, float scale = 1.0f) {
    std::shared_ptr<fe::Object> model = std::make_shared<fe::Object>(path, scale);
    model->isStatic = true;
    model->needsUpdate = false;
    // this->scene->addModel(model);

    return model;
  }

  double getDeltaTime() { return glfwGetTime(); }

  void setClearColor(float r, float g, float b, float a) { glClearColor(r, g, b, a); }

  void redraw() {
    scene->Render(*(this->shader), *(this->camera));

    fpsCounter.update();
    drawImGui();

    glfwSwapBuffers(this->window);
  }

  void update() { scene->Update(); }

  void ProcessInput() {
    double deltaTime = scene->getDeltaTime();

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) stopMouseCapture();
    if (ImGui::GetIO().WantCaptureMouse) {
      stopMouseCapture();
    } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
      startMouseCapture();
    }

    const float cameraSpeed = 10.0f * deltaTime;
    glm::vec3 horizontalFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
    glm::vec3 right = glm::normalize(glm::cross(horizontalFront, cameraUp));
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) this->player->position += cameraSpeed * horizontalFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) this->player->position -= cameraSpeed * horizontalFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) this->player->position -= right * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) this->player->position += right * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) this->player->position += cameraUp * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) this->player->position -= cameraUp * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && canJump) {
      // this->player->acceleration.y = 10.0f;
      this->player->ApplyForce(glm::vec3(0.0f, 10.0f, 0.0f));
      canJump = false;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
      std::shared_ptr<fe::Object> newObj = this->player->Clone();
      newObj->position = this->player->position + horizontalFront * 2.0f;
      glm::vec3 dir = glm::normalize(this->player->position - newObj->position);
      newObj->rotation.y = glm::degrees(atan2(dir.z, dir.x)) - 90.0f;
      newObj->rotation.x = 0.0f;
      this->scene->AddObject(newObj);
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) enableWireframeMode();
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) disableWireframeMode();
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)  // Host server
      disableWireframeMode();
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)  // Join server
      disableWireframeMode();

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
      if (!pWasDown) client->sendPing();
      pWasDown = true;
    } else
      pWasDown = false;
  }

  void enableWireframeMode() { glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); }
  void disableWireframeMode() { glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); }

  void startMouseCapture() {
    // return;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }

  void stopMouseCapture() {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    // glfwSetCursorPosCallback(window, NULL);
  }

  bool ShouldClose() { return glfwWindowShouldClose(this->window); }

  void destroy() {
    glfwDestroyWindow(this->window);
    glfwTerminate();
  }

  void initImGui() {
    const char* glsl_version = "#version 330 core";
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
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
      ImGui::Text("Objects: %zu", this->scene->GetObjects().size());
      size_t totalVertices = 0;
      for (auto& obj : this->scene->GetObjects())
        for (auto& mesh : obj->meshes) totalVertices += mesh.getVertices().size();
      ImGui::Text("Vertices: %zu", totalVertices);
      size_t needsUpdateCount = 0;
      for (auto& obj : this->scene->GetObjects()) {
        if (obj->needsUpdate) needsUpdateCount++;
      }
      ImGui::Text("Needs Update: %zu", needsUpdateCount);
      if (ImGui::Button("Start", ImVec2(50, 20))) {
        std::cout << "Button clicked!" << std::endl;
      }

      if (ImGui::Button("Enable AA", ImVec2(50, 20))) {
        std::cout << "Button clicked!" << std::endl;
      }

      fe::Object* model = this->player.get();
      ImGui::SliderFloat3("Position", &model->position.x, -10.0f, 10.0f);
      for (size_t i = 0; i < this->npcs.size(); ++i) {
        ImGui::Text("NPC %zu", i);
        ImGui::SliderFloat3(("Position##npc" + std::to_string(i)).c_str(), &this->npcs[i]->position.x, -10.0f, 10.0f);
        ImGui::SliderFloat3(("Rotation##npc" + std::to_string(i)).c_str(), &this->npcs[i]->rotation.x, -180.0f, 180.0f);
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
      ImGui::SliderFloat3("Position", &model->position.x, -10.0f, 10.0f);

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

          client->sendMessage(inputBuffer);

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

  void updateAspect() {
    if (this->camera) this->camera->setAspect((float)this->width / (float)this->height);
  }
};

