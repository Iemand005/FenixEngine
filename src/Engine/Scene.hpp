
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

#include "Camera.hpp"
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

  void Render(ShaderProgram shader, Camera const& camera, int width, int height) {
    this->Resize(width, height);
    this->Render(shader, camera);
  }
  
  void Render(ShaderProgram shader, Camera const& camera) {
    this->PrepareRender(shader, camera);

    for (auto& model : objects) model->Render(shader);

    // camera.Render(shader);

    this->EndRender();
  }

  void EnableDepthTest() { glEnable(GL_DEPTH_TEST); }
  void EnableFaceCulling() { glEnable(GL_CULL_FACE); }


  std::vector<std::shared_ptr<Object>>& f() { return objects; }

  std::vector<std::shared_ptr<Object>> GetFilteredObjects(std::shared_ptr<Object> exclude) const {
    std::vector<std::shared_ptr<Object>> filtered;
    std::copy_if(objects.begin(), objects.end(), std::back_inserter(filtered), [exclude](const std::shared_ptr<Object>& obj) {
      return obj != exclude;
    });
    return filtered;
}

  void ClearObjects() { objects.clear(); }

  void AddObject(std::shared_ptr<Object> object) { objects.push_back(object); }


  void PrepareRender(ShaderProgram shader, Camera const& camera) {
    this->Clear();
    shader.Use();
    shader.SetMat4("view", camera.GetViewMatrix());
    shader.SetMat4("projection", camera.GetProjectionMatrix());
  }

  void Clear() { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }


  void EndRender() { glBindVertexArray(0); }

  double Update() {
    auto deltaTime = timer.update();
    for (auto& object : objects) {
      object->Update(deltaTime);
    }
    ResolveCollisions();
    return deltaTime;
  }

  void ResolveCollisions() {
    for (auto& object : objects) {
      if (object->state.position.y < -10.0f) {
        auto pos = object->state.position;
        pos.y = 10;
        object->physicsObject->SetPosition(pos);
        object->physicsObject->SetLinearVelocity(glm::vec3(0.0f));
      }
    }
  }

  double GetDeltaTime() { return timer.deltaTime; }

  void Resize(int width, int height) { glViewport(0, 0, width, height); }
};

} 
