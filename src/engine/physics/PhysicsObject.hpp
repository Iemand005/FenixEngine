
#pragma once
#include <Jolt/Geometry/IndexedTriangle.h>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include "../engine.h"
#include "../bases.h"

using namespace JPH;
using namespace JPH::literals;

namespace Layers {
static constexpr ObjectLayer NON_MOVING = 0;
static constexpr ObjectLayer MOVING = 1;
static constexpr ObjectLayer NUM_LAYERS = 2;
};  // namespace Layers

namespace BroadPhaseLayers {
static constexpr JPH::BroadPhaseLayer MOVING(0);
};

namespace fe {
class PhysicsObject {
 private:
  JPH::BodyID bodyId;
  // fe::Object* renderObject;
  std::shared_ptr<JPH::PhysicsSystem> physicsSystem;

 public:
  // std::unique_ptr<JPH::Body> body;`
  Body* body;
  enum class ShapeType { Box, Sphere, Capsule, Mesh, HeightField, Compound };

  PhysicsObject(std::shared_ptr<JPH::PhysicsSystem> physicsSystem, glm::vec3 size, bool dynamic = true) : physicsSystem(physicsSystem) {
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
  };

  PhysicsObject(std::shared_ptr<JPH::PhysicsSystem> physicsSystem,
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

  PhysicsObject() : physicsSystem(nullptr), bodyId(BodyID()) {};

  ~PhysicsObject() {};

  std::unique_ptr<PhysicsObject> Clone() {
    return nullptr;
  }

  ObjectState SyncToRender() {
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
    state.velocity = ParseVec3(bodyInterface->GetLinearVelocity(bodyId));
    return state;
  }

    static JPH::ShapeRefC CreateMeshShape(const std::vector<glm::vec3>& vertices,
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

  BodyInterface* GetBody() { return &this->physicsSystem->GetBodyInterface(); }

  void SetLinearVelocity(glm::vec3 velocity) { GetBody()->SetLinearVelocity(bodyId, JPH::Vec3(velocity.x, velocity.y, velocity.z)); }

  JPH::Vec3 VecConv(glm::vec3 vec) { return JPH::Vec3(vec.x, vec.y, vec.z); }

  glm::vec3 ParseVec3(JPH::Vec3 vec) { return glm::vec3(vec.GetX(), vec.GetY(), vec.GetZ()); }

  void AddLinearVelocity(glm::vec3 velocity) { GetBody()->AddLinearVelocity(bodyId, VecConv(velocity)); }

  glm::vec3 GetPosition() { return ParseVec3(GetBody()->GetPosition(bodyId)); }
  void SetPosition(glm::vec3 position) { GetBody()->SetPosition(bodyId, VecConv(position), JPH::EActivation::Activate); }
  void AddPosition(glm::vec3 position) { SetPosition(position + GetPosition()); }

  // void CreateMeshBody(const std::vector<fe::Vertex>& vertices, const std::vector<unsigned int>& indices) {
  //   JPH::VertexList vertexList;
  //   JPH::IndexedTriangleList triangleList;

  //   // Convert vertices to JPH::Float3 (or JPH::Vec3)
  //   vertexList.reserve(vertices.size());
  //   for (const fe::Vertex& v : vertices) {
  //     // glm::vec3 to JPH::Float3 conversion
  //     vertexList.push_back(JPH::Float3(v.position.x, v.position.y, v.position.z));
  //   }

  //   // Convert indices to IndexedTriangle objects
  //   // Assumes indices are stored as a triangle list (3 consecutive indices per triangle)
  //   triangleList.reserve(indices.size() / 3);
  //   for (size_t i = 0; i < indices.size(); i += 3) {
  //     JPH::IndexedTriangle triangle(static_cast<uint32_t>(indices[i]), static_cast<uint32_t>(indices[i + 1]), static_cast<uint32_t>(indices[i + 2]), 0u);
  //     triangleList.push_back(triangle);
  //   }
  // }

  // Helper function to create indexed triangles from a vertex list
  // Assumes vertices are in triangle list format: v0, v1, v2, v3, v4, v5...
  std::vector<JPH::IndexedTriangle> CreateTrianglesFromVertexList(const std::vector<JPH::Float3>& vertices) {
    std::vector<JPH::IndexedTriangle> triangles;

    if (vertices.size() % 3 != 0) {
      std::cerr << "Warning: Vertex count not divisible by 3" << std::endl;
    }

    size_t triangle_count = vertices.size() / 3;
    triangles.reserve(triangle_count);

    for (size_t i = 0; i < triangle_count; ++i) {
      JPH::IndexedTriangle triangle;
      triangle.mIdx[0] = static_cast<JPH::uint32>(i * 3);
      triangle.mIdx[1] = static_cast<JPH::uint32>(i * 3 + 1);
      triangle.mIdx[2] = static_cast<JPH::uint32>(i * 3 + 2);
      triangle.mMaterialIndex = 0;
      triangles.push_back(triangle);
    }

    return triangles;
  }

  // Helper function to create indexed triangles with actual indices
  std::vector<JPH::IndexedTriangle> CreateTrianglesFromIndexedData(const std::vector<JPH::Float3>& vertices, const std::vector<uint32_t>& indices) {
    std::vector<JPH::IndexedTriangle> triangles;

    if (indices.size() % 3 != 0) {
      std::cerr << "Warning: Index count not divisible by 3" << std::endl;
    }

    size_t triangle_count = indices.size() / 3;
    triangles.reserve(triangle_count);

    for (size_t i = 0; i < triangle_count; ++i) {
      JPH::IndexedTriangle triangle;
      triangle.mIdx[0] = indices[i * 3];
      triangle.mIdx[1] = indices[i * 3 + 1];
      triangle.mIdx[2] = indices[i * 3 + 2];
      triangle.mMaterialIndex = 0;
      triangles.push_back(triangle);
    }

    return triangles;
  }
};
}  // namespace fe