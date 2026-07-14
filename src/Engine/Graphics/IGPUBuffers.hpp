#pragma once
#include <vector>
#include <memory>
#include <cstdint>

enum class VertexFormat {
	Standard,  // fe::Vertex: vec3 pos, vec3 normal, vec2 uv (32 bytes)
	Array,     // fe::VertexArray: vec3 pos, vec3 normal, vec3 texCoord (36 bytes)
};

class IGPUBuffers {
public:
	int indexCount = 0;
	VertexFormat vertexFormat = VertexFormat::Standard;

	IGPUBuffers() = default;
	IGPUBuffers(uint32_t count) : indexCount(count) {}

    virtual ~IGPUBuffers() = default;

	virtual void bind() const = 0;
};
