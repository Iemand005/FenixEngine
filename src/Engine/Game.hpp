#pragma once
#define GLFW_INCLUDE_NONE
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "../stdafx.h"

// #include <GLFW/glfw3.h>
// #pragma comment(lib, "glfw3.lib")
#include <glad/glad.h>



#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// #include <imgui/imgui.h>
// #include <imgui/imgui_impl_sdl3.h>
// #include <imgui/imgui_impl_opengl3.h>

#include "engine.h"
#include "networking/networking.hpp"
#include "physics/PhysicsEngine.hpp"
#include "Object.hpp"
#include "Camera.hpp"
#include "ShaderProgram.hpp"
#include "saver/Level.hpp"

#include "window/SDLWindow.hpp"

#define WAYLAND

namespace fe {

class Game {
 public:
  std::unique_ptr<fe::SDLWindow> window;
  std::unique_ptr<fe::Scene> scene;
  std::unique_ptr<fe::Camera> camera;
  std::unique_ptr<fe::ShaderProgram> shader;
  fe::Timer fpsCounter;


  float yaw = -90.0f,  pitch = 0.0f;

  int lastX, lastY;

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

  // ImGuiIO io;

  bool isConnectedToServer = false;

  std::unique_ptr<PhysicsEngine> physicsEngine;

  std::unique_ptr<fe::Level> level;

  Game() : Game(0, 0) {}

  Game(int width, int height, bool bpc10 = true) {
    // if (!InitGlfw(bpc10)) return;
    this->window = std::make_unique<fe::SDLWindow>("Game", width, height);

    this->window->resizeEvent = [this](int width, int height) {
      return;
      this->Resize(width, height);
      this->Redraw();
    };

    this->window->mouseMoveEvent = [this](int x, int y) {
      const float sensitivity = 0.1f;

      this->yaw += sensitivity * x;
      this->pitch += sensitivity * -y;

      if (this->pitch > 89.0f) this->pitch = 89.0f;
      if (this->pitch < -89.0f) this->pitch = -89.0f;

      glm::vec3 direction;
      direction.x = cos(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
      direction.y = sin(glm::radians(this->pitch));
      direction.z = sin(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
      this->camera->front = glm::normalize(direction);
    };

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);


    this->SetClearColor(0.1f, 0.4f, 1.0f, 1.0f);

    this->physicsEngine = std::make_unique<PhysicsEngine>();

    this->scene = std::make_unique<fe::Scene>();
    this->shader = std::make_unique<fe::ShaderProgram>("resources/shaders/VertexShader.glsl", "resources/shaders/FragmentShader.glsl");
    this->camera = std::make_unique<fe::Camera>(45.0f, 0.1f, 100.0f);
    this->level = std::make_unique<fe::Level>();
    
    InitUI();
    
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

  double getDeltaTime() { return 1; }

  void SetClearColor(float r, float g, float b, float a) { glClearColor(r, g, b, a); }

  void Resize() {Resize(this->window->width, this->window->height);}
  void Resize(int width, int height) {
    this->scene->Resize(width, height);
    this->UpdateAspect(width, height);
  }

  void Redraw() {

    // DwmFlush();

    // int x, y;
    // window->GetSize(&x, &y);
    // Resize(x, y);
    

    // scene->Render(*this->shader, *this->camera.get());

    fpsCounter.update();

    // DrawUI();

    window->SwapBuffers();

    // glfwSwapBuffers(this->window);
  }

  void Update() { 
    double dt = scene->Update();
    UpdatePhysics(dt);
   }

  void UpdatePhysics(double deltaTime) {
    physicsEngine->Update(deltaTime);
  }

  void EnableWireframeMode() { glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); }
  void DisableWireframeMode() { glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); }

  void StartMouseCapture() {
    window->StartMouseCapture();
  }

  void StopMouseCapture() {
    // io.WantCaptureMouse = true;
    window->StopMouseCapture();
  }

  virtual void InitUI() {}

  virtual void DrawUI() {}

  void UpdateAspect(int width, int height) {
    if (this->camera) this->camera->SetAspect(width, height);
  }

  bool ShouldClose() { return this->window->ShouldClose(); }

  void Destroy() {
    this->window->Destroy();
  }
};

}