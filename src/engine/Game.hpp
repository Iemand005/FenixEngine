#pragma once
#define GLFW_INCLUDE_NONE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <GLFW/glfw3.h>
#pragma comment(lib, "glfw3.lib")
#include <glad/glad.h>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "engine.h"
#include "networking/networking.hpp"
#include "physics/PhysicsEngine.hpp"
#include "Object.hpp"
#include "Camera.hpp"
#include "ShaderProgram.hpp"

class Game {
 public:
  int width;
  int height;
  GLFWwindow* window;
  std::unique_ptr<fe::Scene> scene;
  std::unique_ptr<fe::Camera> camera;
  std::unique_ptr<fe::ShaderProgram> shader;
  fe::Timer fpsCounter;

  glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
  glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);

  float fov = 45.0f;

  float lastX = 0, lastY = 0;

  float yaw = -90.0f;
  float pitch = 0.0f;

  bool vsync = true;

  bool capturingMouse = true;

  std::shared_ptr<fe::Character> player;

  std::vector<std::shared_ptr<fe::Character>> npcs = std::vector<std::shared_ptr<fe::Character>>();

  std::vector<std::shared_ptr<fe::Object>> maps = std::vector<std::shared_ptr<fe::Object>>();

  std::vector<std::string> messages;

  double lastUpdateTime = 0.0f;

  bool canJump = true;

  int mapIndex = 0;

  std::unique_ptr<Networker> client = nullptr;

  std::unordered_map<u_char, std::shared_ptr<fe::Character>> players = std::unordered_map<unsigned char, std::shared_ptr<fe::Character>>();

  ImGuiIO io;

  bool isConnectedToServer = false;

  std::unique_ptr<PhysicsEngine> physicsEngine;

  Game(int width, int height) : width(width), height(height) {
    if (!InitGlfw()) return;
    this->width = width;
    this->height = height;
    // glViewport(0, 0, width, height);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);
    // glCullFace(GL_FRONT);


    this->SetClearColor(0.1f, 0.4f, 1.0f, 1.0f);

    this->physicsEngine = std::make_unique<PhysicsEngine>();

    this->client = std::make_unique<Networker>(2130);

    this->client->receiveHandler = [this](PacketData data, PacketType type, const ClientData sender) {
      switch (type) {
        case PacketType::Position: {
          auto packet = data.As<PositionPacket>();
          if (!this->players.count(sender.id)) this->SpawnPlayer(sender.id);
          auto player = this->players.at(sender.id);
          player->position = packet.position;
          player->rotation = packet.rotation;
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
      // std::cout ;
      messages.push_back(sender.username + ": " + message);
    };

    this->scene = std::make_unique<fe::Scene>();
    // physicsEngine
    this->shader = std::make_unique<fe::ShaderProgram>("resources/shaders/VertexShader.glsl", "resources/shaders/FragmentShader.glsl");
    this->camera = std::make_unique<fe::Camera>(cameraPos, cameraFront, cameraUp, fov, (float)this->width / (float)this->height, 0.1f, 100.0f);


    updateAspect();
    
    initImGui();
    
    StartMouseCapture();

    // loadModels(); Letter U
  }

  void connectToServer(std::string address, unsigned short port, std::string username) {
    // this->client->username = username;
    // if (!this->client)

    this->client->Connect(address, port, username);
    // isConnectedToServer =true;
  }

  void SetSwapInterval(int interval) {
    glfwSwapInterval(interval);
  }

  void EnableVSync() {
    SetSwapInterval(1);
  }

  void DisableVSync() {
    SetSwapInterval(0);
  }

  bool InitGlfw() {
    // if (glfwPlatformSupported(GLFW_PLATFORM_WAYLAND)) {
    //   glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
    // } else {
    //   std::cerr << "No Wayland Support" << std::endl;
    // }

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    this->window = glfwCreateWindow(width, height, "FoxEngine", NULL, NULL);
    if (window == NULL) {
      std::cout << "Failed to create GLFW window" << std::endl;
      glfwTerminate();
      return false;
    }
    glfwMakeContextCurrent(window);

    // glfwSwapInterval(vsync ? 1 : 0);  // Enable vsync
    EnableVSync();

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
      std::cout << "Failed to initialize GLAD" << std::endl;
      return false;
    }

    glfwSetWindowUserPointer(window, this);

    glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yOffset) {
      auto game = static_cast<Game*>(glfwGetWindowUserPointer(window));
      ImGuiIO& io = ImGui::GetIO();
      if (io.WantCaptureMouse) return;
      game->fov -= (float)yOffset;
      if (game->fov < 1.0f) game->fov = 1.0f;
      if (game->fov > 45.0f) game->fov = 45.0f;
    });
    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
      ImGuiIO& io = ImGui::GetIO();
      if (io.WantCaptureMouse) return;
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xPos, double yPos) {
      if (!(glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)) return;

      auto game = static_cast<Game*>(glfwGetWindowUserPointer(window));
      ImGuiIO& io = ImGui::GetIO();
      if (io.WantCaptureMouse) return;

      float xOffset = xPos - game->lastX;
      float yOffset = game->lastY - yPos;
      if (game->lastX == 0 && game->lastY == 0) {
        xOffset = 0;
        yOffset = 0;
      }
      game->lastX = xPos;
      game->lastY = yPos;

      const float sensitivity = 0.1f;
      xOffset *= sensitivity;
      yOffset *= sensitivity;

      game->yaw += xOffset;
      game->pitch += yOffset;

      if (game->pitch > 89.0f) game->pitch = 89.0f;
      if (game->pitch < -89.0f) game->pitch = -89.0f;

      glm::vec3 direction;
      direction.x = cos(glm::radians(game->yaw)) * cos(glm::radians(game->pitch));
      direction.y = sin(glm::radians(game->pitch));
      direction.z = sin(glm::radians(game->yaw)) * cos(glm::radians(game->pitch));
      game->cameraFront = glm::normalize(direction);
    });

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
      auto game = static_cast<Game*>(glfwGetWindowUserPointer(window));
      // game->width = width;
      // game->height = height;
      // game->scene->resize(width, height);

      // game->updateAspect();
      game->Resize(width, height);

      game->Redraw();
    });
    glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int width, int height) {
      auto game = static_cast<Game*>(glfwGetWindowUserPointer(window));
      game->Resize(width, height);
      game->Redraw();
    });

    // glfwGetWindowAttrib(window, GLFW_TOUCH);
    return true;
  }

  void Resize(int width, int height) {
    this->width = width;
    this->height = height;
    this->scene->resize(width, height);
    this->updateAspect();
  }

  void loadMap(int index) { scene->getModels()[0] = maps.at(index); }

  void nextMap() {
    loadMap(mapIndex);
    mapIndex++;
    if (mapIndex >= maps.size()) mapIndex = 0;
  }
  
  void SpawnPlayer(u_char playerId) {
    auto newPlayer = std::static_pointer_cast<fe::Character>(this->player->Clone());

    this->players.insert_or_assign(playerId, newPlayer);
    this->scene->AddModel(newPlayer);
  }

  std::shared_ptr<fe::Object> loadOBJ(std::string path, float scale = 1.0f) {
    std::shared_ptr<fe::Object> model = std::make_shared<fe::Object>(path, scale);
    this->scene->AddModel(model);
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

  void SetClearColor(float r, float g, float b, float a) { glClearColor(r, g, b, a); }

  void Redraw() {
    scene->Render(*(this->shader), *this->camera.get());

    fpsCounter.update();
    drawImGui();

    glfwSwapBuffers(this->window);
  }

  void Update() { 
    double dt = scene->update();
    UpdatePhysics(dt);
   }

  void UpdatePhysics(double deltaTime) {
    physicsEngine->Update(deltaTime);
  }

  void ProcessInput() {
    double deltaTime = scene->getDeltaTime();

    physicsEngine->Update(deltaTime);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) StopMouseCapture();
    if (ImGui::GetIO().WantCaptureMouse) {
      StopMouseCapture();
    } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
      StartMouseCapture();
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
  }

  void EnableWireframeMode() { glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); }
  void DisableWireframeMode() { glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); }

  void StartMouseCapture() { glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); }

  void StopMouseCapture() { glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); }

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
      ImGui::Text("Objects: %zu", this->scene->getModels().size());
      size_t totalVertices = 0;
      for (auto& obj : this->scene->getModels())
        for (auto& mesh : obj->meshes) totalVertices += mesh.getVertices().size();
      ImGui::Text("Vertices: %zu", totalVertices);
      size_t needsUpdateCount = 0;
      for (auto& obj : this->scene->getModels()) {
        if (obj->needsUpdate) needsUpdateCount++;
      }
      ImGui::Text("Needs Update: %zu", needsUpdateCount);
      if (ImGui::Button("Start", ImVec2(50, 20))) {
        std::cout << "Button clicked!" << std::endl;
      }

      if (ImGui::Button("Enable AA", ImVec2(50, 20))) {
        std::cout << "Button clicked!" << std::endl;
      }

      fe::Object* character = this->player.get();
      ImGui::SliderFloat3("Position", &character->position.x, -10.0f, 10.0f);
      ImGui::SliderFloat3("Rotation", &character->rotation.x, -10.0f, 10.0f);
      ImGui::SliderFloat3("Velocity", &character->velocity.x, -10.0f, 10.0f);
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

  bool ShouldClose() { return glfwWindowShouldClose(this->window); }

  void destroy() {
    glfwDestroyWindow(this->window);
    glfwTerminate();
  }
};