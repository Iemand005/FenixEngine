
#include "../engine.h"

#include <Jolt/Jolt.h>
#include <Jolt/Geometry/IndexedTriangle.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>

class PhysicsComponent {
private:
  JPH::BodyID bodyId;
  JPH::Body* body;
  std::shared_ptr<fe::Object> renderObject;
public:
    enum class ShapeType {
        Box,
        Sphere,
        Capsule,
        Mesh,
        HeightField
    };

    PhysicsComponent();
    ~PhysicsComponent();

    void Initialize(JPH::BodyID bodyId, JPH::Body* body, std::shared_ptr<fe::Object> renderObject = nullptr) {
      this->bodyId = bodyId;
      this->body = body;
      this->renderObject = renderObject;
    }

    void SyncToRender() {

    }

//     PhysicsComponent* CreateConvexHullFromVertices(
//     const std::vector<JPH::Vec3>& vertices,
//     float max_convex_shape = 0.25f) {
    
//     JPH::ConvexHullShapeSettings hull_settings(vertices);
//     hull_settings.mMaxConvexRadius = max_convex_shape;
    
//     JPH::ShapeSettings::ShapeResult hull_result = hull_settings.Create();
//     if (hull_result.HasError()) {
//         return nullptr;
//     }
    
//     JPH::ShapeRefC hull_shape = hull_result.Get();
    
//     // Create body as before...
// }

PhysicsComponent* CreateMeshShapeFromVertices(
    const std::vector<glm::vec3>& vertices,
    const std::vector<JPH::IndexedTriangle>& triangles,
    const JPH::Vec3& position,
    float mass,
    void* renderObject) {
    
    // Validate input
    if (vertices.empty() || triangles.empty()) {
        return nullptr;
    }

    JPH::Array<JPH::Float3> newVertices(vertices.size());
    for (auto &vertex : vertices) newVertices.push_back(JPH::Float3(vertex.x, vertex.y, vertex.z));
    
    // Create mesh shape settings
    JPH::MeshShapeSettings mesh_settings(newVertices, triangles);
    
    // Optional: Optimize the mesh
    mesh_settings.mMaxTrianglesPerLeaf = 8;
    
    // Create the mesh shape
    JPH::ShapeSettings::ShapeResult mesh_result = mesh_settings.Create();
    if (mesh_result.HasError()) {
        std::cerr << "Failed to create mesh shape: " 
                  << mesh_result.GetError().c_str() << std::endl;
        return nullptr;
    }
    
    JPH::ShapeRefC mesh_shape = mesh_result.Get();
    
    // Determine motion type based on mass
    JPH::EMotionType motion_type = (mass > 0.0f) ? 
        JPH::EMotionType::Dynamic : JPH::EMotionType::Static;
    
    // Determine layer based on motion
    JPH::ObjectLayer layer = (mass > 0.0f) ? 
        Layers::MOVING : Layers::NON_MOVING;
    
    // Create body settings
    JPH::BodyCreationSettings body_settings(
        mesh_shape,
        position,
        JPH::Quat::sIdentity(),
        motion_type,
        layer
    );
    
    // Set physics properties
    body_settings.mFriction = 0.8f;
    body_settings.mRestitution = 0.3f;
    
    if (mass > 0.0f) {
        body_settings.mMassPropertiesOverride.mMass = mass;
        body_settings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
    }
    
    // Create the body
    JPH::Body* body = mPhysicsSystem->GetBodyInterface().CreateBody(body_settings);
    if (!body) {
        std::cerr << "Failed to create body for mesh shape" << std::endl;
        return nullptr;
    }
    
    // Add body to physics world
    JPH::EActivation activation = (mass > 0.0f) ? 
        JPH::EActivation::Activate : JPH::EActivation::DontActivate;
    
    mPhysicsSystem->GetBodyInterface().AddBody(body->GetID(), activation);
    
    // Create and return physics component
    PhysicsComponent* component = new PhysicsComponent();
    component->Initialize(body->GetID(), body, renderObject);
    
    // Store reference
    mComponents[body->GetID()] = component;
    
    return component;
}

// Helper function to create indexed triangles from a vertex list
// Assumes vertices are in triangle list format: v0, v1, v2, v3, v4, v5...
std::vector<JPH::IndexedTriangle> CreateTrianglesFromVertexList(
    const std::vector<JPH::Float3>& vertices) {
    
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
std::vector<JPH::IndexedTriangle> CreateTrianglesFromIndexedData(
    const std::vector<JPH::Float3>& vertices,
    const std::vector<uint32_t>& indices) {
    
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