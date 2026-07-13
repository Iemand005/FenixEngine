#pragma once

#include "IGPUBuffers.hpp"
#include <vulkan/vulkan.h>

class VulkanGPUBuffers : public IGPUBuffers {
public:
	VkBuffer vertexBuffer = VK_NULL_HANDLE;
	VkBuffer indexBuffer = VK_NULL_HANDLE;

	void bind() const override {}
};
