#pragma once

#include "IGPUBuffers.hpp"
#include "VulkanUtils.hpp"

#include <vulkan/vulkan.h>
#include <cstring>

class VulkanGPUBuffers : public IGPUBuffers {
public:
	VkBuffer vertexBuffer = VK_NULL_HANDLE;
	VkBuffer indexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
	VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

	void bind() const override {}

	void upload(VkDevice device, VkPhysicalDevice physicalDevice,
				VkCommandPool commandPool, VkQueue graphicsQueue,
				const void* vertices, size_t vertexStride, size_t vertexCount,
				const uint32_t* indices, uint32_t idxCount) {

		indexCount = static_cast<int>(idxCount);

		createAndUploadBuffer(device, physicalDevice, commandPool, graphicsQueue,
			vertices, static_cast<VkDeviceSize>(vertexStride) * vertexCount,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			vertexBuffer, vertexBufferMemory);

		createAndUploadBuffer(device, physicalDevice, commandPool, graphicsQueue,
			indices, static_cast<VkDeviceSize>(sizeof(uint32_t)) * idxCount,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			indexBuffer, indexBufferMemory);
	}

	void destroy(VkDevice device) {
		if (indexBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, indexBuffer, nullptr);
		if (indexBufferMemory != VK_NULL_HANDLE) vkFreeMemory(device, indexBufferMemory, nullptr);
		if (vertexBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, vertexBuffer, nullptr);
		if (vertexBufferMemory != VK_NULL_HANDLE) vkFreeMemory(device, vertexBufferMemory, nullptr);
		vertexBuffer = VK_NULL_HANDLE;
		indexBuffer = VK_NULL_HANDLE;
		vertexBufferMemory = VK_NULL_HANDLE;
		indexBufferMemory = VK_NULL_HANDLE;
	}

private:
	static void createAndUploadBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
		VkCommandPool commandPool, VkQueue graphicsQueue,
		const void* data, VkDeviceSize dataSize, VkBufferUsageFlags usage,
		VkBuffer& outBuffer, VkDeviceMemory& outMemory) {

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;
		fe::vk::CreateBuffer(device, physicalDevice, dataSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingMemory);

		void* mapped;
		vkMapMemory(device, stagingMemory, 0, dataSize, 0, &mapped);
		memcpy(mapped, data, static_cast<size_t>(dataSize));
		vkUnmapMemory(device, stagingMemory);

		fe::vk::CreateBuffer(device, physicalDevice, dataSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			outBuffer, outMemory);

		fe::vk::CopyBuffer(device, commandPool, graphicsQueue, stagingBuffer, outBuffer, dataSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingMemory, nullptr);
	}
};