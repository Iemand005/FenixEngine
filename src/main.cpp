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
#include <string>

#include "engine/engine.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
// #include "engine/networking/udp.cpp"
#include "engine/networking/networking.hpp"

glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);

float fov = 45.0f;

float lastX = 0, lastY = 0;

float yaw = -90.0f;
float pitch = 0.0f;

// int windowWidth = 800.0f;
// int windowHeight = 600.0f;

bool vsync = true;

bool capturingMouse = true;

void scrollCallback(GLFWwindow* window, double xoffset, double yOffset) {
  ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureMouse) return;
  fov -= (float)yOffset;
  if (fov < 1.0f) fov = 1.0f;
  if (fov > 45.0f) fov = 45.0f;
}

// void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
//   ImGuiIO& io = ImGui::GetIO();
//   if (io.WantCaptureMouse) return;
//   windowWidth = width;
//   windowHeight = height;
//   glViewport(0, 0, width, height);
// }

class Game {
 public:
  int width;
  int height;
  GLFWwindow* window;
  std::unique_ptr<fe::Scene> scene;
  std::unique_ptr<fe::Camera> playerCamera;
  fe::ShaderProgram* shader;
  fe::Timer fpsCounter;

  std::shared_ptr<fe::Character> player;

  std::vector<std::shared_ptr<fe::Character>> npcs = std::vector<std::shared_ptr<fe::Character>>();

  std::vector<std::shared_ptr<fe::Object>> maps = std::vector<std::shared_ptr<fe::Object>>();

  std::vector<std::string> messages;

  double lastUpdateTime = 0.0f;

  bool canJump = true;

  int mapIndex = 0;

  std::unique_ptr<Networker> client;

  ImGuiIO io;

  Game(int width, int height) : width(width), height(height) {
    if (!initGlfw()) return;
    this->width = width;
    this->height = height;
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);
    // glCullFace(GL_FRONT);

    initImGui();

    this->setClearColor(0.1f, 0.4f, 1.0f, 1.0f);

    this->client = std::make_unique<Networker>(2130);

    client->messageReceiveHandler = [this](std::string message) {
      std::cout << "The server broadcasted a message: " << message << std::endl;
      messages.push_back(message);
    };

    client->connect();

    client->sendPing();

    client->sendMessage("RAWR!!");

    this->scene = std::make_unique<fe::Scene>();
    this->shader = new fe::ShaderProgram("VertexShader.glsl", "FragmentShader.glsl");
    this->playerCamera = std::make_unique<fe::Camera>(cameraPos, cameraFront, cameraUp, fov, (float)this->width / (float)this->height, 0.1f, 100.0f);

    startMouseCapture();

    loadModels();
  }

  void connectToServer(std::string address, unsigned short port) {
    this->client->connect();
  }

  bool initGlfw() {
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

    glfwSwapInterval(vsync ? 1 : 0);  // Enable vsync

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
      std::cout << "Failed to initialize GLAD" << std::endl;
      return false;
    }

    glfwSetWindowUserPointer(window, this);

    glfwSetScrollCallback(window, scrollCallback);
    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
      ImGuiIO& io = ImGui::GetIO();
      if (io.WantCaptureMouse) return;
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xPos, double yPos) {
      if (!(glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)) return;

      ImGuiIO& io = ImGui::GetIO();
      if (io.WantCaptureMouse) return;

      float xOffset = xPos - lastX;
      float yOffset = lastY - yPos;
      if (lastX == 0 && lastY == 0) {
        xOffset = 0;
        yOffset = 0;
      }
      lastX = xPos;
      lastY = yPos;

      const float sensitivity = 0.1f;
      xOffset *= sensitivity;
      yOffset *= sensitivity;

      yaw += xOffset;
      pitch += yOffset;

      if (pitch > 89.0f) pitch = 89.0f;
      if (pitch < -89.0f) pitch = -89.0f;

      glm::vec3 direction;
      direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
      direction.y = sin(glm::radians(pitch));
      direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
      cameraFront = glm::normalize(direction);
    });

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
      auto game = static_cast<Game*>(glfwGetWindowUserPointer(window));
      game->width = width;
      game->height = height;
      glViewport(0, 0, width, height);
      game->updateAspect();

      game->redraw();
    });
    glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int width, int height) {
      auto game = static_cast<Game*>(glfwGetWindowUserPointer(window));
      game->width = width;
      game->height = height;
      glViewport(0, 0, width, height);

      game->updateAspect();
      game->redraw();
    });

    // glfwGetWindowAttrib(window, GLFW_TOUCH);
     return true;
  }

  void loadModels() {
    auto map1 = loadStaticOBJ("resources/models/collisiontest.obj");
    this->scene->addModel(map1);
    this->maps.push_back(map1);

    this->maps.push_back(loadStaticOBJ("resources/testmap/testmappy.obj", 5.0f));

    loadMap(0);

    this->player = std::static_pointer_cast<fe::Character>(loadOBJ("resources/models/citizen.obj", 0.1f));

    spawnZombies(10);
  }

  void loadMap(int index) { scene->getModels()[0] = maps.at(index); }

  void nextMap() {
    loadMap(mapIndex);
    mapIndex++;
    if (mapIndex >= maps.size()) mapIndex = 0;
  }

  void spawnZombies(int count = 10) {
    const float minDistance = 20.0f;
    const float minDistanceSq = minDistance * minDistance;

    auto zombieTemplate = this->player->clone();
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

      auto npc = std::static_pointer_cast<fe::Character>(zombieTemplate->clone());
      npc->position = glm::vec3(x, 0.0f, z);

      if (!npc->meshes.size()) return;

      npc->meshes[0].loadTexture("resources/textures/chau_zombfacemap.png");
      npc->meshes[1].loadTexture("resources/textures/citizenzomb_sheet_reference.png");

      this->scene->addModel(npc);

      npcs.push_back(npc);
    }
  }

  std::shared_ptr<fe::Object> loadOBJ(std::string path, float scale = 1.0f) {
    std::shared_ptr<fe::Object> model = std::make_shared<fe::Object>(path, scale);
    this->scene->addModel(model);
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
    scene->render(*(this->shader), *(this->playerCamera));

    fpsCounter.update();
    drawImGui();

    glfwSwapBuffers(this->window);
  }

  void update() { scene->update(); }

  void processInput() {
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
      this->player->applyForce(glm::vec3(0.0f, 10.0f, 0.0f));
      canJump = false;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
      std::shared_ptr<fe::Object> newObj = this->player->clone();
      newObj->position = this->player->position + horizontalFront * 2.0f;
      glm::vec3 dir = glm::normalize(this->player->position - newObj->position);
      newObj->rotation.y = glm::degrees(atan2(dir.z, dir.x)) - 90.0f;
      newObj->rotation.x = 0.0f;
      this->scene->addModel(newObj);
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

  bool shouldClose() { return glfwWindowShouldClose(this->window); }

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

      static char addressBuffer[256] = "\0";
      int port = 2130;

      ImGui::InputText("##Input", addressBuffer, IM_ARRAYSIZE(addressBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
      ImGui::InputInt("Port", &port);


      if (ImGui::Button("Join", ImVec2(60, 0))) {
        std::cout << "Connecting to server... " << addressBuffer << std::endl;
      }

      fe::Object* model = this->player.get();
      ImGui::SliderFloat3("Position", &model->position.x, -10.0f, 10.0f);

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
    this->playerCamera->setAspect((float)this->width / (float)this->height);
  }
};

int main() {
  Game game(800, 600);

  glm::vec3 playerHeight = glm::vec3(0.0f, 6.5f, 0.0f);

  while (!game.shouldClose()) {
    glfwPollEvents();

    if (game.player->touchedGround) {
      game.canJump = true;
    }
    game.processInput();
    game.player->rotation.y = -yaw + 90.0f;
    glm::vec3 pos = game.player->position + playerHeight;
    cameraPos = pos - cameraFront * 5.0f;
    game.playerCamera->setPos(cameraPos);

    // game.playerCamera->setAspect((float)game.width / (float)game.height);
    game.updateAspect();
    // window.playerCamera->setPos(cameraPos);

    game.playerCamera->setFront(glm::normalize(pos - cameraPos));

    for (auto& npc : game.npcs) {
      npc->lookAt(pos * glm::vec3(1.0f, 0.0f, 1.0f));
      npc->applyVelocity(glm::normalize(pos - npc->position) * glm::vec3(1.0f, 0.0f, 1.0f) * 0.2f * (float)game.getDeltaTime());
      npc->needsUpdate = true;
    }
    for (auto& npc : game.npcs) {
      if (game.player->intersects(*npc)) {
        std::cout << "Player intersects with NPC" << std::endl;
        std::cout << "YOU FUCKING DIED!!!!" << std::endl;
        // client.sendPing();
      }
    }
    game.update();
    game.redraw();
  }

  game.destroy();
  return 0;
}