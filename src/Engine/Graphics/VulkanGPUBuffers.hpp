#pragma once

#include "IGPUBuffers.hpp"
#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>
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
			vertices, vertexStride * vertexCount,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			vertexBuffer, vertexBufferMemory);

		createAndUploadBuffer(device, physicalDevice, commandPool, graphicsQueue,
			indices, sizeof(uint32_t) * idxCount,
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
	static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}
		throw std::runtime_error("Failed to find suitable memory type.");
	}

	static void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
		VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
		VkBuffer& buffer, VkDeviceMemory& bufferMemory) {

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Vulkan buffer.");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate Vulkan buffer memory.");
		}
		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

	static void createAndUploadBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
		VkCommandPool commandPool, VkQueue graphicsQueue,
		const void* data, VkDeviceSize dataSize, VkBufferUsageFlags usage,
		VkBuffer& outBuffer, VkDeviceMemory& outMemory) {

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;
		createBuffer(device, physicalDevice, dataSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingMemory);

		void* mapped;
		vkMapMemory(device, stagingMemory, 0, dataSize, 0, &mapped);
		memcpy(mapped, data, static_cast<size_t>(dataSize));
		vkUnmapMemory(device, stagingMemory);

		createBuffer(device, physicalDevice, dataSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			outBuffer, outMemory);

		copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, outBuffer, dataSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingMemory, nullptr);
	}

	static void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
		VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}
};
