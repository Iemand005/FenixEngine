#pragma once
#include "RenderDevice.hpp"

#include <vulkan/vulkan.h>

class VulkanDevice : public RenderDevice {
public:
	void Init() override {

	}
    // VertexBuffer* CreateVertexBuffer(void* data, size_t size) override {
    //     return new VulkanVertexBuffer(data, size); // Uses vkCreateBuffer, vkBindBufferMemory
    // }

	~RenderDevice() override {}
	void SubmitFrame() override {}	
};