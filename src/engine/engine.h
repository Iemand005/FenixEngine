#pragma once
#include <glad/glad.h>

#include <chrono>
#include <cstdio>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>
#include <string>

#include "physics/PhysicsEngine.hpp"

#ifdef OBJ_LOADER
#include <OBJ_Loader.h>
#endif

#include <stb_image.h>

#include "Object.hpp"

namespace fe {


struct Timer {
  std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

 public:
  double lastTime = 0.0;
  double deltaTime = 0.0;

  Timer() { this->reset(); }

  double update() {
    double currentTime = getTime();
    deltaTime = currentTime - lastTime;
    lastTime = currentTime;
    return deltaTime;
  }

  void reset() { startTime = std::chrono::high_resolution_clock::now(); }

  double getTime() {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = now - startTime;
    return elapsed.count();
  }
};

class Character : public Object {
 public:
};


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

  void render(ShaderProgramoooooooooooooooooooooooooooooooooooooooooooooooooooo shader, const Camera camera, int width, int height);

  void Render(ShaderProgramoooooooooooooooooooooooooooooooooooooooooooooooooooo shader, const Camera camera) {
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
    // for (auto& object : objects) {
    //   if (object->position.y < 0.0f) {
    //     object->position.y = 0.0f;
    //     object->velocity.y *= -0.1f;

    //     object->touchedGround = true;

    //     if (object->velocity.y < 0.01f && object->velocity.y > -0.01f) object->needsUpdate = false;
    //   } else {
    //     object->touchedGround = false;
    //   }

    //   bool foundTouch = false;
    //   for (auto& otherObject : objects) {
    //     if (otherObject == object) continue;

    //     if (object->intersects(*otherObject)) {
    //       if (!object->touchedOtherObject) std::cout << "Intersected" << std::endl;
    //       object->touchedOtherObject = true;
    //       otherObject->touchedOtherObject = true;
    //       foundTouch = true;
    //       break;
    //     }
    //   }

    //   if (!foundTouch) {
    //     object->touchedOtherObject = false;
    //   }
    // }
  }

  std::vector<std::shared_ptr<Object>>& getModels() { return objects; }

  double getDeltaTime() { return timer.deltaTime; }

  void resize(int width, int height) { glViewport(0, 0, width, height); }
};

}  // namespace fe