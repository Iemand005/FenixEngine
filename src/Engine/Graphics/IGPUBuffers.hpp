#pragma once
#include <vector>
#include <memory>
#include <cstdint>

class IGPUBuffers {
public:
	int indexCount = 0;

	IGPUBuffers() = default;
	IGPUBuffers(uint32_t count) : indexCount(count) {}

    virtual ~IGPUBuffers() = default;

	virtual void bind() const = 0;
};
