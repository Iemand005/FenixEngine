#pragma once

#include <vector>

#include <glm/glm.hpp>


namespace fe {

struct VertexAttribute {
    int location;  
    int components;
    size_t offset; 
};

struct IVertex {
public:
	virtual static std::vector<VertexAttribute> getLayout() = 0;
};

struct Vertex {
public:
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;

	Vertex() {}

	Vertex(float x, float y, float z, float nx, float ny, float nz, float u, float v) {
		this->position = glm::vec3(x, y, z);
		this->normal = glm::vec3(nx, ny, nz);
		this->uv = glm::vec2(u, v);
	}

	static std::vector<VertexAttribute> getLayout() {
			return {
					{ 0, 3, offsetof(Vertex, position) }, // 3 floats voor pos
					{ 1, 3, offsetof(Vertex, normal) },   // 3 floats voor normal
					{ 2, 2, offsetof(Vertex, uv) }        // 2 floats voor UV (2D)
			};
	}
};


struct VertexArray {
public:
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 texCoord;

	VertexArray() {}

	VertexArray(float x, float y, float z, float nx, float ny, float nz, float u, float v, float layer) {
		this->position = glm::vec3(x, y, z);
		this->normal = glm::vec3(nx, ny, nz);
		this->texCoord = glm::vec3(u, v, layer);
	}

	static std::vector<VertexAttribute> getLayout() {
			return {
					{ 0, 3, offsetof(VertexArray, position) },
					{ 1, 3, offsetof(VertexArray, normal) },
					{ 2, 3, offsetof(VertexArray, texCoord) }        // 3 floats voor UV (3D!)
			};
	}
};

}