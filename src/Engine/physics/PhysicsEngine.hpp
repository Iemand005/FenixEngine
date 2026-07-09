
#pragma once
#include <cstdarg>
#include <iostream>
#include <memory>
#include <thread>



#include "PhysicsObject.hpp"
#include "../Graphics/Renderer.hpp"

namespace fe{

class PhysicsEngine {
 public:

  struct Impl;
  std::unique_ptr<Impl> impl;

  PhysicsEngine();
  ~PhysicsEngine();


  std::vector<std::unique_ptr<PhysicsObject>> physicsObjects;


  void Update(double dt);

  ObjectState SyncToRender();

  std::unique_ptr<fe::PhysicsObject> CreateObject(glm::vec3 size, bool dynamic = true);

  std::unique_ptr<fe::PhysicsObject> CreateObject(const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices);
  
  void EnableGravity();
  void DisableGravity();
  void RenderDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
};
}
