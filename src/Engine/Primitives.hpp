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
        case PlaneDirection::Front:
            // Looking at front, already correct orientation
            return glm::angleAxis(glm::radians(-90.0f), glm::vec3(1, 0, 0));
        case PlaneDirection::Back:
            // Flip 180° on Y, then rotate to face back
            return glm::angleAxis(glm::radians(90.0f), glm::vec3(1, 0, 0)) * 
                   glm::angleAxis(glm::radians(180.0f), glm::vec3(0, 1, 0));
        case PlaneDirection::Right:
            // Rotate 90° around Z, then adjust
            return glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 1, 0)) *
                   glm::angleAxis(glm::radians(-90.0f), glm::vec3(1, 0, 0));
        case PlaneDirection::Left:
            return glm::angleAxis(glm::radians(-90.0f), glm::vec3(0, 1, 0)) *
                   glm::angleAxis(glm::radians(-90.0f), glm::vec3(1, 0, 0));
        default:
        case PlaneDirection::Top:
            return glm::quat(1, 0, 0, 0); // No rotation needed
        case PlaneDirection::Bottom:
            return glm::angleAxis(glm::radians(180.0f), glm::vec3(1, 0, 0));
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

	
	// TODO change size to a vec3 and do that shit
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

			for(auto& vertex : plane.vertices)
        		vertex.position += planeOffset;
			
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

	inline Mesh GenerateCube(const std::vector<PlaneDirection>& directions, float size = 1.0f, float inset = 0.0f) {
		static const CubeUVs defaultUVs = {
			{0, 0, 1, 1}, {0, 0, 1, 1}, {0, 0, 1, 1},
			{0, 0, 1, 1}, {0, 0, 1, 1}, {0, 0, 1, 1}
		};
		return GenerateCube(directions, defaultUVs, size, inset);
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

	inline Mesh GenerateCube(const CubeUVs& uvs, float size = 1.0f) {
		std::vector<PlaneDirection> directions = {
			PlaneDirection::Front,
			PlaneDirection::Back,
			PlaneDirection::Left,
			PlaneDirection::Right,
			PlaneDirection::Top,
			PlaneDirection::Bottom
		};
		
		return GenerateCube(directions, uvs, size);
	}

	inline Mesh GenerateTunnel(float radius = 1.0f, float height = 10.0f,
							   int segments = 32, int heightSegments = 10) {
		Mesh mesh;
		const float PI = 3.14159265359f;

		for (int h = 0; h <= heightSegments; h++) {
			float y = (h / (float)heightSegments) * height;
			float vCoord = h / (float)heightSegments;

			for (int i = 0; i < segments; i++) {
				float angle = (i / (float)segments) * 2.0f * PI;
				float uCoord = i / (float)segments;

				// Position on cylinder surface
				float x = radius * cos(angle);
				float z = radius * sin(angle);

				// Normal points outward (radial direction)
				float nx = cos(angle);
				float ny = 0.0f;
				float nz = sin(angle);

				mesh.vertices.push_back(Vertex(x, y, z, nx, ny, nz, uCoord, vCoord));
			}
		}

		for (int h = 0; h < heightSegments; h++) {
			for (int i = 0; i < segments; i++) {
				// Current and next segment indices
				int current = h * segments + i;
				int next = h * segments + ((i + 1) % segments);
				int currentTop = (h + 1) * segments + i;
				int nextTop = (h + 1) * segments + ((i + 1) % segments);

				// First triangle
				mesh.indices.push_back(current);
				mesh.indices.push_back(currentTop);
				mesh.indices.push_back(next);

				// Second triangle
				mesh.indices.push_back(next);
				mesh.indices.push_back(currentTop);
				mesh.indices.push_back(nextTop);
			}
		}

		return mesh;
	}

} // namespace fe::Primitives
