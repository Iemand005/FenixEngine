#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "RenderDevice.hpp"

class VulkanDevice : public RenderDevice {
public:
	void Init() override {

	}
    // VertexBuffer* CreateVertexBuffer(void* data, size_t size) override {
    //     return new VulkanVertexBuffer(data, size); // Uses vkCreateBuffer, vkBindBufferMemory
    // }

	void SubmitFrame() override {}	
};