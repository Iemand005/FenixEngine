

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

#define JPH_FLOATING_POINT_EXCEPTIONS_ENABLED
#define JPH_PROFILE_ENABLED
#define JPH_DEBUG_RENDERER
#define JPH_OBJECT_STREAM

#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include "PhysicsObject.hpp"

JPH_SUPPRESS_WARNINGS

using namespace JPH;

// namespace BroadPhaseLayers
// {
//   static constexpr BroadPhaseLayer MOVING(0);
// };
#ifdef JPH_ENABLE_ASSERTS

// // Callback for asserts, connect this to your own assert handler if you have one
// static bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, uint inLine)
// {
// 	// Print to the TTY
// 	std::cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr? inMessage : "") << std::endl;

// 	// Breakpoint
// 	return true;
// };

static void TraceImpl(const char* inFMT, ...) {
  // Format the message
  va_list list;
  va_start(list, inFMT);
  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), inFMT, list);
  va_end(list);

  // Print to the TTY
  std::cout << buffer << std::endl;
}

#endif  // JPH_ENABLE_ASSERTS

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
  // PhysicsSystem physicsSystem;
  // JobSystemThreadPool *jobSystem;
  // TempAllocatorMalloc temp_allocator;
  std::unique_ptr<JPH::JobSystemThreadPool> jobSystem;  // Unique ownership
  std::unique_ptr<JPH::TempAllocatorImpl> temp_allocator;

  std::shared_ptr<PhysicsSystem> physicsSystem;

  std::shared_ptr<BPLayerInterfaceImpl> broad_phase_layer_interface;
  std::shared_ptr<ObjectLayerPairFilterImpl> object_vs_object_layer_filter;
  std::shared_ptr<ObjectVsBroadPhaseLayerFilterImpl> object_vs_broadphase_layer_filter;

  std::vector<std::unique_ptr<fe::PhysicsObject>> physicsObjects;

  PhysicsEngine() {
    RegisterDefaultAllocator();
    Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)
    Factory::sInstance = new Factory();
    RegisterTypes();

    // Create heap-allocated members
    temp_allocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
    jobSystem = std::make_unique<JPH::JobSystemThreadPool>(cMaxPhysicsJobs, cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

    physicsSystem = std::make_shared<JPH::PhysicsSystem>();

    broad_phase_layer_interface = std::make_shared<BPLayerInterfaceImpl>();
    object_vs_object_layer_filter = std::make_shared<ObjectLayerPairFilterImpl>();
    object_vs_broadphase_layer_filter = std::make_shared<ObjectVsBroadPhaseLayerFilterImpl>();

    physicsSystem->Init(1024, 0, 1024, 1024, *broad_phase_layer_interface, *object_vs_broadphase_layer_filter, *object_vs_object_layer_filter);

    physicsSystem->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));


    physicsSystem->OptimizeBroadPhase();

  }

  void Update(double dt) {
    // Validate input parameters
    if (dt <= 0.0) {
      // Log warning or handle invalid delta time
      std::cerr << "Warning: Invalid delta time " << dt << ", skipping physics update." << std::endl;
      return;
    }

    // Ensure physics system and dependencies are initialized
    if (!physicsSystem || !temp_allocator || !jobSystem) {
      std::cerr << "Error: Physics system or dependencies not initialized." << std::endl;
      return;
    }

    // Number of collision steps (configurable, currently set to 1 for simplicity)
    const int collisionSteps = 1;

    // Cast dt to float as required by Jolt Physics
    float deltaTime = static_cast<float>(dt);
    
    physicsSystem->Update(deltaTime, collisionSteps, temp_allocator.get(), jobSystem.get());
  }

  std::unique_ptr<fe::PhysicsObject> CreateObject(bool dynamic = true) {
    auto obj = std::make_unique<fe::PhysicsObject>(physicsSystem, dynamic);
    return std::move(obj);
  }

  std::unique_ptr<fe::PhysicsObject> CreateObject(const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices) {
    return std::make_unique<fe::PhysicsObject>(physicsSystem, vertices, indices);
  }
};