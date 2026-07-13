#pragma once

#include "IGPUBuffers.hpp"

class VulkanGPUBuffers : public IGPUBuffers {
public:
	void bind() const override {}
};