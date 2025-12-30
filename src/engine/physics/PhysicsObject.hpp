
#pragma once
#include "../engine.h"

#include <Jolt/Jolt.h>
#include <Jolt/Geometry/IndexedTriangle.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

using namespace JPH;
using namespace JPH::literals;

namespace Layers
{
  static constexpr ObjectLayer NON_MOVING = 0;
  static constexpr ObjectLayer MOVING = 1;
  static constexpr ObjectLayer NUM_LAYERS = 2;
};

namespace BroadPhaseLayers
{
  static constexpr JPH::BroadPhaseLayer MOVING(0);
};

struct ObjectState {
  glm::vec3 position;
  glm::vec3 rotationX;
  glm::vec3 rotationY;
  glm::vec3 rotationZ67;
};

namespace fe {
class PhysicsObject {
 private:
  JPH::BodyID bodyId;
  // fe::Object* renderObject;
  std::shared_ptr<JPH::PhysicsSystem> physicsSystem;
  
  public:
  // std::unique_ptr<JPH::Body> body;`
  Body *body;
  enum class ShapeType { Box, Sphere, Capsule, Mesh, HeightField };

  PhysicsObject(std::shared_ptr<JPH::PhysicsSystem> physicsSystem) {
  float a = 1.0;
  float b = 0.1;
  float c = 0.5;
    JPH::BoxShapeSettings bodyShapeSettings(JPH::Vec3(a, b, c));
    bodyShapeSettings.mConvexRadius = 0.01;
    bodyShapeSettings.SetDensity(1000.0);
    bodyShapeSettings.SetEmbedded();
    JPH::ShapeSettings::ShapeResult body_shape_result = bodyShapeSettings.Create();
    JPH::ShapeRefC body_shape = body_shape_result.Get();
    
    JPH::BodyCreationSettings bodySettings(body_shape, JPH::RVec3(0.0, 0.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, 0);
    bodySettings.mMaxLinearVelocity = 10000.0;
    bodySettings.mApplyGyroscopicForce = true;
    bodySettings.mLinearDamping = 0.0;
    bodySettings.mAngularDamping = 0.0;

    this->physicsSystem = physicsSystem;

    auto bodyInterface = &this->physicsSystem->GetBodyInterface();
    Body* bodya = bodyInterface->CreateBody(bodySettings);
    this->body = bodya;
    bodyInterface->AddBody(body->GetID(), JPH::EActivation::Activate);

      bodyInterface->SetLinearVelocity(body->GetID(), JPH::Vec3(0.1, 0.0, 0.0));
    bodyInterface->SetAngularVelocity(body->GetID(), JPH::Vec3(0.3, 0.0, 5.0));
  };

  PhysicsObject(){};

  ~PhysicsObject(){};

  void Initialize(JPH::BodyID bodyId, JPH::Body* body) {
    this->bodyId = bodyId;
    this->body = body;
    // this->renderObject = renderObject;
  }

  ObjectState SyncToRender() {
    auto bodyInterface = &this->physicsSystem->GetBodyInterface();

    JPH::RMat44 transform = bodyInterface->GetWorldTransform(body->GetID());
    JPH::RVec3 position = transform.GetTranslation();
    JPH::Vec3 x = transform.GetAxisX();
    JPH::Vec3 y = transform.GetAxisY();
    JPH::Vec3 z = transform.GetAxisZ();
    float translation[3] = {(float)position.GetX(), (float)position.GetY(), (float)position.GetZ()};
    float rotation[9] = {x.GetX(), y.GetX(), z.GetX(), x.GetY(), y.GetY(), z.GetY(), x.GetZ(), y.GetZ(), z.GetZ()};
    ObjectState state;
    state.position = glm::vec3(position.GetX(), position.GetY(), position.GetZ());
    state.rotationX = glm::vec3(x.GetX(), y.GetX(), z.GetX());
    state.rotationY = glm::vec3(x.GetY(), y.GetY(), z.GetY());
    state.rotationZ67 = glm::vec3(x.GetZ(), y.GetZ(), z.GetZ());
    return state;
  }

  // void CreateMeshBody(
  //   const std::vector<fe::Vertex>& vertices,
  //   const std::vector<unsigned int>& indices,
  //   JPH::PhysicsSystem& physicsSystem) {
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
}