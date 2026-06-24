#pragma once
#include "Mesh.hpp"

namespace fe::Primitives {

	enum class PlaneDirection {
		Front,   // +Z
		Back,    // -Z
		Right,   // +X
		Left,    // -X
		Top,     // +Y
		Bottom   // -Y
	};

	inline glm::quat GetRotationFromDirection(PlaneDirection direction) {
		switch(direction) {
			default:
			case PlaneDirection::Front:
				return glm::quat(1, 0, 0, 0);
			case PlaneDirection::Back:
				return glm::angleAxis(glm::radians(180.0f), glm::vec3(0, 1, 0));
			case PlaneDirection::Right:
				return glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 1, 0));
			case PlaneDirection::Left:
				return glm::angleAxis(glm::radians(-90.0f), glm::vec3(0, 1, 0));
			case PlaneDirection::Top:
				return glm::angleAxis(glm::radians(90.0f), glm::vec3(1, 0, 0));
			case PlaneDirection::Bottom:
				return glm::angleAxis(glm::radians(-90.0f), glm::vec3(1, 0, 0));
		}
	}

	inline Mesh GeneratePlane(float width = 1.0f, float height = 1.0f, const glm::quat& rotation = glm::quat(1, 0, 0, 0)) {
		float w = width * 0.5f;
		float h = height * 0.5f;
		
		std::vector<Vertex> vertices = {
			Vertex(-w, -h, 0,  0, 0, 1,  0, 0),
			Vertex(-w,  h, 0,  0, 0, 1,  0, 1),
			Vertex( w,  h, 0,  0, 0, 1,  1, 1),
			Vertex( w, -h, 0,  0, 0, 1,  1, 0),
		};
		
		for(auto& v : vertices) {
			glm::vec3 pos(v.position.x, v.position.y, v.position.z);
			glm::vec3 norm(v.normal.x, v.normal.y, v.normal.z);
			
			pos = rotation * pos;
			norm = rotation * norm;
			
			v.position = pos;
			v.nx = norm.x; v.ny = norm.y; v.nz = norm.z;
		}
		
		std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 3};
		return Mesh(vertices, indices);
	}

	inline Mesh GeneratePlane(float width = 1.0f, float height = 1.0f, PlaneDirection direction = PlaneDirection::Front) {
		return GeneratePlane(width, height, GetRotationFromDirection(direction));
	}

	inline Mesh GenerateCube(float size = 1.0f) {
		std::vector<Vertex> allVertices;
		std::vector<uint32_t> allIndices;
		
		PlaneDirection directions[] = {
			PlaneDirection::Front,
			PlaneDirection::Back,
			PlaneDirection::Left,
			PlaneDirection::Right,
			PlaneDirection::Top,
			PlaneDirection::Bottom
		};
		
		for(auto direction : directions) {
			Mesh plane = GeneratePlane(size, size, direction);
			uint32_t vertexOffset = allVertices.size();
			
			allVertices.insert(allVertices.end(), plane.vertices.begin(), plane.vertices.end());
			for(uint32_t idx : plane.indices)
				allIndices.push_back(idx + vertexOffset);
		}
		
		return Mesh(allVertices, allIndices);
	}

} // namespace fe::Primitives