#pragma once

#include <vector>

#include <glm/glm.hpp>

#include <Vertex.hpp>

// Voxel mesh vertex used by the shared chunk/voxel system. Same data as
// fe::VertexArray (position, normal, u/v + texture array layer) plus a baked
// per-vertex block light level (0..1) used by the emissive-block lighting in
// the voxel shaders. The engine accepts self-supplied vertex structs via
// Mesh<VertexType>: provide getLayout() and a VertexTraits<> specialisation.
#pragma pack(push, 1)
struct VoxelVertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 texCoord; // x/y = uv, z = texture-array layer
	float blockLight;   // 0 = no light .. 1 = full emissive light

	VoxelVertex() {}

	VoxelVertex(float x, float y, float z, float nx, float ny, float nz,
	            float u, float v, float layer, float light = 1.0f) {
		this->position = glm::vec3(x, y, z);
		this->normal = glm::vec3(nx, ny, nz);
		this->texCoord = glm::vec3(u, v, layer);
		this->blockLight = light;
	}

	static std::vector<fe::VertexAttribute> getLayout() {
		return {
			{ 0, 3, offsetof(VoxelVertex, position), fe::VertexAttribType::Float },
			{ 1, 3, offsetof(VoxelVertex, normal),   fe::VertexAttribType::Float },
			{ 2, 3, offsetof(VoxelVertex, texCoord), fe::VertexAttribType::Float },
			{ 3, 1, offsetof(VoxelVertex, blockLight), fe::VertexAttribType::Float },
		};
	}
};
#pragma pack(pop)

namespace fe {
	template<>
	struct VertexTraits<VoxelVertex> {
		static glm::vec3 getPosition(const VoxelVertex& v) { return v.position; }
	};
}
