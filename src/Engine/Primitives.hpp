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
	struct UVRect {
		float u0;
		float v0;
		float u1;
		float v1;
	};

	struct CubeUVs {
		UVRect front;
		UVRect back;
		UVRect left;
		UVRect right;
		UVRect top;
		UVRect bottom;
	};
}
namespace fe::Primitives {

	static const CubeUVs defaultUVs = {
		{0, 0, 1, 1},  // front
		{0, 0, 1, 1},  // back
		{0, 0, 1, 1},  // left
		{0, 0, 1, 1},  // right
		{0, 0, 1, 1},  // top
		{0, 0, 1, 1}   // bottom
	};

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

	inline const UVRect& GetUVForDirection(const CubeUVs& uvs, PlaneDirection direction) {
		switch(direction) {
			case PlaneDirection::Front:
				return uvs.front;
			case PlaneDirection::Back:
				return uvs.back;
			case PlaneDirection::Left:
				return uvs.left;
			case PlaneDirection::Right:
				return uvs.right;
			case PlaneDirection::Top:
				return uvs.top;
			case PlaneDirection::Bottom:
				return uvs.bottom;
			default:
				return uvs.front;
		}
	}

	
	inline Mesh GeneratePlane(float width = 1.0f, float height = 1.0f, const glm::quat& rotation = glm::quat(1, 0, 0, 0), UVRect uv = {0, 0, 1, 1}) {
		float w = width * 0.5f;
		float h = height * 0.5f;
		
		std::vector<Vertex> vertices = {
			Vertex(-w,0,-h, 0,1,0, uv.u0, uv.v0),
			Vertex(-w,0, h, 0,1,0, uv.u0, uv.v1),
			Vertex( w,0, h, 0,1,0, uv.u1, uv.v1),
			Vertex( w,0,-h, 0,1,0, uv.u1, uv.v0),
		};
		
		for(auto& v : vertices) {
			v.position = rotation * v.position;
			v.normal = rotation * v.normal;
		}
		
		std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 3};
		return Mesh(vertices, indices);
	}
	
	inline Mesh GeneratePlane(PlaneDirection direction, float width = 1.0f, float height = 1.0f, UVRect uv = {0, 0, 1, 1}) {
		return GeneratePlane(width, height, GetRotationFromDirection(direction), uv);
	}

	inline Mesh GenerateCube(const std::vector<PlaneDirection>& directions, float size = 1.0f, float inset = 0.0f) {
    static const CubeUVs defaultUVs = {
        {0, 0, 1, 1}, {0, 0, 1, 1}, {0, 0, 1, 1},
        {0, 0, 1, 1}, {0, 0, 1, 1}, {0, 0, 1, 1}
    };
    return GenerateCube(directions, defaultUVs, size, inset);
}
	
	inline Mesh GenerateCube(const std::vector<PlaneDirection>& directions, const CubeUVs& uvs, float size = 1.0f, float inset = 0.0f) {
		std::vector<Vertex> allVertices;
		std::vector<uint32_t> allIndices;
		
		float offset = size / 2.0f - inset;
		
		for(auto direction : directions) {
			UVRect faceUV = GetUVForDirection(uvs, direction);
			Mesh plane = GeneratePlane(direction, size, size, faceUV);
			
			glm::vec3 planeOffset = glm::vec3(0.0f);
			bool flipWinding = false;
			
			switch(direction) {
				case PlaneDirection::Front:  
					planeOffset = glm::vec3(0, 0, -offset); 
					break;
				case PlaneDirection::Back:   
					planeOffset = glm::vec3(0, 0, offset); 
					break;
				case PlaneDirection::Right:  
					planeOffset = glm::vec3(-offset, 0, 0);
					break;
				case PlaneDirection::Left:   
					planeOffset = glm::vec3(offset, 0, 0);
					break;
				case PlaneDirection::Top:    planeOffset = glm::vec3(0, offset, 0); break;
				case PlaneDirection::Bottom: planeOffset = glm::vec3(0, -offset, 0); break;
			}
			
			for(auto& vertex : plane.vertices) {
				vertex.position += planeOffset;
				
				switch(direction) {
					case PlaneDirection::Back:
						vertex.uv.x = 1.0f - vertex.uv.x;
						vertex.uv.y = 1.0f - vertex.uv.y;
						break;
					case PlaneDirection::Right:
					case PlaneDirection::Left:
						std::swap(vertex.uv.x, vertex.uv.y);
						if(direction == PlaneDirection::Left)
							vertex.uv.y = 1.0f - vertex.uv.y;
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
			
			if(flipWinding) {
				allIndices.push_back(vertexOffset + 0);
				allIndices.push_back(vertexOffset + 2);
				allIndices.push_back(vertexOffset + 1);
				allIndices.push_back(vertexOffset + 0);
				allIndices.push_back(vertexOffset + 3);
				allIndices.push_back(vertexOffset + 2);
			} else {
				for(uint32_t idx : plane.indices)
					allIndices.push_back(idx + vertexOffset);
			}
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