#pragma once
#include <vector>
#include <memory>

// #include <MeshArray.hpp>

// struct Vertex { /* ... */ };

class IGPUBuffers {
public:
	int indexCount;

	IGPUBuffers(uint32_t count) : indexCount(count) {}

    virtual ~IGPUBuffers() = default;

	virtual void bind() const;
};
