#pragma once
#include <vector>
#include <memory>

// #include <MeshArray.hpp>

// struct Vertex { /* ... */ };

class IGPUBuffers {
public:
    virtual ~IGPUBuffers() = default;

	virtual void bind();
};
