#ifndef JPH_PROFILE_ENABLED
#define JPH_FLOATING_POINT_EXCEPTIONS_ENABLED
#define JPH_PROFILE_ENABLED
#define JPH_DEBUG_RENDERER
#define JPH_OBJECT_STREAM
#define JPH_CROSS_PLATFORM_DETERMINISTIC
#endif

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

#include "PhysicsEngine.hpp"

// JPH_SUPPRESS_WARNINGS

// using namespace JPH;

using namespace fe;

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
  va_list list;
  va_start(list, inFMT);
  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), inFMT, list);
  va_end(list);

  std::cout << buffer << std::endl;
}

#endif  // JPH_ENABLE_ASSERTS

struct PhysicsEngine::Impl {
  std::unique_ptr<JPH::JobSystemThreadPool> jobSystem;
  std::unique_ptr<JPH::TempAllocatorImpl> temp_allocator;
  std::shared_ptr<JPH::PhysicsSystem> physicsSystem;
  std::shared_ptr<BPLayerInterfaceImpl> broad_phase_layer_interface;
  std::shared_ptr<ObjectLayerPairFilterImpl> object_vs_object_layer_filter;
  std::shared_ptr<ObjectVsBroadPhaseLayerFilterImpl> objectVsBroadphaseLayerFilter;
};

// PhysicsEngine::PhysicsEngine() {
//   impl->physicsSystem = nullptr;
//   impl->temp_allocator = nullptr;
//   impl->jobSystem = nullptr;
// };

PhysicsEngine::PhysicsEngine() {
  impl = std::make_unique<Impl>();

  RegisterDefaultAllocator();

  Trace = TraceImpl;
  JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)
  Factory::sInstance = new Factory();
  RegisterTypes();

  // Create heap-allocated members
  impl->temp_allocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
  impl->jobSystem = std::make_unique<JPH::JobSystemThreadPool>(cMaxPhysicsJobs, cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

  physicsSystem = std::make_shared<JPH::PhysicsSystem>();

  broad_phase_layer_interface = std::make_shared<BPLayerInterfaceImpl>();
  object_vs_object_layer_filter = std::make_shared<ObjectLayerPairFilterImpl>();
  objectVsBroadphaseLayerFilter = std::make_shared<ObjectVsBroadPhaseLayerFilterImpl>();

  physicsSystem->Init(1024, 0, 1024, 1024, *broad_phase_layer_interface, *objectVsBroadphaseLayerFilter, *object_vs_object_layer_filter);

  EnableGravity();

  physicsSystem->OptimizeBroadPhase();

}

ObjectState PhysicsEngine::SyncToRender() {
  auto bodyInterface = &this->physicsSystem->GetBodyInterface();

  JPH::RMat44 transform = bodyInterface->GetWorldTransform(bodyId);
  JPH::RVec3 position = transform.GetTranslation();
  JPH::Vec3 x = transform.GetAxisX();
  JPH::Vec3 y = transform.GetAxisY();
  JPH::Vec3 z = transform.GetAxisZ();
  float translation[3] = {(float)position.GetX(), (float)position.GetY(), (float)position.GetZ()};
  float rotation[9] = {x.GetX(), y.GetX(), z.GetX(), x.GetY(), y.GetY(), z.GetY(), x.GetZ(), y.GetZ(), z.GetZ()};
  ObjectState state;
  state.position = glm::vec3(position.GetX(), position.GetY(), position.GetZ());
  state.rotation = glm::vec3(x.GetX(), y.GetX(), z.GetX());
  // state.rotationY = glm::vec3(x.GetY(), y.GetY(), z.GetY());
  // state.rotationZ67 = glm::vec3(x.GetZ(), y.GetZ(), z.GetZ());
  state.velocity = JPH::ParseVec3(bodyInterface->GetLinearVelocity(bodyId));
  return state;
}

void PhysicsEngine::Update(double dt) {
    // Validate input parameters
    if (dt <= 0.0) {
      // Log warning or handle invalid delta time
      std::cerr << "Warning: Invalid delta time " << dt << ", skipping physics update." << std::endl;
      return;
    }

    // Ensure physics system and dependencies are initialized
    if (!physicsSystem || !impl->temp_allocator || !impl->jobSystem) {
      std::cerr << "Error: Physics system or dependencies not initialized." << std::endl;
      return;
    }

    // Number of collision steps (configurable, currently set to 1 for simplicity)
    const int collisionSteps = 1;

    // Cast dt to float as required by Jolt Physics
    float deltaTime = static_cast<float>(dt);
    
    physicsSystem->Update(deltaTime, collisionSteps, temp_allocator.get(), jobSystem.get());
  }

  void PhysicsEngine::EnableGravity() {
    physicsSystem->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));
  }

  void PhysicsEngine::DisableGravity() {
    physicsSystem->SetGravity(JPH::Vec3(0, 0, 0));
  }