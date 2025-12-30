
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
    this->enableDepthTest();
    this->enableFaceCulling();
  }

  void render(ShaderProgram shader, const Camera camera, int width, int height) {
      glViewport(0, 0, width, height);
      this->Render(shader, camera);
    }

  void enableDepthTest() { glEnable(GL_DEPTH_TEST); }

  void enableFaceCulling() { glEnable(GL_CULL_FACE); }

  void addModel(std::shared_ptr<Object> object) { objects.push_back(object); }

  void prepareRender(ShaderProgram shader, const Camera camera) {
    this->clear();
    shader.use();
    shader.setMat4("view", camera.getViewMatrix());
    shader.setMat4("projection", camera.getProjectionMatrix());
  }

  void clear() { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }

  void render(ShaderProgram shader, const Camera camera, int width, int height);

  void Render(ShaderProgram shader, const Camera camera) {
    this->prepareRender(shader, camera);

    for (auto& model : objects) model->render(shader);

    camera.render(shader);

    this->endRender();
  }

  void endRender() { glBindVertexArray(0); }

  double update() {
    auto deltaTime = timer.update();
    for (auto& object : objects) {
      object->applyGravity(gravity, deltaTime);
      object->update(deltaTime);
    }
    resolveCollisions();
    return deltaTime;
  }

  void resolveCollisions() {
    for (auto& object : objects) {
      if (object->position.y < -200.0f) {
        auto pos = object->position;
        pos.y = 10;
        object->physicsObject->SetPosition(pos);
        object->physicsObject->SetLinearVelocity(glm::vec3(0.0f));
      }
    }
  }

  std::vector<std::shared_ptr<Object>>& getModels() { return objects; }

  double getDeltaTime() { return timer.deltaTime; }

  void resize(int width, int height) { glViewport(0, 0, width, height); }
};

} 