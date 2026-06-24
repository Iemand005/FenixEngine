#pragma once
#include "Mesh.hpp"

namespace fe {
	enum class PlaneDirection {
		Front,   // +Z
		Back,    // -Z
		Right,   // +X
		Left,    // -X
		Top,     // +Y
		Bottom   // -Y
	};
}

namespace fe::Primitives {
	inline glm::quat GetRotationFromDirection(PlaneDirection direction) {
		switch(direction) {
			default:
			case PlaneDirection::Top:
				return glm::quat(1, 0, 0, 0);
			case PlaneDirection::Bottom:
				return glm::angleAxis(glm::radians(180.0f), glm::vec3(1, 0, 0));
			case PlaneDirection::Front:
				return glm::angleAxis(glm::radians(-90.0f), glm::vec3(1, 0, 0));
			case PlaneDirection::Back:
				return glm::angleAxis(glm::radians(90.0f), glm::vec3(1, 0, 0));
			case PlaneDirection::Right:
				return glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 0, 1));
			case PlaneDirection::Left:
				return glm::angleAxis(glm::radians(-90.0f), glm::vec3(0, 0, 1));
		}
	}
	inline Mesh GeneratePlane(float width = 1.0f, float height = 1.0f, const glm::quat& rotation = glm::quat(1, 0, 0, 0)) {
		float w = width * 0.5f;
		float h = height * 0.5f;
		
		std::vector<Vertex> vertices = {
			Vertex(-w, 0, -h,  0, 1, 0,  0, 0),
			Vertex(-w, 0,  h,  0, 1, 0,  0, 1),
			Vertex( w, 0,  h,  0, 1, 0,  1, 1),
			Vertex( w, 0, -h,  0, 1, 0,  1, 0),
		};
		
		for(auto& v : vertices) {
			v.position = rotation * v.position;
			v.normal = rotation * v.normal;
		}
		
		std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 3};
		return Mesh(vertices, indices);
	}
	
	inline Mesh GeneratePlane(PlaneDirection direction, float width = 1.0f, float height = 1.0f) {
		return GeneratePlane(width, height, GetRotationFromDirection(direction));
	}

	inline Mesh GenerateCube(const std::vector<PlaneDirection>& directions, float size = 1.0f, float inset = 0.0f) {
		std::vector<Vertex> allVertices;
		std::vector<uint32_t> allIndices;
		
		float offset = size / 2.0f + inset;
		
		for(auto direction : directions) {
			Mesh plane = GeneratePlane(direction, size, size);
			
			glm::vec3 planeOffset = glm::vec3(0.0f);
			switch(direction) {
				case PlaneDirection::Front:  planeOffset = glm::vec3(0, 0, offset); break;
				case PlaneDirection::Back:   planeOffset = glm::vec3(0, 0, -offset); break;
				case PlaneDirection::Right:  planeOffset = glm::vec3(offset, 0, 0); break;
				case PlaneDirection::Left:   planeOffset = glm::vec3(-offset, 0, 0); break;
				case PlaneDirection::Top:    planeOffset = glm::vec3(0, offset, 0); break;
				case PlaneDirection::Bottom: planeOffset = glm::vec3(0, -offset, 0); break;
			}
			
			for(auto& vertex : plane.vertices) {
				vertex.position += planeOffset;
				
				switch(direction) {
					case PlaneDirection::Back:
						vertex.uv.x = 1.0f - vertex.uv.x;
						break;
					case PlaneDirection::Right:
					case PlaneDirection::Left:
						std::swap(vertex.uv.x, vertex.uv.y);
						break;
					case PlaneDirection::Top:
					case PlaneDirection::Bottom:
						vertex.uv.y = 1.0f - vertex.uv.y;
						break;
					default:
						break;
				}
			}
			
			uint32_t vertexOffset = allVertices.size();
			allVertices.insert(allVertices.end(), plane.vertices.begin(), plane.vertices.end());
			for(uint32_t idx : plane.indices)
				allIndices.push_back(idx + vertexOffset);
		}
		
		return Mesh(allVertices, allIndices);
	}

	inline Mesh GenerateCube(float size = 1.0f) {
		std::vector<PlaneDirection> directions = {
			PlaneDirection::Front,
			PlaneDirection::Back,
			PlaneDirection::Left,
			PlaneDirection::Right,
			PlaneDirection::Top,
			PlaneDirection::Bottom
		};
		
		return GenerateCube(directions, size);
	}
} // namespace fe::Primitives