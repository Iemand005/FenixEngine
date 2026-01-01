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

class Object {
 private:
 public:
  glm::vec3 position;
  glm::vec3 rotation;
  glm::vec3 velocity;
  glm::vec3 acceleration;

  glm::vec3 scale;
  glm::mat4 modelMatrix;

  std::vector<Mesh> meshes;

  bool isStatic = false;

  bool needsUpdate = true;

  bool touchedGround = false;

  bool touchedOtherObject = false;

  unsigned int boundingBoxVAO = 0, boundingBoxVBO = 0;

  std::vector<glm::vec3> boundingBoxVertices;

  std::unique_ptr<fe::PhysicsObject> physicsObject = nullptr;

  // bool needsUpdate = true;

  Object() {
    position = glm::vec3(0.0f, 0.0f, 0.0f);
    rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
    scale = glm::vec3(1.0f, 1.0f, 1.0f);
    modelMatrix = glm::mat4(1.0f);
    meshes = std::vector<Mesh>();
  }

  Object(Mesh mesh) : Object() {
    meshes.push_back(mesh);
  }

  Object(std::string objFilePath, float scale = 1.0f) : Object() { loadOBJ(objFilePath, scale); }

  bool loadOBJ(std::string path, float scale = 1.0f);

  void applyForce(const glm::vec3& force) {
    this->acceleration = this->acceleration + force;
    this->needsUpdate = true;
  }

  void applyAcceleration(const glm::vec3& acceleration) {
    this->velocity = this->velocity + acceleration;
    this->needsUpdate = true;
  }

  void applyVelocity(const glm::vec3& vel) {
    this->velocity = vel;
    this->needsUpdate = true;
  }

  void applyGravity(const glm::vec3& gravity, double deltaTime) { this->applyAcceleration(gravity * glm::vec3(deltaTime)); }

  void SetPhysicsObject(std::unique_ptr<PhysicsObject> physicsObject) { this->physicsObject = std::move(physicsObject); }

  void update(double deltaTime) {
    // this->applyAcceleration(this->acceleration);
    // this->position = this->position + this->velocity * static_cast<float>(deltaTime) + glm::radians(0.0001f);
    // this->acceleration = glm::vec3(0.0f);

    if (this->physicsObject) {
      auto state = this->physicsObject->SyncToRender();
      this->position = state.position;
      this->velocity = state.velocity;
      this->rotation = state.rotationX;
    }
  }

  glm::mat4 getModelMatrix() {
    glm::mat4 model = glm::translate(this->modelMatrix, this->position);
    model = glm::rotate(model, glm::radians(this->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(this->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::scale(model, this->scale);
    return model;
  }

  void render(ShaderProgram& shader) {
    for (auto& mesh : meshes) mesh.render(shader, this->getModelMatrix());
    if (boundingBoxVAO && touchedOtherObject) {
      shader.use();
      shader.setMat4("model", this->getModelMatrix());
      glBindVertexArray(boundingBoxVAO);
      glDrawArrays(GL_LINES, 0, boundingBoxVertices.size());
      glBindVertexArray(0);
    }
  }

  std::shared_ptr<Object> clone() const {
    auto newObj = std::make_shared<Object>();
    newObj->meshes = this->meshes;
    newObj->scale = this->scale;
    newObj->modelMatrix = this->modelMatrix;
    return newObj;
  }

  void lookAt(const glm::vec3& target) {
    glm::vec3 direction = glm::normalize(target - this->position);
    float pitch = glm::degrees(asin(direction.y));
    float yaw = glm::degrees(atan2(direction.z, direction.x));

    this->rotation.x = pitch;
    this->rotation.y = -yaw + 90.0f;
  }
};

}