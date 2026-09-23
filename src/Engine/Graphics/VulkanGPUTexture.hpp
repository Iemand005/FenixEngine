#pragma once

#include "IGPUTexture.hpp"
#include "VulkanUtils.hpp"
#include "../ImageLoader.hpp"

#include <vulkan/vulkan.h>
#include <cstring>
#include <string>
#include <vector>

namespace fe {

class VulkanGPUTexture : public IGPUTexture {
public:
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory imageMemory = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;
	VkSampler sampler = VK_NULL_HANDLE;
	bool arrayTexture = false;
	int layerCount_ = 0;

	~VulkanGPUTexture() override {}

	bool isTextureArray() const override { return arrayTexture; }
	int getLayerCount() const override { return layerCount_; }

	void destroy(VkDevice device) {
		if (sampler != VK_NULL_HANDLE) vkDestroySampler(device, sampler, nullptr);
		if (imageView != VK_NULL_HANDLE) vkDestroyImageView(device, imageView, nullptr);
		if (image != VK_NULL_HANDLE) vkDestroyImage(device, image, nullptr);
		if (imageMemory != VK_NULL_HANDLE) vkFreeMemory(device, imageMemory, nullptr);
		image = VK_NULL_HANDLE;
		imageMemory = VK_NULL_HANDLE;
		imageView = VK_NULL_HANDLE;
		sampler = VK_NULL_HANDLE;
	}

	void upload(VkDevice device, VkPhysicalDevice physicalDevice,
				VkCommandPool commandPool, VkQueue graphicsQueue,
				const std::string& textureFilePath, TextureScaling scaling = TextureScaling::Linear) {

		auto textureImage = fe::ImageLoader::Load(textureFilePath);
		if (textureImage.pixels.size() == 0) return;
		uploadFromImage(device, physicalDevice, commandPool, graphicsQueue, textureImage, scaling);
	}

	void upload(VkDevice device, VkPhysicalDevice physicalDevice,
				VkCommandPool commandPool, VkQueue graphicsQueue,
				const ImageData& imageData, TextureScaling scaling = TextureScaling::Linear) {
		if (imageData.pixels.empty()) return;
		uploadFromImage(device, physicalDevice, commandPool, graphicsQueue, imageData, scaling);
	}

	void uploadFromImage(VkDevice device, VkPhysicalDevice physicalDevice,
				VkCommandPool commandPool, VkQueue graphicsQueue,
				const ImageData& imageData, TextureScaling scaling) {

		uint32_t w = static_cast<uint32_t>(imageData.width);
		uint32_t h = static_cast<uint32_t>(imageData.height);
		VkDeviceSize imageSize = static_cast<VkDeviceSize>(w) * h * imageData.channels;
		VkFormat format = (imageData.channels == 4) ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8_SRGB;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;
		fe::vk::CreateBuffer(device, physicalDevice, imageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingMemory);

		void* mapped;
		vkMapMemory(device, stagingMemory, 0, imageSize, 0, &mapped);
		memcpy(mapped, imageData.pixels.data(), static_cast<size_t>(imageSize));
		vkUnmapMemory(device, stagingMemory);

		fe::vk::CreateImage(device, physicalDevice, w, h, format, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, this->image, this->imageMemory);

		fe::vk::TransitionImageLayout(device, commandPool, graphicsQueue, this->image,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		fe::vk::CopyBufferToImage(device, commandPool, graphicsQueue, stagingBuffer, this->image, w, h);

		fe::vk::TransitionImageLayout(device, commandPool, graphicsQueue, this->image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingMemory, nullptr);

		this->imageView = fe::vk::CreateImageView(device, this->image, format);

		VkFilter vkFilter = (scaling == TextureScaling::Nearest) ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
		this->sampler = fe::vk::CreateSampler(device, vkFilter);
	}

	void uploadTextureArray(VkDevice device, VkPhysicalDevice physicalDevice,
				VkCommandPool commandPool, VkQueue graphicsQueue,
				const std::vector<std::string>& textureFilePaths, TextureScaling scaling = TextureScaling::Linear) {

		if (textureFilePaths.empty()) return;

		auto firstImage = fe::ImageLoader::Load(textureFilePaths[0]);
		if (firstImage.pixels.empty()) return;

		uint32_t w = static_cast<uint32_t>(firstImage.width);
		uint32_t h = static_cast<uint32_t>(firstImage.height);
		int nrChannels = firstImage.channels;
		VkFormat format = (nrChannels == 4) ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8_SRGB;
		uint32_t layers = static_cast<uint32_t>(textureFilePaths.size());
		VkDeviceSize layerSize = static_cast<VkDeviceSize>(w) * h * nrChannels;
		VkDeviceSize totalSize = layerSize * layers;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;
		fe::vk::CreateBuffer(device, physicalDevice, totalSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingMemory);

		void* mapped;
		vkMapMemory(device, stagingMemory, 0, totalSize, 0, &mapped);
		auto* dst = static_cast<uint8_t*>(mapped);
		memcpy(dst, firstImage.pixels.data(), static_cast<size_t>(layerSize));

		for (uint32_t i = 1; i < layers; ++i) {
			auto img = fe::ImageLoader::Load(textureFilePaths[i]);
			if (!img.pixels.empty() && img.width == firstImage.width && img.height == firstImage.height && img.channels == nrChannels) {
				memcpy(dst + i * layerSize, img.pixels.data(), static_cast<size_t>(layerSize));
			}
		}
		vkUnmapMemory(device, stagingMemory);

		fe::vk::CreateImage(device, physicalDevice, w, h, format, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, this->image, this->imageMemory, layers);

		fe::vk::TransitionImageLayout(device, commandPool, graphicsQueue, this->image,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layers);

		fe::vk::CopyBufferToImage(device, commandPool, graphicsQueue, stagingBuffer, this->image, w, h, layers);

		fe::vk::TransitionImageLayout(device, commandPool, graphicsQueue, this->image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, layers);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingMemory, nullptr);

		this->imageView = fe::vk::CreateImageView(device, this->image, format, VK_IMAGE_ASPECT_COLOR_BIT, layers);

		VkFilter vkFilter = (scaling == TextureScaling::Nearest) ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
		this->sampler = fe::vk::CreateSampler(device, vkFilter);

		this->arrayTexture = true;
		this->layerCount_ = static_cast<int>(layers);
	}

	bool load(const std::string& textureFilePath, TextureScaling scaling = TextureScaling::Linear) override {
		return true;
	}
};

}