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
		const float PI = 3.14159265359f;
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		for (int h = 0; h <= heightSegments; h++) {
			float y = (h / (float)heightSegments) * height - height * 0.5f;
			float vCoord = h / (float)heightSegments;

			for (int i = 0; i < segments; i++) {
				float angle = (i / (float)segments) * 2.0f * PI;
				float uCoord = i / (float)segments;

				float x = radius * cos(angle);
				float z = radius * sin(angle);

				float nx = cos(angle);
				float ny = 0.0f;
				float nz = sin(angle);

				vertices.push_back(Vertex(x, y, z, nx, ny, nz, uCoord, vCoord));
			}
		}

		for (int h = 0; h < heightSegments; h++) {
			for (int i = 0; i < segments; i++) {
				int current = h * segments + i;
				int next = h * segments + ((i + 1) % segments);
				int currentTop = (h + 1) * segments + i;
				int nextTop = (h + 1) * segments + ((i + 1) % segments);

				indices.push_back(current);
				indices.push_back(next);
				indices.push_back(currentTop);

				indices.push_back(next);
				indices.push_back(nextTop);
				indices.push_back(currentTop);
			}
		}

		return Mesh(vertices, indices);
	}

	glm::vec3 CatmullRom(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t) {
		float t2 = t * t;
		float t3 = t2 * t;
		return 0.5f * (
			(2.0f * p1) +
			(-p0 + p2) * t +
			(2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
			(-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
		);
	}

	inline glm::vec3 CubicBezier(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t) {
		float u = 1.0f - t;
		float u2 = u * u;
		float t2 = t * t;
		return u2 * u * p0 + 3.0f * u2 * t * p1 + 3.0f * u * t2 * p2 + t2 * t * p3;
	}

	inline glm::vec3 CubicBezierTangent(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t) {
		float u = 1.0f - t;
		float u2 = u * u;
		float t2 = t * t;
		return 3.0f * u2 * (p1 - p0) + 6.0f * u * t * (p2 - p1) + 3.0f * t2 * (p3 - p2);
	}

	inline Mesh GenerateBentTunnel(
		const std::vector<glm::vec3>& path,
		float radius = 1.0f,
		int segments = 32,
		int subdivisionsPerSegment = 12,
		bool insideOut = false,
		glm::vec3* inUp = nullptr,
		glm::vec3* inRight = nullptr,
		glm::vec3* outUp = nullptr,
		glm::vec3* outRight = nullptr)
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<glm::vec3> smoothPath;
		const float PI = 3.14159265359f;

		int totalSamples = subdivisionsPerSegment * (int)(path.size() - 1);
		for (int sub = 0; sub < totalSamples; sub++) {
			float t = sub / (float)totalSamples;
			smoothPath.push_back(CubicBezier(path[0], path[1], path[2], path[3], t));
		}
		smoothPath.push_back(CubicBezier(path[0], path[1], path[2], path[3], 1.0f));

		glm::vec3 prevForward(0.0f);
		glm::vec3 right = inRight ? *inRight : glm::vec3(1, 0, 0);
		glm::vec3 up = inUp ? *inUp : glm::vec3(0, 1, 0);

		for (size_t p = 0; p < smoothPath.size(); p++) {
			glm::vec3 pos = smoothPath[p];
			glm::vec3 forward;
			if (p == smoothPath.size() - 1) {
				forward = glm::normalize(smoothPath[p] - smoothPath[p-1]);
			} else {
				forward = glm::normalize(smoothPath[p+1] - smoothPath[p]);
			}

			if (p > 0) {
				float cosAng = glm::clamp(glm::dot(prevForward, forward), -1.0f, 1.0f);
				if (cosAng < 0.9999f) {
					glm::vec3 axis = glm::normalize(glm::cross(prevForward, forward));
					float angle = acos(cosAng);
					glm::quat q = glm::angleAxis(angle, axis);
					right = glm::normalize(q * right);
					up = glm::normalize(q * up);
				}
			} else if (!inRight) {
				glm::vec3 r = glm::cross(up, forward);
				if (glm::length(r) < 1e-6f) {
					up = glm::vec3(0, 0, 1);
					r = glm::cross(up, forward);
				}
				right = glm::normalize(r);
			}
			right = glm::normalize(right - glm::dot(right, forward) * forward);
			up = glm::cross(forward, right);
			prevForward = forward;

			for (int i = 0; i < segments; i++) {
				float angle = (i / (float)segments) * 2.0f * PI;
				float x = cos(angle);
				float y = sin(angle);
				glm::vec3 offset = right * x * radius + up * y * radius;
				glm::vec3 vPos = pos + offset;
				glm::vec3 normal = glm::normalize(offset);
				if (insideOut) normal = -normal;

				vertices.push_back(Vertex(vPos.x, vPos.y, vPos.z, normal.x, normal.y, normal.z,
										  i / (float)segments, p / (float)(smoothPath.size()-1)));
			}
		}

		for (size_t p = 0; p < smoothPath.size() - 1; p++) {
			for (int i = 0; i < segments; i++) {
				int current = p * segments + i;
				int next = p * segments + ((i + 1) % segments);
				int currentNext = (p + 1) * segments + i;
				int nextNext = (p + 1) * segments + ((i + 1) % segments);

				if (insideOut) {
					indices.push_back(currentNext);
					indices.push_back(next);
					indices.push_back(current);
					indices.push_back(currentNext);
					indices.push_back(nextNext);
					indices.push_back(next);
				} else {
					indices.push_back(current);
					indices.push_back(next);
					indices.push_back(currentNext);
					indices.push_back(next);
					indices.push_back(nextNext);
					indices.push_back(currentNext);
				}
			}
		}
		if (outUp) *outUp = up;
		if (outRight) *outRight = right;
		return Mesh(vertices, indices);
	}

	inline glm::vec3 GetPositionAlongPath(const std::vector<glm::vec3>& path, float progress) {
		progress = glm::clamp(progress, 0.0f, 1.0f);

		float scaledProgress = progress * (path.size() - 1);
		int currentSegment = (int)scaledProgress;
		int nextSegment = currentSegment + 1;

		if (nextSegment >= path.size()) {
			nextSegment = path.size() - 1;
			currentSegment = path.size() - 2;
		}

		float blend = scaledProgress - currentSegment;
		return glm::mix(path[currentSegment], path[nextSegment], blend);
	}

} // namespace fe::Primitives
