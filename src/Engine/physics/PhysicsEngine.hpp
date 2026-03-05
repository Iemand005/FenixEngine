

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


class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter {
 public:
  virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override { return true; }
};

class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface {
 public:
  virtual uint GetNumBroadPhaseLayers() const override { return 1; }

  virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override { return BroadPhaseLayer(0); }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
  virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override { return "MOVING"; }
#endif
};

class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter {
 public:
  virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override { return true; }
};

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine) {
  std::cerr << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << std::endl;
  return true;
};

#endif

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
