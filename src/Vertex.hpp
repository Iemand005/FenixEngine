#pragma once

#include <vector>
#include <array>
#include <cstdint>

#include <glm/glm.hpp>
#ifdef FE_HAS_VULKAN
#include <vulkan/vulkan.h>
#endif


namespace fe {

enum class VertexAttribType : uint8_t {
    Float = 0,
    Short = 1,
    UByte = 2,
};

struct VertexAttribute {
    int location;  
    int components;
    size_t offset;
    VertexAttribType type = VertexAttribType::Float;
};

// struct IVertex {
// public:
// 	virtual static std::vector<VertexAttribute> getLayout() = 0;
// };

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
					{ 0, 3, offsetof(Vertex, position), VertexAttribType::Float },
					{ 1, 3, offsetof(Vertex, normal), VertexAttribType::Float },
					{ 2, 2, offsetof(Vertex, uv), VertexAttribType::Float }
			};
		}

#ifdef FE_HAS_VULKAN
	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription binding{};
		binding.binding = 0;
		binding.stride = sizeof(Vertex);
		binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return binding;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attrs{};
		attrs[0].binding = 0;
		attrs[0].location = 0;
		attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attrs[0].offset = offsetof(Vertex, position);

		attrs[1].binding = 0;
		attrs[1].location = 1;
		attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attrs[1].offset = offsetof(Vertex, normal);

		attrs[2].binding = 0;
		attrs[2].location = 2;
		attrs[2].format = VK_FORMAT_R32G32_SFLOAT;
		attrs[2].offset = offsetof(Vertex, uv);

		return attrs;
	}
#endif
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
					{ 0, 3, offsetof(VertexArray, position), VertexAttribType::Float },
					{ 1, 3, offsetof(VertexArray, normal), VertexAttribType::Float },
					{ 2, 3, offsetof(VertexArray, texCoord), VertexAttribType::Float }
			};
	}
};

template<typename T>
struct VertexTraits;

template<>
struct VertexTraits<Vertex> {
	static glm::vec3 getPosition(const Vertex& v) { return v.position; }
};

template<>
struct VertexTraits<VertexArray> {
	static glm::vec3 getPosition(const VertexArray& v) { return v.position; }
};

}