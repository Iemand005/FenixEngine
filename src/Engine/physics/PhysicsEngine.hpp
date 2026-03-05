

// #include <Jolt/Jolt.h>
// #include <Jolt/Geometry/IndexedTriangle.h>
// #include <Jolt/Physics/PhysicsSystem.h>
// #include <Jolt/Physics/Body/BodyCreationSettings.h>
// #include <Jolt/Physics/Body/BodyInterface.h>
// #include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
// #include <Jolt/Physics/Collision/Shape/MeshShape.h>
#pragma once
#include <cstdarg>
#include <iostream>
#include <memory>
#include <thread>



#include "PhysicsObject.hpp"



// namespace BroadPhaseLayers
// {
//   static constexpr BroadPhaseLayer MOVING(0);
// };


namespace fe{

class PhysicsEngine {
 public:

  struct Impl;
  std::unique_ptr<Impl> impl;




  std::vector<std::unique_ptr<fe::PhysicsObject>> physicsObjects;


  void Update(double dt);

  ObjectState SyncToRender();

  std::unique_ptr<fe::PhysicsObject> CreateObject(glm::vec3 size, bool dynamic = true) {
    auto obj = std::make_unique<fe::PhysicsObject>(this, size, dynamic);
    return std::move(obj);
  }

  std::unique_ptr<fe::PhysicsObject> CreateObject(const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices) {
    return std::make_unique<fe::PhysicsObject>(this, vertices, indices);
  }
  
  void EnableGravity();
  void DisableGravity();
};
}