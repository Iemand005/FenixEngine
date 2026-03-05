
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



namespace fe {
class PhysicsObject {
  struct Impl;
  std::unique_ptr<Impl> impl;
 private:
  JPH::BodyID bodyId;
  // fe::Object* renderObject;
  std::shared_ptr<JPH::PhysicsSystem> physicsSystem;


 public:
  // std::unique_ptr<JPH::Body> body;`
  Body* body;
  enum class ShapeType { Box, Sphere, Capsule, Mesh, HeightField, Compound };

  PhysicsObject(std::shared_ptr<JPH::PhysicsSystem> physicsSystem, glm::vec3 size, bool dynamic = true);

  PhysicsObject(std::shared_ptr<JPH::PhysicsSystem> physicsSystem, const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices,const glm::vec3& position = glm::vec3(0.0f), float density = 1000.0f,bool isStatic = true);

  PhysicsObject() : physicsSystem(nullptr), bodyId(BodyID()) {};

  ~PhysicsObject() {};

  std::unique_ptr<PhysicsObject> Clone() {
    return nullptr;
  }

  ObjectState SyncToRender();

    

  void CreateBodyFromShape(JPH::ShapeRefC shape,
                          const glm::vec3& position,
                          JPH::EMotionType motionType,
                          JPH::ObjectLayer layer);

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

};
}  // namespace fe