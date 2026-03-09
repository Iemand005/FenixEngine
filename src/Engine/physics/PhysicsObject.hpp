
#pragma once

#include "../bases.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
// #include "PhysicsEngine.hpp"

// 7

namespace JPH { class PhysicsSystem; }


namespace fe {
class PhysicsEngine;

class PhysicsObject {
  friend class PhysicsEngine;
  struct Impl;
  std::unique_ptr<Impl> impl;

 public:
  // std::unique_ptr<JPH::Body> body;`
  enum class ShapeType { Box, Sphere, Capsule, Mesh, HeightField, Compound };

  PhysicsObject(glm::vec3 size, bool dynamic = true);

  PhysicsObject(const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices,const glm::vec3& position = glm::vec3(0.0f), float density = 1000.0f,bool isStatic = true);

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

 private:
  void BindPhysicsSystem(std::shared_ptr<JPH::PhysicsSystem> physicsSystem);
  void InitializeBoxBody(glm::vec3 size, bool dynamic);
  void InitializeMeshBody(const std::vector<glm::vec3>& vertices,
                          const std::vector<uint32_t>& indices,
                          const glm::vec3& position,
                          float density,
                          bool isStatic);
};
}  // namespace fe
