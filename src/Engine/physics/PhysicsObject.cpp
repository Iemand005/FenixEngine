#include "PhysicsObject.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Geometry/IndexedTriangle.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/PhysicsSystem.h>

using namespace fe;
// using namespace JPH;
using namespace JPH::literals;

namespace Layers {
static constexpr ObjectLayer NON_MOVING = 0;
static constexpr ObjectLayer MOVING = 1;
static constexpr ObjectLayer NUM_LAYERS = 2;
};  // namespace Layers

namespace BroadPhaseLayers {
static constexpr JPH::BroadPhaseLayer MOVING(0);
};

struct PhysicsObject::Impl {
  JPH::BodyID bodyId;
  std::shared_ptr<JPH::PhysicsSystem> physicsSystem;
  JPH::Body* body;
  // std::shared_ptr<JPH::PhysicsSystem> physicsSystem;
};

PhysicsObject::PhysicsObject() {
  impl = std::make_unique<Impl>();
  impl->bodyId = JPH::BodyId();
  impl->physicsSystem = nullptr;
}

JPH::Vec3 VecConv(glm::vec3 vec) { return JPH::Vec3(vec.x, vec.y, vec.z); }

  glm::vec3 ParseVec3(JPH::Vec3 vec) { return glm::vec3(vec.GetX(), vec.GetY(), vec.GetZ()); }

  void AddLinearVelocity(glm::vec3 velocity) {
    this->physicsSystem->GetBodyInterface()->AddLinearVelocity(bodyId, VecConv(velocity));
  }

  glm::vec3 GetPosition() { return ParseVec3(this->physicsSystem->GetBodyInterface()->GetPosition(bodyId)); }

JPH::ShapeRefC CreateMeshShape(const std::vector<glm::vec3>& vertices,
                                      const std::vector<uint32_t>& indices,
                                      float density = 1000.0f) {
    // Validate vertices
    if (vertices.empty()) {
        std::cerr << "Error: No vertices provided!" << std::endl;
        return nullptr;
    }
    
    // Validate indices
    if (indices.empty()) {
        std::cerr << "Error: No indices provided!" << std::endl;
        return nullptr;
    }
    
    if (indices.size() % 3 != 0) {
        std::cerr << "Error: Index count must be divisible by 3! Got " 
                  << indices.size() << " indices." << std::endl;
        return nullptr;
    }
    
    size_t triangleCount = indices.size() / 3;
    if (triangleCount == 0) {
        std::cerr << "Error: Need at least one triangle!" << std::endl;
        return nullptr;
    }
    
    std::cout << "Creating mesh with " << vertices.size() 
              << " vertices and " << triangleCount << " triangles." << std::endl;
    
    // Check for valid index ranges
    for (uint32_t index : indices) {
        if (index >= vertices.size()) {
            std::cerr << "Error: Index " << index 
                      << " out of bounds (max: " << vertices.size() - 1 << ")" << std::endl;
            return nullptr;
        }
    }
    
    // Convert glm::vec3 to JPH::Float3
    JPH::VertexList vertexList;
    // vertexList.reserve(vertices.size());
    for (const auto& v : vertices) {
        vertexList.emplace_back(v.x, v.y, v.z);
    }
    
    // Create triangle list from indices
    JPH::IndexedTriangleList triangleList;
    triangleList.reserve(triangleCount);
    
    for (size_t i = 0; i < indices.size(); i += 3) {
        JPH::IndexedTriangle triangle(
            indices[i], 
            indices[i + 1], 
            indices[i + 2], 
            0  // Default material
        );
        triangleList.push_back(triangle);
    }
    
    // Create mesh shape settings
    JPH::MeshShapeSettings meshSettings(vertexList, triangleList);
    
    // IMPORTANT: Set Sanitize = true to clean up the mesh
    // meshSettings.Sanitize();
    // meshSettings.
    // Create the shape
    JPH::ShapeSettings::ShapeResult result = meshSettings.Create();

    
    if (result.HasError()) {
        std::cerr << "Error creating mesh shape: " << result.GetError() << std::endl;
        return nullptr;
    }
    
    return result.Get();
}

void CreateBodyFromShape(JPH::ShapeRefC shape,
                          const glm::vec3& position,
                          JPH::EMotionType motionType,
                          JPH::ObjectLayer layer) {
    if (!shape) {
      std::cerr << "Error: Cannot create body with null shape!" << std::endl;
      return;
    }
    
    // Create body settings
    JPH::BodyCreationSettings bodySettings(
      shape,
      JPH::RVec3(position.x, position.y, position.z),
      JPH::Quat::sIdentity(),
      motionType,
      layer
    );
    
    // Configure body properties
    bodySettings.mFriction = 0.5f;
    bodySettings.mRestitution = 0.1f;
    bodySettings.mLinearDamping = 0.05f;
    bodySettings.mAngularDamping = 0.05f;
    bodySettings.mMaxLinearVelocity = 100.0f;
    bodySettings.mAllowSleeping = true;
    
    if (motionType == JPH::EMotionType::Dynamic) {
      bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
      JPH::MassProperties massProperties;
      massProperties.mMass = 1.0f;
      bodySettings.mMassPropertiesOverride = massProperties;
    }
    
    // Get body interface and create body
    auto& bodyInterface = physicsSystem->GetBodyInterface();
    JPH::Body* body = bodyInterface.CreateBody(bodySettings);
    
    if (!body) {
      std::cerr << "Failed to create physics body!" << std::endl;
      return;
    }
    
    bodyId = body->GetID();
    bodyInterface.AddBody(bodyId, JPH::EActivation::Activate);
    
    std::cout << "Created PhysicsObject with BodyID: " << bodyId.GetIndex()
              << " (MotionType: " << (motionType == JPH::EMotionType::Static ? "Static" : "Dynamic")
              << ", Layer: " << layer << ")" << std::endl;
  }

PhysicsObject::PhysicsObject(void *physicsSystem, glm::vec3 size, bool dynamic) : physicsSystem(physicsSystem) {
  float a = size.x;
  float b = size.y;
  float c = size.z;

  JPH::BoxShapeSettings bodyShapeSettings(JPH::Vec3(a, b, c));
  bodyShapeSettings.mConvexRadius = 0.01;
  bodyShapeSettings.SetDensity(1000.0);
  bodyShapeSettings.SetEmbedded();
  JPH::ShapeSettings::ShapeResult body_shape_result = bodyShapeSettings.Create();
  JPH::ShapeRefC body_shape = body_shape_result.Get();

  JPH::BodyCreationSettings bodySettings(body_shape, JPH::RVec3(0.0, 0.0, 0.0), JPH::Quat::sIdentity(), dynamic ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static, Layers::MOVING);
  bodySettings.mMaxLinearVelocity = 10000.0;
  bodySettings.mApplyGyroscopicForce = true;
  bodySettings.mLinearDamping = 0.0;
  bodySettings.mAngularDamping = 0.0;

  // this->physicsSystem = physicsSystem;

  auto bodyInterface = &this->physicsSystem->GetBodyInterface();
  this->body = bodyInterface->CreateBody(bodySettings);
  this->bodyId = body->GetID();
  bodyInterface->AddBody(this->body->GetID(), JPH::EActivation::Activate);
  // bodyInterface->SetGravityFactor()
}

PhysicsObject::PhysicsObject(void *physicsSystem,
              const std::vector<glm::vec3>& vertices,
              const std::vector<uint32_t>& indices,
              const glm::vec3& position = glm::vec3(0.0f),
              float density = 1000.0f,
              bool isStatic = true)
  : physicsSystem(physicsSystem)
{
  if (!physicsSystem) {
    std::cerr << "Error: PhysicsObject created with null physicsSystem!" << std::endl;
    return;
  }
  
  JPH::ShapeRefC shape = CreateMeshShape(vertices, indices, density);
  if (!shape) {
    std::cerr << "Failed to create mesh shape!" << std::endl;
    return;
  }
  
  CreateBodyFromShape(shape, position, 
                      isStatic ? JPH::EMotionType::Static : JPH::EMotionType::Dynamic,
                      isStatic ? Layers::NON_MOVING : Layers::MOVING);
}


void PhysicsObject::SetPosition(glm::vec3 position) {
  this->physicsSystem->GetBodyInterface()->SetPosition(bodyId, VecConv(position), JPH::EActivation::Activate);
}

void PhysicsObject::AddPosition(glm::vec3 position) {
  SetPosition(position + GetPosition());
}

void PhysicsObject::SetLinearVelocity(glm::vec3 velocity) {
  this->physicsSystem->GetBodyInterface()->SetLinearVelocity(bodyId, JPH::Vec3(velocity.x, velocity.y, velocity.z));
}
