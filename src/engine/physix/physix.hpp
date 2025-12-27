
#include <Jolt/Jolt.h>
#include <Jolt/Geometry/IndexedTriangle.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

#include "../engine.h"

class PhysicsComponent {
 private:
  JPH::BodyID bodyId;
  JPH::Body* body;
  std::shared_ptr<fe::Object> renderObject;

 public:
  enum class ShapeType { Box, Sphere, Capsule, Mesh, HeightField };

  PhysicsComponent();
  ~PhysicsComponent();

  void Initialize(JPH::BodyID bodyId, JPH::Body* body, std::shared_ptr<fe::Object> renderObject = nullptr) {
    this->bodyId = bodyId;
    this->body = body;
    this->renderObject = renderObject;
  }

  void SyncToRender() {}

  void CreateMeshShapeFromVertices(std::vector<fe::Vertex> *vertices, std::vector<uint32_t> *indices) {
    JPH::VertexList vertexList;
    JPH::IndexedTriangleList triangleList;

    // Convert vertices to JPH::Float3 (or JPH::Vec3)
    vertexList.reserve(vertices.size());
    for (const fe::Vertex& v : vertices) {
      // glm::vec3 to JPH::Float3 conversion
      vertexList.push_back(JPH::Float3(v.position.x, v.position.y, v.position.z));
    }

    // Convert indices to IndexedTriangle objects
    // Assumes indices are stored as a triangle list (3 consecutive indices per triangle)
    triangleList.reserve(indices.size() / 3);
    for (size_t i = 0; i < indices.size(); i += 3) {
      JPH::IndexedTriangle triangle(static_cast<uint32_t>(indices[i]), static_cast<uint32_t>(indices[i + 1]), static_cast<uint32_t>(indices[i + 2]), 0u);
      triangleList.push_back(triangle);
    }
  }

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