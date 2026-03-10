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

using namespace JPH;

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

class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
 public:
  virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override { return true; }
};

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
 public:
  virtual uint GetNumBroadPhaseLayers() const override { return 1; }

  virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override { return BroadPhaseLayer(0); }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
  virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override { return "MOVING"; }
#endif
};

class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
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

  impl->physicsSystem = std::make_shared<JPH::PhysicsSystem>();

  impl->broad_phase_layer_interface = std::make_shared<BPLayerInterfaceImpl>();
  impl->object_vs_object_layer_filter = std::make_shared<ObjectLayerPairFilterImpl>();
  impl->objectVsBroadphaseLayerFilter = std::make_shared<ObjectVsBroadPhaseLayerFilterImpl>();

  impl->physicsSystem->Init(1024, 0, 1024, 1024, *impl->broad_phase_layer_interface, *impl->objectVsBroadphaseLayerFilter, *impl->object_vs_object_layer_filter);

  EnableGravity();

  impl->physicsSystem->OptimizeBroadPhase();

}

PhysicsEngine::~PhysicsEngine() = default;



void PhysicsEngine::Update(double dt) {
    // Validate input parameters
    if (dt <= 0.0) {
      // Log warning or handle invalid delta time
      std::cerr << "Warning: Invalid delta time " << dt << ", skipping physics update." << std::endl;
      return;
    }

    // Ensure physics system and dependencies are initialized
    if (!impl || !impl->physicsSystem || !impl->temp_allocator || !impl->jobSystem) {
      std::cerr << "Error: Physics system or dependencies not initialized." << std::endl;
      return;
    }

    // Number of collision steps (configurable, currently set to 1 for simplicity)
    const int collisionSteps = 1;

    // Cast dt to float as required by Jolt Physics
    float deltaTime = static_cast<float>(dt);
    
    impl->physicsSystem->Update(deltaTime, collisionSteps, impl->temp_allocator.get(), impl->jobSystem.get());
  }

  void PhysicsEngine::EnableGravity() {
    impl->physicsSystem->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));
  }

  void PhysicsEngine::DisableGravity() {
    impl->physicsSystem->SetGravity(JPH::Vec3(0, 0, 0));
  }

  std::unique_ptr<fe::PhysicsObject> PhysicsEngine::CreateObject(glm::vec3 size, bool dynamic) {
    auto obj = std::make_unique<fe::PhysicsObject>(size, dynamic);
    obj->BindPhysicsSystem(impl->physicsSystem);
    obj->InitializeBoxBody(size, dynamic);
    return obj;
  }

  std::unique_ptr<fe::PhysicsObject> PhysicsEngine::CreateObject(const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices) {
    auto obj = std::make_unique<fe::PhysicsObject>(vertices, indices);
    obj->BindPhysicsSystem(impl->physicsSystem);
    obj->InitializeMeshBody(vertices, indices, glm::vec3(0.0f), 1000.0f, true);
    return obj;
  }
