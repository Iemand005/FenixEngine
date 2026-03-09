
#pragma once


#include "../engine.h"
#include "../bases.h"
#include <memory>
// #include "PhysicsEngine.hpp"

// 7

namespace fe {
class PhysicsObject {
  struct Impl;
  std::unique_ptr<Impl> impl;

 public:
  // std::unique_ptr<JPH::Body> body;`
  enum class ShapeType { Box, Sphere, Capsule, Mesh, HeightField, Compound };

  PhysicsObject(void *physicsSystem, glm::vec3 size, bool dynamic = true);

  PhysicsObject(void *physicsSystem, const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices,const glm::vec3& position = glm::vec3(0.0f), float density = 1000.0f,bool isStatic = true);

  PhysicsObject();

  ~PhysicsObject();

  std::unique_ptr<PhysicsObject> Clone() {
    return nullptr;
  }

  ObjectState SyncToRender();

  // CreateBodyFromShape(JPH::ShapeRefC shape,
  //                         const glm::vec3& position,
  //                         JPH::EMotionType motionType,
  //                         JPH::ObjectLayer layer);

  // BodyInterface* GetBody() { return &this->physicsSystem->GetBodyInterface(); }

  void SetLinearVelocity(glm::vec3 velocity);

  void AddLinearVelocity(glm::vec3 velocity);

  glm::vec3 GetPosition();
  void SetPosition(glm::vec3 position);
  void AddPosition(glm::vec3 position);

};
};  // namespace fe
