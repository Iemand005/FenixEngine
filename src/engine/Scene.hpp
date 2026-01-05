
#pragma once
#include <glad/glad.h>

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Timer.hpp"
#include "Mesh.hpp"
#include "ShaderProgram.hpp"
#include "physics/PhysicsEngine.hpp"

namespace fe {

class Scene {
 private:
  std::vector<std::shared_ptr<Object>> objects;
  glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);
  Timer timer;

 public:
  Scene() {
    objects = std::vector<std::shared_ptr<Object>>();
    this->EnableDepthTest();
    this->EnableFaceCulling();
  }

  void Render(ShaderProgram shader, Camera &camera, int width, int height) {
    this->Resize(width, height);
    this->Render(shader, camera);
  }
  
  void Render(ShaderProgram shader, Camera &camera) {
    this->PrepareRender(shader, camera);

    for (auto& model : objects) model->render(shader);

    camera.Render(shader);

    this->EndRender();
  }

  void EnableDepthTest() { glEnable(GL_DEPTH_TEST); }

  void EnableFaceCulling() { glEnable(GL_CULL_FACE); }

  void AddObject(std::shared_ptr<Object> object) { objects.push_back(object); }

  void PrepareRender(ShaderProgram shader, Camera camera) {
    this->Clear();
    shader.Use();
    shader.SetMat4("view", camera.GetViewMatrix());
    shader.SetMat4("projection", camera.GetProjectionMatrix());
  }

  void Clear() { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }


  void EndRender() { glBindVertexArray(0); }

  double update() {
    auto deltaTime = timer.update();
    for (auto& object : objects) {
      object->Update(deltaTime);
    }
    ResolveCollisions();
    return deltaTime;
  }

  void ResolveCollisions() {
    for (auto& object : objects) {
      if (object->state.position.y < -200.0f) {
        auto pos = object->state.position;
        pos.y = 10;
        object->physicsObject->SetPosition(pos);
        object->physicsObject->SetLinearVelocity(glm::vec3(0.0f));
      }
    }
  }

  std::vector<std::shared_ptr<Object>>& GetObjects() { return objects; }

  double GetDeltaTime() { return timer.deltaTime; }

  void Resize(int width, int height) { glViewport(0, 0, width, height); }
};

} 