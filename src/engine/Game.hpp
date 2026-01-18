#pragma once
#define GLFW_INCLUDE_NONE
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "../stdafx.h"

#include <GLFW/glfw3.h>
#pragma comment(lib, "glfw3.lib")
#include <glad/glad.h>



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
#include "saver/Level.hpp"

#define WAYLAND

class Game {
 public:
  int width;
  int height;
  GLFWwindow* window;
  std::unique_ptr<fe::Scene> scene;
  std::unique_ptr<fe::Camera> camera;
  std::unique_ptr<fe::ShaderProgram> shader;
  fe::Timer fpsCounter;

  // glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
  // glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
  // glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
  // glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);

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

  std::unique_ptr<fe::Level> level;

  Game() : Game(0, 0) {}

  Game(int width, int height, bool bpc10 = true) : width(width), height(height) {
    if (!InitGlfw(bpc10)) return;
    this->width = width;
    this->height = height;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);


    this->SetClearColor(0.1f, 0.4f, 1.0f, 1.0f);

    this->physicsEngine = std::make_unique<PhysicsEngine>();

    this->scene = std::make_unique<fe::Scene>();
    this->shader = std::make_unique<fe::ShaderProgram>("resources/shaders/VertexShader.glsl", "resources/shaders/FragmentShader.glsl");
    this->camera = std::make_unique<fe::Camera>(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), fov, (float)this->width / (float)this->height, 0.1f, 100.0f);
    this->level = std::make_unique<fe::Level>();


    UpdateAspect();
    
    InitImGui();
    
    StartMouseCapture();
  }

  void SaveLevel(std::string fileName = "level.fes") {
    this->level->Save(this->scene->GetFilteredObjects(player), fileName);
  }

  void LoadLevel(std::string fileName = "level.fes") {
    auto objects = this->level->Load(fileName);
    this->scene->ClearObjects();
    this->scene->AddObject(player);
    for (auto &object : objects)
      this->scene->AddObject(object);
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

  bool InitGlfw(bool tenBit = false) {
#ifdef WAYLAND
    if (glfwPlatformSupported(GLFW_PLATFORM_WAYLAND)) {
      glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
    } else {
      std::cerr << "No Wayland Support" << std::endl;
    }
#endif

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if (tenBit) {
      glfwWindowHint(GLFW_RED_BITS, 10);
      glfwWindowHint(GLFW_GREEN_BITS, 10);
      glfwWindowHint(GLFW_BLUE_BITS, 10);
      glfwWindowHint(GLFW_ALPHA_BITS, 2);
    }

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
      if (!(glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)) {
        // ImGui::SetNextFrameWantCaptureMouse(false);
        return;
      }

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
      game->camera->front = glm::normalize(direction);
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
    this->scene->Resize(width, height);
    this->UpdateAspect();
  }

  void Resize() {
    Resize(width, height);
  }

  void loadMap(int index) {
    auto map = maps.at(index);
    scene->GetObjects()[0] = map;

    for (auto &mesh : map->meshes) {
      auto vertices = std::vector<glm::vec3>();
      for (auto &vertex : mesh.vertices)
        vertices.push_back(vertex.position);
      mesh.SetPhysicsObject(physicsEngine->CreateObject(vertices, mesh.indices));
    }

  }

  void nextMap() {
    loadMap(mapIndex);
    mapIndex++;
    if (mapIndex >= maps.size()) mapIndex = 0;
  }
  
  void SpawnPlayer(u_char playerId) {
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
    return model;
  }

  double getDeltaTime() { return glfwGetTime(); }

  void SetClearColor(float r, float g, float b, float a) { glClearColor(r, g, b, a); }

  void Redraw() {
    scene->Render(*this->shader, *this->camera.get());

    fpsCounter.update();

    DrawUI();

    glfwSwapBuffers(this->window);
  }

  void Update() { 
    double dt = scene->Update();
    UpdatePhysics(dt);
   }

  void UpdatePhysics(double deltaTime) {
    physicsEngine->Update(deltaTime);
  }

  virtual void ProcessMovementInput() {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) this->player->Move(fe::Direction::Forwards, camera.get());
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) this->player->Move(fe::Direction::Backwards, camera.get());
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) this->player->Move(fe::Direction::Left, camera.get());
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) this->player->Move(fe::Direction::Right, camera.get());

  }

  void EnableWireframeMode() { glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); }
  void DisableWireframeMode() { glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); }

  void StartMouseCapture() {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    io.WantCaptureMouse = false;
  }

  void StopMouseCapture() {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    io.WantCaptureMouse = true;

  }

  void InitImGui() {
    const char* glsl_version = "#version 330 core";
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
  }

  virtual void DrawUI() {};

  void UpdateAspect() {
    if (this->camera) this->camera->setAspect((float)this->width / (float)this->height);
  }

  bool ShouldClose() { return glfwWindowShouldClose(this->window); }

  void destroy() {
    glfwDestroyWindow(this->window);
    glfwTerminate();
  }
};