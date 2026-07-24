#pragma once

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "IRenderDevice.hpp"
#include "IGPUBuffers.hpp"
#include "IGPUTexture.hpp"
#include "VulkanGPUBuffers.hpp"
#include "VulkanGPUTexture.hpp"
#include "../Vertex.hpp"

#include "../window/IWindow.hpp"

#ifdef NDEBUG
constexpr bool kEnableValidationLayers = false;
#else
constexpr bool kEnableValidationLayers = true;
#endif

namespace fe {
const std::vector<const char*> kValidationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> kDeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME,
	VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
	VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
	VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
};

constexpr int kMaxFramesInFlight = 2;
constexpr int kMaxDrawsPerFrame = 2048;
constexpr int kMaxCachedTextures = 1024;

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;
	bool isComplete() const {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities{};
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct VulkanWindowResources {
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    std::vector<VkImageView> imageViews;
};


struct UniformBufferObject {
	glm::mat4 view;
	glm::mat4 proj;
};


class VulkanDevice : public fe::IRenderDevice {
public:
	VkClearValue m_VulkanClearColor{};

	VkDescriptorSet currentBoundTexture_   = VK_NULL_HANDLE;
    VkPipeline       currentBoundPipeline_ = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> frameDescriptorSets_;

    struct TextureDescriptorKey {
        VkImageView imageView;
        VkSampler   sampler;

        bool operator==(const TextureDescriptorKey& other) const {
            return imageView == other.imageView && sampler == other.sampler;
        }
    };

    struct TextureDescriptorKeyHash {
        size_t operator()(const TextureDescriptorKey& k) const {
            size_t h1 = std::hash<void*>{}(reinterpret_cast<void*>(k.imageView));
            size_t h2 = std::hash<void*>{}(reinterpret_cast<void*>(k.sampler));
            return h1 ^ (h2 << 1);
        }
    };

    std::unordered_map<TextureDescriptorKey, VkDescriptorSet, TextureDescriptorKeyHash> textureDescriptorCache_;

    // VkDescriptorSet GetOrCreateTextureDescriptorSet(VkImageView imageView, VkSampler sampler);

	void SetShaderPaths(const std::string& vertPath, const std::string& fragPath) {
		vertShaderPath_ = vertPath;
		fragShaderPath_ = fragPath;
	}

	void SetArrayShaderPaths(const std::string& vertPath, const std::string& fragPath) {
		vertShaderArrayPath_ = vertPath;
		fragShaderArrayPath_ = fragPath;
	}

	void SetFoxcraftShaderPaths(const std::string& vertPath, const std::string& fragPath) {
		vertShaderFoxcraftPath_ = vertPath;
		fragShaderArrayPath_ = fragPath;
	}

	void Init() override {
		CreateInstance();
		// CreateSurface();
		PickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();

		createImageViews();
		createRenderPass();
		createDescriptorSetLayout();
		createGraphicsPipeline(vertShaderPath_, fragShaderPath_, VertexFormat::Standard, graphicsPipeline_);
		createGraphicsPipeline(vertShaderArrayPath_, fragShaderArrayPath_, VertexFormat::Array, graphicsPipelineArray_);
		createGraphicsPipeline(vertShaderFoxcraftPath_, fragShaderArrayPath_, VertexFormat::Foxcraft, graphicsPipelineFoxcraft_);
		createGraphicsPipeline(vertShaderPath_, fragShaderPath_, VertexFormat::Standard, graphicsPipelineCW_, VK_FRONT_FACE_CLOCKWISE);
		createGraphicsPipeline(vertShaderArrayPath_, fragShaderArrayPath_, VertexFormat::Array, graphicsPipelineArrayCW_, VK_FRONT_FACE_CLOCKWISE);
		createGraphicsPipeline(vertShaderFoxcraftPath_, fragShaderArrayPath_, VertexFormat::Foxcraft, graphicsPipelineFoxcraftCW_, VK_FRONT_FACE_CLOCKWISE);

		createGraphicsPipeline(vertShaderPath_, fragShaderPath_, VertexFormat::Standard, graphicsPipelineTransparent_, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE);
		createGraphicsPipeline(vertShaderArrayPath_, fragShaderArrayPath_, VertexFormat::Array, graphicsPipelineArrayTransparent_, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE);
		createGraphicsPipeline(vertShaderFoxcraftPath_, fragShaderArrayPath_, VertexFormat::Foxcraft, graphicsPipelineFoxcraftTransparent_, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE);
		createGraphicsPipeline(vertShaderPath_, fragShaderPath_, VertexFormat::Standard, graphicsPipelineCWTransparent_, VK_FRONT_FACE_CLOCKWISE, VK_FALSE);
		createGraphicsPipeline(vertShaderArrayPath_, fragShaderArrayPath_, VertexFormat::Array, graphicsPipelineArrayCWTransparent_, VK_FRONT_FACE_CLOCKWISE, VK_FALSE);
		createGraphicsPipeline(vertShaderFoxcraftPath_, fragShaderArrayPath_, VertexFormat::Foxcraft, graphicsPipelineFoxcraftCWTransparent_, VK_FRONT_FACE_CLOCKWISE, VK_FALSE);
		createDepthResources();
		createFramebuffers();
		createCommandPool();
		createUniformBuffers();
		createDescriptorPool();
		createDefaultTexture();
		createDescriptorSets();
		createFrameDescriptorSets();
		createCommandBuffers();
		createSyncObjects();
	}
	// VertexBuffer* CreateVertexBuffer(void* data, size_t size) override {
	//     return new VulkanVertexBuffer(data, size); // Uses vkCreateBuffer, vkBindBufferMemory
	// }

	void SubmitFrame() override {
		auto cmd = commandBuffers_[currentFrame_];

		vkCmdEndRenderPass(cmd);

		if (depthReadbackRequested_) {
			auto extent = swapChainExtent_;
			VkDeviceSize needed = static_cast<VkDeviceSize>(extent.width) * extent.height * sizeof(float);
			if (depthStagingBuffer_ == VK_NULL_HANDLE) {
				createBuffer(needed, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					depthStagingBuffer_, depthStagingMemory_);
			}

			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.image = depthImage_;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.layerCount = 1;
			barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

			VkBufferImageCopy copyRegion{};
			copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageExtent = {extent.width, extent.height, 1};
			vkCmdCopyImageToBuffer(cmd, depthImage_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, depthStagingBuffer_, 1, &copyRegion);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

			depthReadbackAvailable_ = false;
		}

		if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record command buffer.");
		}

		VkSemaphore waitSemaphores[] = {imageAvailableSemaphores_[currentFrame_]};
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		VkSemaphore signalSemaphores[] = {renderFinishedSemaphores_[currentImageIndex_]};

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmd;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to submit draw command buffer.");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		VkSwapchainKHR swapChains[] = {swapChain_};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &currentImageIndex_;

		VkResult presentResult = vkQueuePresentKHR(presentQueue_, &presentInfo);
		if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
			recreateSwapChain();
		}

		currentFrame_ = (currentFrame_ + 1) % kMaxFramesInFlight;
	}

	void RegisterWindow(const IWindow* window) override {
			// void* nativeHandle = window->GetWindow();
			
			VulkanWindowResources resources;
			// Create your VkSurfaceKHR using the nativeHandle here...
			// Create your VkSwapchainKHR...
			
			windowRegistry[window] = resources;
		}

		void UnregisterWindow(const IWindow* window) override {
			auto it = windowRegistry.find(window);
			if (it != windowRegistry.end()) {
				// Destroy Vulkan resources using vkDestroySwapchainKHR, vkDestroySurfaceKHR
				windowRegistry.erase(it);
			}
		}

	void Clear() override {
		vkWaitForFences(_device, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

		if (depthReadbackRequested_ && depthStagingBuffer_ != VK_NULL_HANDLE && !depthReadbackAvailable_) {
			void* data;
			vkMapMemory(_device, depthStagingMemory_, 0, VK_WHOLE_SIZE, 0, &data);
			VkFormat depthFormat = findDepthFormat();
			size_t pixelCount = static_cast<size_t>(swapChainExtent_.width) * swapChainExtent_.height;
			cachedDepthData_.resize(pixelCount);
			cachedDepthW_ = static_cast<int>(swapChainExtent_.width);
			cachedDepthH_ = static_cast<int>(swapChainExtent_.height);
			if (depthFormat == VK_FORMAT_D24_UNORM_S8_UINT) {
				const uint32_t* pixels = static_cast<const uint32_t*>(data);
				for (size_t i = 0; i < pixelCount; i++) {
					cachedDepthData_[i] = static_cast<float>(pixels[i] & 0x00FFFFFF) / 16777215.0f;
				}
			} else {
				const float* pixels = static_cast<const float*>(data);
				std::copy(pixels, pixels + pixelCount, cachedDepthData_.begin());
			}
			vkUnmapMemory(_device, depthStagingMemory_);
			depthReadbackAvailable_ = true;
		}

		VkResult result = vkAcquireNextImageKHR(_device, swapChain_, UINT64_MAX,
			imageAvailableSemaphores_[currentFrame_], VK_NULL_HANDLE, &currentImageIndex_);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			vkWaitForFences(_device, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);
			result = vkAcquireNextImageKHR(_device, swapChain_, UINT64_MAX,
				imageAvailableSemaphores_[currentFrame_], VK_NULL_HANDLE, &currentImageIndex_);
			if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
				return;
			}
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			return;
		}

		drawCallCount_ = 0;

		updateUniformBuffer(currentFrame_);

		vkResetFences(_device, 1, &inFlightFences_[currentFrame_]);
		vkResetCommandBuffer(commandBuffers_[currentFrame_], 0);

		auto cmd = commandBuffers_[currentFrame_];

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginCommandBuffer(cmd, &beginInfo);

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = m_VulkanClearColor.color;
		clearValues[1].depthStencil = {1.0f, 0};

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass_;
		renderPassInfo.framebuffer = swapChainFramebuffers_[currentImageIndex_];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swapChainExtent_;
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapChainExtent_.width);
		viewport.height = static_cast<float>(swapChainExtent_.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swapChainExtent_;
		vkCmdSetScissor(cmd, 0, 1, &scissor);
	}

	void SetClearColor(float r, float g, float b, float a = 1) override {
		m_VulkanClearColor.color = {{ r, g, b, a }};
	}

	void Resize(int width, int height) override {
		recreateSwapChain();
	}

	VkDescriptorSet GetOrCreateTextureDescriptorSet(VkImageView imageView, VkSampler sampler) {
		TextureDescriptorKey key{imageView, sampler};

		auto it = textureDescriptorCache_.find(key);
		if (it != textureDescriptorCache_.end()) {
			return it->second;
		}

		// allocate a new descriptor set for this texture (set = 1, texture-only layout)
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool_;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayout_; // your set=1 layout (sampler only)

		VkDescriptorSet newSet = VK_NULL_HANDLE;
		VkResult result = vkAllocateDescriptorSets(_device, &allocInfo, &newSet);
		if (result != VK_SUCCESS) {
			std::cerr << "[VulkanDevice] Failed to allocate texture descriptor set (VkResult=" << result << ")" << std::endl;
			return VK_NULL_HANDLE;
		}

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = imageView;
		imageInfo.sampler = sampler;

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = newSet;
		write.dstBinding = 0; // binding 0 within set 1
		write.dstArrayElement = 0;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.descriptorCount = 1;
		write.pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(_device, 1, &write, 0, nullptr);

		textureDescriptorCache_[key] = newSet;
		return newSet;
	}

	void createFrameDescriptorSets() {
		uint32_t framesInFlight = static_cast<uint32_t>(uniformBuffers_.size()); // or MAX_FRAMES_IN_FLIGHT, whatever you use elsewhere

		std::vector<VkDescriptorSetLayout> layouts(framesInFlight, descriptorSetLayout_);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool_;
		allocInfo.descriptorSetCount = framesInFlight;
		allocInfo.pSetLayouts = layouts.data();

		frameDescriptorSets_.resize(framesInFlight);
		if (vkAllocateDescriptorSets(_device, &allocInfo, frameDescriptorSets_.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate frame (UBO) descriptor sets.");
		}

		// write each one to point at its corresponding uniform buffer
		for (uint32_t i = 0; i < framesInFlight; ++i) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers_[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkWriteDescriptorSet write{};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = frameDescriptorSets_[i];
			write.dstBinding = 0;
			write.dstArrayElement = 0;
			write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write.descriptorCount = 1;
			write.pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(_device, 1, &write, 0, nullptr);
		}
	}

	void BeginFrame() override {
		auto cmd = commandBuffers_[currentFrame_];
		if (!cmd) {
			std::cerr << "[VulkanDevice] BeginFrame: command buffer is null" << std::endl;
			return;
		}

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width  = static_cast<float>(swapChainExtent_.width);
		viewport.height = static_cast<float>(swapChainExtent_.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swapChainExtent_;
		vkCmdSetScissor(cmd, 0, 1, &scissor);
		drawCallCount_ = 0;
		currentBoundPipeline_ = VK_NULL_HANDLE;
	}

	void DrawMesh(const IGPUBuffers* buffers, const fe::IGPUTexture* texture = nullptr) override {
		if (!buffers) return;

		const auto* vkBuffers = dynamic_cast<const VulkanGPUBuffers*>(buffers);
		if (!vkBuffers) {
			std::cerr << "[VulkanDevice] DrawMesh: buffers is not a VulkanGPUBuffers" << std::endl;
			return;
		}
		if (vkBuffers->vertexBuffer == VK_NULL_HANDLE || vkBuffers->indexBuffer == VK_NULL_HANDLE) {
			std::cerr << "[VulkanDevice] DrawMesh: vertexBuffer or indexBuffer is VK_NULL_HANDLE" << std::endl;
			return;
		}

		auto cmd = commandBuffers_[currentFrame_];
		if (!cmd) {
			std::cerr << "[VulkanDevice] DrawMesh: command buffer is null" << std::endl;
			return;
		}

		if (drawCallCount_ >= kMaxDrawsPerFrame) {
			std::cerr << "[VulkanDevice] DrawMesh: exceeded max draws per frame (" << kMaxDrawsPerFrame << ")" << std::endl;
			return;
		}

		uint32_t setIndex = currentFrame_ * kMaxDrawsPerFrame + drawCallCount_;
		drawCallCount_++;

		VkDescriptorSet descriptorSet = descriptorSets_[setIndex];

		VkImageView imageView = defaultImageView_;
		VkSampler sampler = defaultSampler_;
		if (texture) {
			const auto* vkTex = dynamic_cast<const fe::VulkanGPUTexture*>(texture);
			if (vkTex && vkTex->imageView != VK_NULL_HANDLE) {
				imageView = vkTex->imageView;
				sampler = vkTex->sampler;
			}
		}

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffers_[currentFrame_];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = imageView;
		imageInfo.sampler = sampler;

		std::array<VkWriteDescriptorSet, 2> writes{};

		writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet = descriptorSet;
		writes[0].dstBinding = 0;
		writes[0].dstArrayElement = 0;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writes[0].descriptorCount = 1;
		writes[0].pBufferInfo = &bufferInfo;

		writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[1].dstSet = descriptorSet;
		writes[1].dstBinding = 1;
		writes[1].dstArrayElement = 0;
		writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[1].descriptorCount = 1;
		writes[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

		VkBuffer vertexBuffers[] = {vkBuffers->vertexBuffer};
		VkDeviceSize offsets[] = {0};

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout_, 0, 1, &descriptorSet, 0, nullptr);

		VkPipeline requiredPipeline;
		if (transparentMode_) {
			if (vkBuffers->vertexFormat == VertexFormat::Array) {
				requiredPipeline = reverseWinding_ ? graphicsPipelineArrayCWTransparent_ : graphicsPipelineArrayTransparent_;
			} else if (vkBuffers->vertexFormat == VertexFormat::Foxcraft) {
				requiredPipeline = reverseWinding_ ? graphicsPipelineFoxcraftCWTransparent_ : graphicsPipelineFoxcraftTransparent_;
			} else {
				requiredPipeline = reverseWinding_ ? graphicsPipelineCWTransparent_ : graphicsPipelineTransparent_;
			}
		} else {
			if (vkBuffers->vertexFormat == VertexFormat::Array) {
				requiredPipeline = reverseWinding_ ? graphicsPipelineArrayCW_ : graphicsPipelineArray_;
			} else if (vkBuffers->vertexFormat == VertexFormat::Foxcraft) {
				requiredPipeline = reverseWinding_ ? graphicsPipelineFoxcraftCW_ : graphicsPipelineFoxcraft_;
			} else {
				requiredPipeline = reverseWinding_ ? graphicsPipelineCW_ : graphicsPipeline_;
			}
		}
		if (requiredPipeline != currentBoundPipeline_) {
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, requiredPipeline);
			currentBoundPipeline_ = requiredPipeline;
		}

		vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(cmd, vkBuffers->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		struct PushData { glm::mat4 model; glm::vec4 objectColor; } pushData{currentModel_, currentObjectColor_};
		vkCmdPushConstants(cmd, pipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushData), &pushData);
		vkCmdDrawIndexed(cmd, vkBuffers->indexCount, 1, 0, 0, 0);
	}

	void DrawIndirect(VkBuffer vertexBuffer, VkBuffer indexBuffer,
		VkBuffer indirectBuffer, VkDeviceSize indirectOffset,
		uint32_t drawCount, uint32_t stride,
		VkDescriptorSet descriptorSet) {
		auto cmd = commandBuffers_[currentFrame_];
		if (!cmd || drawCount == 0) return;

		VkPipeline requiredPipeline = reverseWinding_ ? graphicsPipelineFoxcraftCW_ : graphicsPipelineFoxcraft_;
		if (requiredPipeline != currentBoundPipeline_) {
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, requiredPipeline);
			currentBoundPipeline_ = requiredPipeline;
		}

		VkBuffer vb = vertexBuffer;
		VkDeviceSize vbOffset = 0;
		vkCmdBindVertexBuffers(cmd, 0, 1, &vb, &vbOffset);
		vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout_, 0, 1, &descriptorSet, 0, nullptr);

		glm::mat4 identity(1.0f);
		glm::vec4 defaultColor(1.0f, 1.0f, 1.0f, 1.0f);
		struct PushData { glm::mat4 model; glm::vec4 objectColor; } pushData{identity, defaultColor};
		vkCmdPushConstants(cmd, pipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0, sizeof(PushData), &pushData);

		vkCmdDrawIndexedIndirect(cmd, indirectBuffer, indirectOffset,
			drawCount, stride);
	}

	void UpdateIndirectDescriptorSet(VkDescriptorSet set, VkImageView textureView, VkSampler sampler) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffers_[currentFrame_];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = textureView;
		imageInfo.sampler = sampler;

		std::array<VkWriteDescriptorSet, 2> writes{};
		writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet = set;
		writes[0].dstBinding = 0;
		writes[0].dstArrayElement = 0;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writes[0].descriptorCount = 1;
		writes[0].pBufferInfo = &bufferInfo;

		writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[1].dstSet = set;
		writes[1].dstBinding = 1;
		writes[1].dstArrayElement = 0;
		writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[1].descriptorCount = 1;
		writes[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	}

	std::unique_ptr<IGPUBuffers> CreateGPUBuffers() override {
		return std::make_unique<VulkanGPUBuffers>();
	}

	std::unique_ptr<fe::IGPUTexture> CreateGPUTexture() override {
		return std::make_unique<fe::VulkanGPUTexture>();
	}

	void UploadBuffers(IGPUBuffers* buffers,
		const void* vertices, size_t vertexStride, size_t vertexCount,
		const uint32_t* indices, uint32_t indexCount,
		const std::vector<fe::VertexAttribute>& layout = {}) override {

		if (!buffers) return;

		auto* vkBuffers = static_cast<VulkanGPUBuffers*>(buffers);
		vkBuffers->upload(_device, _physicalDevice, commandPool_, graphicsQueue_,
			vertices, vertexStride, vertexCount, indices, indexCount);
	}

	void UploadTexture(fe::IGPUTexture* texture,
		const std::string& path, fe::TextureScaling scaling = fe::TextureScaling::Linear) override {
		if (!texture) return;
		auto* vkTexture = static_cast<fe::VulkanGPUTexture*>(texture);
		vkTexture->upload(_device, _physicalDevice, commandPool_, graphicsQueue_, path, scaling);
	}

	void UploadTexture(fe::IGPUTexture* texture,
		const fe::ImageData& image, fe::TextureScaling scaling = fe::TextureScaling::Linear) override {
		if (!texture) return;
		auto* vkTexture = static_cast<fe::VulkanGPUTexture*>(texture);
		vkTexture->upload(_device, _physicalDevice, commandPool_, graphicsQueue_, image, scaling);
	}

	void UploadTextureArray(fe::IGPUTexture* texture,
		const std::vector<std::string>& paths, fe::TextureScaling scaling = fe::TextureScaling::Linear) override {
		if (!texture) return;
		auto* vkTexture = static_cast<fe::VulkanGPUTexture*>(texture);
		vkTexture->uploadTextureArray(_device, _physicalDevice, commandPool_, graphicsQueue_, paths, scaling);
	}

	VkInstance GetInstance() const { return _instance; }
	VkPhysicalDevice GetPhysicalDevice() const { return _physicalDevice; }
	VkDevice GetDevice() const { return _device; }
	VkQueue GetGraphicsQueue() const { return graphicsQueue_; }
	const char* GetDeviceName() const override { return _deviceName.c_str(); }
	uint32_t GetGraphicsQueueFamily() const { return graphicsQueueFamily_; }
	VkRenderPass GetRenderPass() const { return renderPass_; }

	VkImage GetColorAttachmentImage(uint64_t handle) const {
		if (handle == 0) return VK_NULL_HANDLE;
		return reinterpret_cast<const ColorAttachment*>(handle)->image;
	}
	VkCommandPool GetCommandPool() const { return commandPool_; }
	VkDescriptorPool GetDescriptorPool() const { return descriptorPool_; }
	VkCommandBuffer GetCurrentCommandBuffer() const { return commandBuffers_[currentFrame_]; }
	size_t GetSwapChainImageCount() const { return swapChainImages_.size(); }
	VkPipeline GetGraphicsPipeline() const { return graphicsPipeline_; }
	const glm::mat4& GetViewMatrix() const { return currentView_; }
	const glm::mat4& GetProjectionMatrix() const { return currentProj_; }
	VkPipeline GetGraphicsPipelineArray() const { return graphicsPipelineArray_; }

	static void SetPreferIntegratedGPU(bool v) { preferIntegratedGPU_ = v; }
	VkPipelineLayout GetPipelineLayout() const { return pipelineLayout_; }
	VkDescriptorSetLayout GetDescriptorSetLayout() const { return descriptorSetLayout_; }
	uint32_t GetCurrentFrame() const { return currentFrame_; }
	uint32_t GetDrawCallCount() const { return drawCallCount_; }

	void SetModelMatrix(const glm::mat4& m) { currentModel_ = m; updateUniformBuffer(currentFrame_); }
	void SetViewMatrix(const glm::mat4& v) { currentView_ = v; }
	void SetProjectionMatrix(const glm::mat4& p) { currentProj_ = p; }

	void SetMat4(const char* name, const glm::mat4& value) override {
		if (strcmp(name, "model") == 0) { currentModel_ = value; }
		else if (strcmp(name, "view") == 0) { currentView_ = value; updateUniformBuffer(currentFrame_); }
		else if (strcmp(name, "projection") == 0) { currentProj_ = value; updateUniformBuffer(currentFrame_); }
	}

	void SetVec3(const char* name, const glm::vec3& value) override {
		if (strcmp(name, "objectColor") == 0) { currentObjectColor_ = glm::vec4(value, 1.0f); }
	}

	void SetFrontFace(bool ccw) override {
		reverseWinding_ = !ccw;
	}

	void SetTransparentMode(bool enabled) override {
		transparentMode_ = enabled;
	}

	bool ReadDepthBuffer(std::vector<float>& outDepths, int& outW, int& outH) override {
		if (!depthReadbackRequested_) {
			depthReadbackRequested_ = true;
		}
		if (!depthReadbackAvailable_) {
			return false;
		}
		outDepths = cachedDepthData_;
		outW = cachedDepthW_;
		outH = cachedDepthH_;
		return true;
	}

	void* UploadToImGui(const unsigned char* rgba, int w, int h) override {
		if (w <= 0 || h <= 0) return nullptr;

		if (depthVizSampler_ == VK_NULL_HANDLE) {
			VkSamplerCreateInfo si{};
			si.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			si.magFilter = VK_FILTER_LINEAR;
			si.minFilter = VK_FILTER_LINEAR;
			si.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			si.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			si.maxAnisotropy = 1.0f;
			if (vkCreateSampler(_device, &si, nullptr, &depthVizSampler_) != VK_SUCCESS)
				return nullptr;
		}

		if (depthVizLayout_ == VK_NULL_HANDLE) {
			VkDescriptorSetLayoutBinding binding{};
			binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			binding.descriptorCount = 1;
			VkDescriptorSetLayoutCreateInfo li{};
			li.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			li.bindingCount = 1;
			li.pBindings = &binding;
			if (vkCreateDescriptorSetLayout(_device, &li, nullptr, &depthVizLayout_) != VK_SUCCESS)
				return nullptr;
		}

		if (depthVizPool_ == VK_NULL_HANDLE) {
			VkDescriptorPoolSize ps{};
			ps.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			ps.descriptorCount = 1;
			VkDescriptorPoolCreateInfo pi{};
			pi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			pi.maxSets = 1;
			pi.poolSizeCount = 1;
			pi.pPoolSizes = &ps;
			if (vkCreateDescriptorPool(_device, &pi, nullptr, &depthVizPool_) != VK_SUCCESS)
				return nullptr;
		}

		if (depthVizDescriptorSet_ == VK_NULL_HANDLE) {
			VkDescriptorSetAllocateInfo ai{};
			ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			ai.descriptorPool = depthVizPool_;
			ai.descriptorSetCount = 1;
			ai.pSetLayouts = &depthVizLayout_;
			if (vkAllocateDescriptorSets(_device, &ai, &depthVizDescriptorSet_) != VK_SUCCESS)
				return nullptr;
		}

		if (w != depthVizW_ || h != depthVizH_ || depthVizImage_ == VK_NULL_HANDLE) {
			if (depthVizImageView_ != VK_NULL_HANDLE) {
				vkDestroyImageView(_device, depthVizImageView_, nullptr);
				depthVizImageView_ = VK_NULL_HANDLE;
			}
			if (depthVizImage_ != VK_NULL_HANDLE) {
				vkDestroyImage(_device, depthVizImage_, nullptr);
				depthVizImage_ = VK_NULL_HANDLE;
			}
			if (depthVizMemory_ != VK_NULL_HANDLE) {
				vkFreeMemory(_device, depthVizMemory_, nullptr);
				depthVizMemory_ = VK_NULL_HANDLE;
			}
			createImage(w, h, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthVizImage_, depthVizMemory_);
			depthVizImageView_ = createImageView(depthVizImage_, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
			depthVizW_ = w; depthVizH_ = h;

			VkDescriptorImageInfo ii{};
			ii.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ii.imageView = depthVizImageView_;
			ii.sampler = depthVizSampler_;
			VkWriteDescriptorSet wd{};
			wd.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			wd.dstSet = depthVizDescriptorSet_;
			wd.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			wd.descriptorCount = 1;
			wd.pImageInfo = &ii;
			vkUpdateDescriptorSets(_device, 1, &wd, 0, nullptr);
		}

		VkDeviceSize imgSize = static_cast<VkDeviceSize>(w) * h * 4;
		VkBuffer stageBuf;
		VkDeviceMemory stageMem;
		createBuffer(imgSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stageBuf, stageMem);
		void* mapped;
		vkMapMemory(_device, stageMem, 0, imgSize, 0, &mapped);
		memcpy(mapped, rgba, static_cast<size_t>(imgSize));
		vkUnmapMemory(_device, stageMem);

		VkCommandBufferAllocateInfo ai{};
		ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		ai.commandPool = commandPool_;
		ai.commandBufferCount = 1;
		VkCommandBuffer cmd;
		vkAllocateCommandBuffers(_device, &ai, &cmd);

		VkCommandBufferBeginInfo bi{};
		bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(cmd, &bi);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = depthVizImage_;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.layerCount = 1;

		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		VkBufferImageCopy region{};
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.layerCount = 1;
		region.imageExtent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1};
		vkCmdCopyBufferToImage(cmd, stageBuf, depthVizImage_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		vkEndCommandBuffer(cmd);

		VkSubmitInfo si{};
		si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		si.commandBufferCount = 1;
		si.pCommandBuffers = &cmd;
		VkFence fence;
		VkFenceCreateInfo fi{};
		fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		vkCreateFence(_device, &fi, nullptr, &fence);
		vkQueueSubmit(graphicsQueue_, 1, &si, fence);
		vkWaitForFences(_device, 1, &fence, VK_TRUE, UINT64_MAX);
		vkDestroyFence(_device, fence, nullptr);
		vkFreeCommandBuffers(_device, commandPool_, 1, &cmd);

		vkDestroyBuffer(_device, stageBuf, nullptr);
		vkFreeMemory(_device, stageMem, nullptr);

		return (void*)depthVizDescriptorSet_;
	}

	uint64_t CreateFramebuffer(uint64_t nativeImage, uint32_t w, uint32_t h, uint32_t layer = 0, uint64_t depthFormat = 0, uint64_t colorFormat = 0) override {
		VkImage colorImage = colorFormat != 0
			? reinterpret_cast<VkImage>(nativeImage)
			: GetColorAttachmentImage(nativeImage);
		VkFormat fmt = colorFormat != 0 ? static_cast<VkFormat>(colorFormat) : swapChainImageFormat_;

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = colorImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = fmt;
		viewInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
							   VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = layer;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView colorImageView;
		if (vkCreateImageView(_device, &viewInfo, nullptr, &colorImageView) != VK_SUCCESS)
			throw std::runtime_error("Failed to create XR color image view.");

		VkFormat depthFmt = depthFormat != 0 ? static_cast<VkFormat>(depthFormat) : findDepthFormat();
		VkImage depthImage;
		VkDeviceMemory depthImageMemory;
		createImage(w, h, depthFmt, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
		VkImageView depthImageView = createImageView(depthImage, depthFmt, VK_IMAGE_ASPECT_DEPTH_BIT);

		VkRenderPass rp = getOrCreateXrRenderPass(fmt, depthFmt);

		std::array<VkImageView, 2> attachments = {colorImageView, depthImageView};

		VkFramebufferCreateInfo fbInfo{};
		fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbInfo.renderPass = rp;
		fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		fbInfo.pAttachments = attachments.data();
		fbInfo.width = w;
		fbInfo.height = h;
		fbInfo.layers = 1;

		VkFramebuffer framebuffer;
		if (vkCreateFramebuffer(_device, &fbInfo, nullptr, &framebuffer) != VK_SUCCESS) {
			vkDestroyImageView(_device, depthImageView, nullptr);
			vkDestroyImage(_device, depthImage, nullptr);
			vkFreeMemory(_device, depthImageMemory, nullptr);
			vkDestroyImageView(_device, colorImageView, nullptr);
			return 0;
		}

		ExternalFramebuffer* ext = new ExternalFramebuffer{
			framebuffer, colorImageView,
			depthImageView, depthImage, depthImageMemory,
			depthFmt, fmt
		};
		return reinterpret_cast<uint64_t>(ext);
	}

	void DestroyFramebuffer(uint64_t fb) override {
		if (fb == 0) return;
		ExternalFramebuffer* ext = reinterpret_cast<ExternalFramebuffer*>(fb);
		vkDestroyFramebuffer(_device, ext->framebuffer, nullptr);
		vkDestroyImageView(_device, ext->colorImageView, nullptr);
		vkDestroyImageView(_device, ext->depthImageView, nullptr);
		vkDestroyImage(_device, ext->depthImage, nullptr);
		vkFreeMemory(_device, ext->depthImageMemory, nullptr);
		delete ext;
	}

	void BeginVRFrame() override {
		vkWaitForFences(_device, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);
		vkResetFences(_device, 1, &inFlightFences_[currentFrame_]);
		vkResetCommandBuffer(commandBuffers_[currentFrame_], 0);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginCommandBuffer(commandBuffers_[currentFrame_], &beginInfo);
	}

	void BeginEyeFrame(uint64_t fb, uint32_t w, uint32_t h) override {
		ExternalFramebuffer* ext = reinterpret_cast<ExternalFramebuffer*>(fb);
		auto cmd = commandBuffers_[currentFrame_];

		VkRenderPass rp = getOrCreateXrRenderPass(ext->colorFormat != VK_FORMAT_UNDEFINED ? ext->colorFormat : swapChainImageFormat_, ext->depthFormat);

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = m_VulkanClearColor.color;
		clearValues[1].depthStencil = {1.0f, 0};

		VkRenderPassBeginInfo rpBegin{};
		rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rpBegin.renderPass = rp;
		rpBegin.framebuffer = ext->framebuffer;
		rpBegin.renderArea.offset = {0, 0};
		rpBegin.renderArea.extent = {w, h};
		rpBegin.clearValueCount = static_cast<uint32_t>(clearValues.size());
		rpBegin.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(w);
		viewport.height = static_cast<float>(h);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = {w, h};
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		drawCallCount_ = 0;
		currentBoundPipeline_ = VK_NULL_HANDLE;
	}

	void EndEyeFrame() override {
		vkCmdEndRenderPass(commandBuffers_[currentFrame_]);
	}

	void EndVRFrame() override {
		auto cmd = commandBuffers_[currentFrame_];
		vkEndCommandBuffer(cmd);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmd;

		vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]);

		currentFrame_ = (currentFrame_ + 1) % kMaxFramesInFlight;
	}

	void BeginExternalFrame(uint64_t fb, uint32_t w, uint32_t h) override {
		BeginVRFrame();
		BeginEyeFrame(fb, w, h);
	}

	void EndExternalFrame() override {
		EndEyeFrame();
		EndVRFrame();
	}

	bool IsVulkan() const override { return true; }

	uint64_t CreateColorAttachment(uint32_t w, uint32_t h) override {
		VkImage image;
		VkDeviceMemory memory;
		createImage(w, h, swapChainImageFormat_, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory);

		ColorAttachment* ca = new ColorAttachment{image, memory};
		return reinterpret_cast<uint64_t>(ca);
	}

	void DestroyColorAttachment(uint64_t handle) override {
		if (handle == 0) return;
		ColorAttachment* ca = reinterpret_cast<ColorAttachment*>(handle);
		vkDestroyImage(_device, ca->image, nullptr);
		vkFreeMemory(_device, ca->memory, nullptr);
		delete ca;
	}

	void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {
		transitionImageLayout(image, swapChainImageFormat_, oldLayout, newLayout);
	}

	uint64_t GetSwapchainFormat() const override {
		return static_cast<uint64_t>(swapChainImageFormat_);
	}

private:

	struct ColorAttachment {
		VkImage image;
		VkDeviceMemory memory;
	};

	struct ExternalFramebuffer {
		VkFramebuffer framebuffer;
		VkImageView colorImageView;
		VkImageView depthImageView;
		VkImage depthImage;
		VkDeviceMemory depthImageMemory;
		VkFormat depthFormat = VK_FORMAT_UNDEFINED;
		VkFormat colorFormat = VK_FORMAT_UNDEFINED;
	};

	VkRenderPass getOrCreateXrRenderPass(VkFormat colorFormat, VkFormat depthFormat) {
		if (xrRenderPass_ != VK_NULL_HANDLE && xrRenderPassFormat_ == colorFormat)
			return xrRenderPass_;

		if (xrRenderPass_ != VK_NULL_HANDLE) {
			vkDestroyRenderPass(_device, xrRenderPass_, nullptr);
			xrRenderPass_ = VK_NULL_HANDLE;
		}

		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = colorFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorRef{};
		colorRef.attachment = 0;
		colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = depthFormat;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthRef{};
		depthRef.attachment = 1;
		depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorRef;
		subpass.pDepthStencilAttachment = &depthRef;

		VkSubpassDependency dep{};
		dep.srcSubpass = VK_SUBPASS_EXTERNAL;
		dep.dstSubpass = 0;
		dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dep.srcAccessMask = 0;
		dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

		VkRenderPassCreateInfo rpInfo{};
		rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		rpInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		rpInfo.pAttachments = attachments.data();
		rpInfo.subpassCount = 1;
		rpInfo.pSubpasses = &subpass;
		rpInfo.dependencyCount = 1;
		rpInfo.pDependencies = &dep;

		if (vkCreateRenderPass(_device, &rpInfo, nullptr, &xrRenderPass_) != VK_SUCCESS)
			throw std::runtime_error("Failed to create XR render pass.");

		xrRenderPassFormat_ = colorFormat;
		return xrRenderPass_;
	}

	// XR/external rendering resources
	VkRenderPass xrRenderPass_ = VK_NULL_HANDLE;
	VkFormat xrRenderPassFormat_ = VK_FORMAT_UNDEFINED;
	// fe::IWindow *window;

	std::string vertShaderPath_;
	std::string fragShaderPath_;
	std::string vertShaderArrayPath_;
	std::string fragShaderArrayPath_;
	std::string vertShaderFoxcraftPath_;
	std::string _deviceName;
	static inline bool preferIntegratedGPU_ = false;

	VkInstance _instance = VK_NULL_HANDLE;
	VkSurfaceKHR _surface = VK_NULL_HANDLE;
	std::unordered_map<const IWindow*, VulkanWindowResources> windowRegistry;
	VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
	VkDevice _device = VK_NULL_HANDLE;
	VkQueue graphicsQueue_ = VK_NULL_HANDLE;
	VkQueue presentQueue_ = VK_NULL_HANDLE;
	uint32_t graphicsQueueFamily_ = 0;

	VkSwapchainKHR swapChain_ = VK_NULL_HANDLE;
	std::vector<VkImage> swapChainImages_;
	std::vector<VkImageView> swapChainImageViews_;
	std::vector<VkFramebuffer> swapChainFramebuffers_;
	VkFormat swapChainImageFormat_ = VK_FORMAT_UNDEFINED;
	VkExtent2D swapChainExtent_{};

	VkRenderPass renderPass_ = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
	VkPipeline graphicsPipeline_ = VK_NULL_HANDLE;
	VkPipeline graphicsPipelineArray_ = VK_NULL_HANDLE;
	VkPipeline graphicsPipelineFoxcraft_ = VK_NULL_HANDLE;
	VkPipeline graphicsPipelineCW_ = VK_NULL_HANDLE;
	VkPipeline graphicsPipelineArrayCW_ = VK_NULL_HANDLE;
	VkPipeline graphicsPipelineFoxcraftCW_ = VK_NULL_HANDLE;

	VkPipeline graphicsPipelineTransparent_ = VK_NULL_HANDLE;
	VkPipeline graphicsPipelineArrayTransparent_ = VK_NULL_HANDLE;
	VkPipeline graphicsPipelineFoxcraftTransparent_ = VK_NULL_HANDLE;
	VkPipeline graphicsPipelineCWTransparent_ = VK_NULL_HANDLE;
	VkPipeline graphicsPipelineArrayCWTransparent_ = VK_NULL_HANDLE;
	VkPipeline graphicsPipelineFoxcraftCWTransparent_ = VK_NULL_HANDLE;

	bool transparentMode_ = false;

	std::vector<VkBuffer> uniformBuffers_;
	std::vector<VkDeviceMemory> uniformBuffersMemory_;
	std::vector<void*> uniformBuffersMapped_;

	VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> descriptorSets_;

	VkImage defaultImage_ = VK_NULL_HANDLE;
	VkDeviceMemory defaultImageMemory_ = VK_NULL_HANDLE;
	VkImageView defaultImageView_ = VK_NULL_HANDLE;
	VkSampler defaultSampler_ = VK_NULL_HANDLE;

	VkImage depthImage_ = VK_NULL_HANDLE;
	VkDeviceMemory depthImageMemory_ = VK_NULL_HANDLE;
	VkImageView depthImageView_ = VK_NULL_HANDLE;

	VkBuffer depthStagingBuffer_ = VK_NULL_HANDLE;
	VkDeviceMemory depthStagingMemory_ = VK_NULL_HANDLE;
	bool depthReadbackRequested_ = false;
	bool depthReadbackAvailable_ = false;
	std::vector<float> cachedDepthData_;
	int cachedDepthW_ = 0, cachedDepthH_ = 0;

	VkImage depthVizImage_ = VK_NULL_HANDLE;
	VkDeviceMemory depthVizMemory_ = VK_NULL_HANDLE;
	VkImageView depthVizImageView_ = VK_NULL_HANDLE;
	VkSampler depthVizSampler_ = VK_NULL_HANDLE;
	VkDescriptorPool depthVizPool_ = VK_NULL_HANDLE;
	VkDescriptorSetLayout depthVizLayout_ = VK_NULL_HANDLE;
	VkDescriptorSet depthVizDescriptorSet_ = VK_NULL_HANDLE;
	int depthVizW_ = 0, depthVizH_ = 0;

	VkCommandPool commandPool_ = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> commandBuffers_;

	std::vector<VkSemaphore> imageAvailableSemaphores_;
	std::vector<VkSemaphore> renderFinishedSemaphores_;
	std::vector<VkFence> inFlightFences_;
	uint32_t currentFrame_ = 0;
	uint32_t currentImageIndex_ = 0;
	uint32_t drawCallCount_ = 0;

	glm::mat4 currentModel_ = glm::mat4(1.0f);
	glm::mat4 currentView_ = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0), glm::vec3(0,1,0));
	glm::mat4 currentProj_ = glm::mat4(1.0f);
	glm::vec4 currentObjectColor_ = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	bool reverseWinding_ = false;

	void CreateInstance();

	void CreateSurface(IWindow *window);

	bool checkValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : kValidationLayers) {
			bool found = false;
			for (const auto& layerProps : availableLayers) {
				if (std::strcmp(layerName, layerProps.layerName) == 0) {
					found = true;
					break;
				}
			}
			if (!found) return false;
		}
		return true;
	}

	


	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice dev) {
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}
			VkBool32 presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, _surface, &presentSupport);
			if (presentSupport) {
				indices.presentFamily = i;
			}
			if (indices.isComplete()) break;
		}

		return indices;
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice dev) {
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(kDeviceExtensions.begin(), kDeviceExtensions.end());
		for (const auto& ext : availableExtensions) {
			requiredExtensions.erase(ext.extensionName);
		}
		return requiredExtensions.empty();
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice dev) {
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, _surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(dev, _surface, &formatCount, nullptr);
		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(dev, _surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(dev, _surface, &presentModeCount, nullptr);
		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(dev, _surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	int rateDeviceSuitability(VkPhysicalDevice dev) {
		QueueFamilyIndices indices = findQueueFamilies(dev);
		if (!indices.isComplete()) return -1;
		if (!checkDeviceExtensionSupport(dev)) return -1;

		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(dev);
		if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) return -1;

		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(dev, &features);
		if (!features.samplerAnisotropy) return -1;

		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(dev, &props);

		int score = 0;
		if (preferIntegratedGPU_) {
			if (props.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
				return -1;
			score += 1000000;
		} else {
			if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 1000;
			else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) score += 100;
		}
		score += static_cast<int>(props.limits.maxImageDimension2D);
		return score;
	}



	void PickPhysicalDevice();

	void createLogicalDevice() {
		QueueFamilyIndices indices = findQueueFamilies(_physicalDevice);

		std::set<uint32_t> uniqueQueueFamilies = {
			indices.graphicsFamily.value(),
			indices.presentFamily.value()
		};

		float queuePriority = 1.0f;
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(kDeviceExtensions.size());
		createInfo.ppEnabledExtensionNames = kDeviceExtensions.data();

		if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create logical device.");
		}

		vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &graphicsQueue_);
		vkGetDeviceQueue(_device, indices.presentFamily.value(), 0, &presentQueue_);
		graphicsQueueFamily_ = indices.graphicsFamily.value();
	}

	void createSwapChain() {
		SwapChainSupportDetails support = querySwapChainSupport(_physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(support.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(support.presentModes);
		VkExtent2D extent = chooseSwapExtent(support.capabilities);

		uint32_t imageCount = support.capabilities.minImageCount + 1;
		if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount) {
			imageCount = support.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = _surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = findQueueFamilies(_physicalDevice);
		uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		} else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		createInfo.preTransform = support.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		if (vkCreateSwapchainKHR(_device, &createInfo, nullptr, &swapChain_) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create swap chain.");
		}

		vkGetSwapchainImagesKHR(_device, swapChain_, &imageCount, nullptr);
		swapChainImages_.resize(imageCount);
		vkGetSwapchainImagesKHR(_device, swapChain_, &imageCount, swapChainImages_.data());

		swapChainImageFormat_ = surfaceFormat.format;
		swapChainExtent_ = extent;
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
		for (const auto& fmt : formats) {
			if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB &&
				fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return fmt;
			}
		}
		return formats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes) {
		for (const auto& mode : modes) {
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR) return mode;
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != UINT32_MAX) {
			return capabilities.currentExtent;
		}
		// int width, height;
		// glfwGetFramebufferSize(window_, &width, &height);
		// auto fbs = window->GetFramebufferSize();
		// VkExtent2D actualExtent = {static_cast<uint32_t>(fbs.width), static_cast<uint32_t>(fbs.height)};
		// actualExtent.width = std::clamp(actualExtent.width,
		// 	capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		// actualExtent.height = std::clamp(actualExtent.height,
		// 	capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		// return actualExtent;
	}

	void createImageViews() {
		swapChainImageViews_.resize(swapChainImages_.size());
		for (size_t i = 0; i < swapChainImages_.size(); ++i) {
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages_[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat_;
			createInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
									VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(_device, &createInfo, nullptr, &swapChainImageViews_[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create image view.");
			}
		}
	}

	void createRenderPass() {
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainImageFormat_;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = findDepthFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(_device, &renderPassInfo, nullptr, &renderPass_) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create render pass.");
		}
	}

	VkFormat findSupportedDepthFormat() {
		std::vector<VkFormat> candidates = {
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT
		};
		for (VkFormat format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(_physicalDevice, format, &props);
			if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
				return format;
			}
		}
		throw std::runtime_error("Failed to find supported depth format.");
	}

	VkFormat findDepthFormat() {
		return findSupportedDepthFormat();
	}

	// ---------------------------------------------------------------
	// Descriptor set layout (UBO binding for the vertex shader)
	// ---------------------------------------------------------------
	void createDescriptorSetLayout() {
		VkDescriptorSetLayoutBinding uboBinding{};
		uboBinding.binding = 0;
		uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboBinding.descriptorCount = 1;
		uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding samplerBinding{};
		samplerBinding.binding = 1;
		samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerBinding.descriptorCount = 1;
		samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboBinding, samplerBinding};

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &descriptorSetLayout_) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor set layout.");
		}
	}

	// ---------------------------------------------------------------
	// Shader helpers
	// ---------------------------------------------------------------
	static std::vector<char> readFile(const std::string& path) {
		std::ifstream file(path, std::ios::ate | std::ios::binary);
		if (!file.is_open()) {
			std::cerr << "[VulkanDevice] Failed to open shader file: " << path << std::endl;
			return {};
		}
		size_t fileSize = static_cast<size_t>(file.tellg());
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
		return buffer;
	}

	VkShaderModule createShaderModule(const std::vector<char>& code) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create shader module.");
		}
		return shaderModule;
	}

	// ---------------------------------------------------------------
	// Graphics pipeline (vertex input, dynamic viewport, depth test)
	// ---------------------------------------------------------------
	void createGraphicsPipeline(const std::string& vertPath, const std::string& fragPath,
								VertexFormat format, VkPipeline& outPipeline,
								VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
								VkBool32 depthWriteEnable = VK_TRUE) {
		auto vertShaderCode = readFile(vertPath);
		auto fragShaderCode = readFile(fragPath);

		if (vertShaderCode.empty() || fragShaderCode.empty()) {
			std::cerr << "[VulkanDevice] Skipping pipeline creation — missing shader: " << vertPath << " or " << fragPath << std::endl;
			outPipeline = VK_NULL_HANDLE;
			return;
		}

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertStageInfo{};
		vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertStageInfo.module = vertShaderModule;
		vertStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragStageInfo{};
		fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragStageInfo.module = fragShaderModule;
		fragStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};

		VkVertexInputBindingDescription bindingDesc{};
		std::vector<VkVertexInputAttributeDescription> attributeDescs;

		if (format == VertexFormat::Array) {
			bindingDesc.binding = 0;
			bindingDesc.stride = sizeof(fe::VertexArray);
			bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			attributeDescs.resize(3);
			attributeDescs[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(fe::VertexArray, position)};
			attributeDescs[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(fe::VertexArray, normal)};
			attributeDescs[2] = {2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(fe::VertexArray, texCoord)};
		} else if (format == VertexFormat::Foxcraft) {
			bindingDesc.binding = 0;
			bindingDesc.stride = 7;
			bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			attributeDescs.resize(2);
			attributeDescs[0] = {0, 0, VK_FORMAT_R16G16B16_SINT, 0};
			attributeDescs[1] = {1, 0, VK_FORMAT_R8_UINT, 6};
		} else {
			bindingDesc = fe::Vertex::getBindingDescription();
			auto stdAttrs = fe::Vertex::getAttributeDescriptions();
			attributeDescs.assign(stdAttrs.begin(), stdAttrs.end());
		}

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescs.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescs.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = frontFace;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = depthWriteEnable;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(glm::mat4) + sizeof(glm::vec4);

		if (pipelineLayout_ == VK_NULL_HANDLE) {
			VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = 1;
			pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout_;
			pipelineLayoutInfo.pushConstantRangeCount = 1;
			pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

			if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create pipeline layout.");
			}
		}

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = pipelineLayout_;
		pipelineInfo.renderPass = renderPass_;
		pipelineInfo.subpass = 0;

		if (vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
									&outPipeline) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create graphics pipeline.");
		}

		vkDestroyShaderModule(_device, fragShaderModule, nullptr);
		vkDestroyShaderModule(_device, vertShaderModule, nullptr);
	}

	// ---------------------------------------------------------------
	// Depth resources
	// ---------------------------------------------------------------
	void createDepthResources() {
		VkFormat depthFormat = findDepthFormat();
		createImage(swapChainExtent_.width, swapChainExtent_.height, depthFormat,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage_, depthImageMemory_);
		depthImageView_ = createImageView(depthImage_, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	}

	void createImage(uint32_t width, uint32_t height, VkFormat format,
					VkImageTiling tiling, VkImageUsageFlags usage,
					VkMemoryPropertyFlags properties,
					VkImage& image, VkDeviceMemory& imageMemory) {
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateImage(_device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create image.");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(_device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate image memory.");
		}
		vkBindImageMemory(_device, image, imageMemory, 0);
	}

	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		if (vkCreateImageView(_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create image view.");
		}
		return imageView;
	}

	// ---------------------------------------------------------------
	// Framebuffers (with depth attachment)
	// ---------------------------------------------------------------
	void createFramebuffers() {
		swapChainFramebuffers_.resize(swapChainImageViews_.size());
		for (size_t i = 0; i < swapChainImageViews_.size(); ++i) {
			std::array<VkImageView, 2> attachments = {swapChainImageViews_[i], depthImageView_};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass_;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent_.width;
			framebufferInfo.height = swapChainExtent_.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(_device, &framebufferInfo, nullptr,
									&swapChainFramebuffers_[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create framebuffer.");
			}
		}
	}

	// ---------------------------------------------------------------
	// Command pool + buffers
	// ---------------------------------------------------------------
	void createCommandPool() {
		QueueFamilyIndices indices = findQueueFamilies(_physicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = indices.graphicsFamily.value();

		if (vkCreateCommandPool(_device, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create command pool.");
		}
	}

	void createCommandBuffers() {
		commandBuffers_.resize(kMaxFramesInFlight);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool_;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());

		if (vkAllocateCommandBuffers(_device, &allocInfo, commandBuffers_.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate command buffers.");
		}
	}

	// ---------------------------------------------------------------
	// Buffers (vertex, index, uniform)
	// ---------------------------------------------------------------
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) &&
				(memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}
		throw std::runtime_error("Failed to find suitable memory type.");
	}

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
					VkMemoryPropertyFlags properties,
					VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create buffer.");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(_device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate buffer memory.");
		}
		vkBindBufferMemory(_device, buffer, bufferMemory, 0);
	}

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool_;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer);

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

		vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue_);

		vkFreeCommandBuffers(_device, commandPool_, 1, &commandBuffer);
	}

	template <typename T>
	void uploadToGPUBuffer(const std::vector<T>& data, VkBufferUsageFlags usage, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
		VkDeviceSize bufferSize = sizeof(T) * data.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingMemory);

		void* mappedData;
		vkMapMemory(_device, stagingMemory, 0, bufferSize, 0, &mappedData);
		memcpy(mappedData, data.data(), static_cast<size_t>(bufferSize));
		vkUnmapMemory(_device, stagingMemory);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, bufferMemory);

		copyBuffer(stagingBuffer, buffer, bufferSize);

		vkDestroyBuffer(_device, stagingBuffer, nullptr);
		vkFreeMemory(_device, stagingMemory, nullptr);
	}

	// ---------------------------------------------------------------
	// Uniform buffers
	// ---------------------------------------------------------------
	void createUniformBuffers() {
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);
		uniformBuffers_.resize(kMaxFramesInFlight);
		uniformBuffersMemory_.resize(kMaxFramesInFlight);
		uniformBuffersMapped_.resize(kMaxFramesInFlight);

		for (size_t i = 0; i < kMaxFramesInFlight; i++) {
			createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				uniformBuffers_[i], uniformBuffersMemory_[i]);
			vkMapMemory(_device, uniformBuffersMemory_[i], 0, bufferSize, 0, &uniformBuffersMapped_[i]);
		}
	}

	// ---------------------------------------------------------------
	// Default white texture (1x1) for meshes without textures
	// ---------------------------------------------------------------
	void createDefaultTexture() {
		uint32_t white = 0xFFFFFFFF;
		VkDeviceSize imageSize = sizeof(uint32_t);

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;
		createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingMemory);
		void* data;
		vkMapMemory(_device, stagingMemory, 0, imageSize, 0, &data);
		memcpy(data, &white, sizeof(uint32_t));
		vkUnmapMemory(_device, stagingMemory);

		createImage(1, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, defaultImage_, defaultImageMemory_);

		transitionImageLayout(defaultImage_, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyBufferToImage(stagingBuffer, defaultImage_, 1, 1);
		transitionImageLayout(defaultImage_, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(_device, stagingBuffer, nullptr);
		vkFreeMemory(_device, stagingMemory, nullptr);

		defaultImageView_ = createImageView(defaultImage_, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 16;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		if (vkCreateSampler(_device, &samplerInfo, nullptr, &defaultSampler_) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create default sampler.");
		}
	}

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool_;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		} else {
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		}

		vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0,
			0, nullptr, 0, nullptr, 1, &barrier);

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue_);

		vkFreeCommandBuffers(_device, commandPool_, 1, &commandBuffer);
	}

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool_;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = {0, 0, 0};
		region.imageExtent = {width, height, 1};

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue_);

		vkFreeCommandBuffers(_device, commandPool_, 1, &commandBuffer);
	}

	// ---------------------------------------------------------------
	// Descriptors
	// ---------------------------------------------------------------
	void createDescriptorPool() {
		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(kMaxDrawsPerFrame * kMaxFramesInFlight + kMaxFramesInFlight + kMaxCachedTextures);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(kMaxDrawsPerFrame * kMaxFramesInFlight + kMaxFramesInFlight + kMaxCachedTextures);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(kMaxDrawsPerFrame * kMaxFramesInFlight + kMaxFramesInFlight + kMaxCachedTextures);

		if (vkCreateDescriptorPool(_device, &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor pool.");
		}
	}

	void createDescriptorSets() {
		int totalSets = kMaxDrawsPerFrame * kMaxFramesInFlight;
		std::vector<VkDescriptorSetLayout> layouts(totalSets, descriptorSetLayout_);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool_;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(totalSets);
		allocInfo.pSetLayouts = layouts.data();

		descriptorSets_.resize(totalSets);
		if (vkAllocateDescriptorSets(_device, &allocInfo, descriptorSets_.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate descriptor sets.");
		}

		for (int i = 0; i < totalSets; i++) {
			int frameIndex = i / kMaxDrawsPerFrame;

			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers_[frameIndex];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = defaultImageView_;
			imageInfo.sampler = defaultSampler_;

			std::array<VkWriteDescriptorSet, 2> writes{};

			writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[0].dstSet = descriptorSets_[i];
			writes[0].dstBinding = 0;
			writes[0].dstArrayElement = 0;
			writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writes[0].descriptorCount = 1;
			writes[0].pBufferInfo = &bufferInfo;

			writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[1].dstSet = descriptorSets_[i];
			writes[1].dstBinding = 1;
			writes[1].dstArrayElement = 0;
			writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[1].descriptorCount = 1;
			writes[1].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
		}
	}

	// ---------------------------------------------------------------
	// Sync objects
	// ---------------------------------------------------------------
	void createSyncObjects() {
		size_t imageCount = swapChainImages_.size();
		imageAvailableSemaphores_.resize(kMaxFramesInFlight);
		renderFinishedSemaphores_.resize(imageCount);
		inFlightFences_.resize(kMaxFramesInFlight);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < kMaxFramesInFlight; i++) {
			if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]) != VK_SUCCESS)
				throw std::runtime_error("Failed to create sync objects.");
		}

		for (size_t i = 0; i < imageCount; i++) {
			if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]) != VK_SUCCESS)
				throw std::runtime_error("Failed to create sync objects.");
		}

		for (size_t i = 0; i < kMaxFramesInFlight; i++) {
			if (vkCreateFence(_device, &fenceInfo, nullptr, &inFlightFences_[i]) != VK_SUCCESS)
				throw std::runtime_error("Failed to create sync objects.");
		}
	}

	

	void updateUniformBuffer(uint32_t currentImage) {
		UniformBufferObject ubo{};
		ubo.view = currentView_;
		ubo.proj = currentProj_;
		ubo.proj[1][1] *= -1;

		ubo.proj[2][2] = ubo.proj[2][2] * 0.5f - 0.5f;
		ubo.proj[3][2] *= 0.5f;

		memcpy(uniformBuffersMapped_[currentImage], &ubo, sizeof(ubo));
	}

	// ---------------------------------------------------------------
	// Swapchain recreation (for resize)
	// ---------------------------------------------------------------
	void cleanupSwapChain() {
		for (auto framebuffer : swapChainFramebuffers_)
			vkDestroyFramebuffer(_device, framebuffer, nullptr);
		swapChainFramebuffers_.clear();

		for (auto imageView : swapChainImageViews_)
			vkDestroyImageView(_device, imageView, nullptr);
		swapChainImageViews_.clear();

		if (depthImageView_ != VK_NULL_HANDLE) { vkDestroyImageView(_device, depthImageView_, nullptr); depthImageView_ = VK_NULL_HANDLE; }
		if (depthImage_ != VK_NULL_HANDLE) { vkDestroyImage(_device, depthImage_, nullptr); depthImage_ = VK_NULL_HANDLE; }
		if (depthImageMemory_ != VK_NULL_HANDLE) { vkFreeMemory(_device, depthImageMemory_, nullptr); depthImageMemory_ = VK_NULL_HANDLE; }

		if (depthStagingBuffer_ != VK_NULL_HANDLE) { vkDestroyBuffer(_device, depthStagingBuffer_, nullptr); depthStagingBuffer_ = VK_NULL_HANDLE; }
		if (depthStagingMemory_ != VK_NULL_HANDLE) { vkFreeMemory(_device, depthStagingMemory_, nullptr); depthStagingMemory_ = VK_NULL_HANDLE; }
		depthReadbackAvailable_ = false;
		cachedDepthData_.clear();

		if (swapChain_ != VK_NULL_HANDLE) { vkDestroySwapchainKHR(_device, swapChain_, nullptr); swapChain_ = VK_NULL_HANDLE; }
	}

	void recreateSwapChain() {
		vkDeviceWaitIdle(_device);

		cleanupSwapChain();

		for (auto& sem : imageAvailableSemaphores_) vkDestroySemaphore(_device, sem, nullptr);
		imageAvailableSemaphores_.clear();
		for (auto& sem : renderFinishedSemaphores_) vkDestroySemaphore(_device, sem, nullptr);
		renderFinishedSemaphores_.clear();
		for (auto& fence : inFlightFences_) vkDestroyFence(_device, fence, nullptr);
		inFlightFences_.clear();

		createSwapChain();
		createImageViews();
		createDepthResources();
		createFramebuffers();
		createSyncObjects();
	}

	// ---------------------------------------------------------------
	// Cleanup
	// ---------------------------------------------------------------
	void cleanup() {
		if (xrRenderPass_ != VK_NULL_HANDLE) {
			vkDestroyRenderPass(_device, xrRenderPass_, nullptr);
			xrRenderPass_ = VK_NULL_HANDLE;
		}

		for (auto& sem : imageAvailableSemaphores_)
			if (sem != VK_NULL_HANDLE) vkDestroySemaphore(_device, sem, nullptr);
		imageAvailableSemaphores_.clear();
		for (auto& sem : renderFinishedSemaphores_)
			if (sem != VK_NULL_HANDLE) vkDestroySemaphore(_device, sem, nullptr);
		renderFinishedSemaphores_.clear();

		for (size_t i = 0; i < kMaxFramesInFlight; i++) {
			if (inFlightFences_[i] != VK_NULL_HANDLE)
				vkDestroyFence(_device, inFlightFences_[i], nullptr);
		}

		if (commandPool_ != VK_NULL_HANDLE)
			vkDestroyCommandPool(_device, commandPool_, nullptr);

		for (auto framebuffer : swapChainFramebuffers_)
			vkDestroyFramebuffer(_device, framebuffer, nullptr);

		if (graphicsPipeline_ != VK_NULL_HANDLE)
			vkDestroyPipeline(_device, graphicsPipeline_, nullptr);
		if (graphicsPipelineArray_ != VK_NULL_HANDLE)
			vkDestroyPipeline(_device, graphicsPipelineArray_, nullptr);
		if (graphicsPipelineFoxcraft_ != VK_NULL_HANDLE)
			vkDestroyPipeline(_device, graphicsPipelineFoxcraft_, nullptr);
		if (graphicsPipelineCW_ != VK_NULL_HANDLE)
			vkDestroyPipeline(_device, graphicsPipelineCW_, nullptr);
		if (graphicsPipelineArrayCW_ != VK_NULL_HANDLE)
			vkDestroyPipeline(_device, graphicsPipelineArrayCW_, nullptr);
		if (graphicsPipelineFoxcraftCW_ != VK_NULL_HANDLE)
			vkDestroyPipeline(_device, graphicsPipelineFoxcraftCW_, nullptr);
		if (graphicsPipelineTransparent_ != VK_NULL_HANDLE)
			vkDestroyPipeline(_device, graphicsPipelineTransparent_, nullptr);
		if (graphicsPipelineArrayTransparent_ != VK_NULL_HANDLE)
			vkDestroyPipeline(_device, graphicsPipelineArrayTransparent_, nullptr);
		if (graphicsPipelineFoxcraftTransparent_ != VK_NULL_HANDLE)
			vkDestroyPipeline(_device, graphicsPipelineFoxcraftTransparent_, nullptr);
		if (graphicsPipelineCWTransparent_ != VK_NULL_HANDLE)
			vkDestroyPipeline(_device, graphicsPipelineCWTransparent_, nullptr);
		if (graphicsPipelineArrayCWTransparent_ != VK_NULL_HANDLE)
			vkDestroyPipeline(_device, graphicsPipelineArrayCWTransparent_, nullptr);
		if (graphicsPipelineFoxcraftCWTransparent_ != VK_NULL_HANDLE)
			vkDestroyPipeline(_device, graphicsPipelineFoxcraftCWTransparent_, nullptr);
		if (pipelineLayout_ != VK_NULL_HANDLE)
			vkDestroyPipelineLayout(_device, pipelineLayout_, nullptr);
		if (renderPass_ != VK_NULL_HANDLE)
			vkDestroyRenderPass(_device, renderPass_, nullptr);

		for (auto imageView : swapChainImageViews_)
			vkDestroyImageView(_device, imageView, nullptr);
		if (swapChain_ != VK_NULL_HANDLE)
			vkDestroySwapchainKHR(_device, swapChain_, nullptr);

		for (size_t i = 0; i < kMaxFramesInFlight; i++) {
			if (uniformBuffers_[i] != VK_NULL_HANDLE)
				vkDestroyBuffer(_device, uniformBuffers_[i], nullptr);
			if (uniformBuffersMemory_[i] != VK_NULL_HANDLE)
				vkFreeMemory(_device, uniformBuffersMemory_[i], nullptr);
		}

		if (descriptorPool_ != VK_NULL_HANDLE)
			vkDestroyDescriptorPool(_device, descriptorPool_, nullptr);
		if (descriptorSetLayout_ != VK_NULL_HANDLE)
			vkDestroyDescriptorSetLayout(_device, descriptorSetLayout_, nullptr);

		if (defaultSampler_ != VK_NULL_HANDLE) vkDestroySampler(_device, defaultSampler_, nullptr);
		if (defaultImageView_ != VK_NULL_HANDLE) vkDestroyImageView(_device, defaultImageView_, nullptr);
		if (defaultImage_ != VK_NULL_HANDLE) vkDestroyImage(_device, defaultImage_, nullptr);
		if (defaultImageMemory_ != VK_NULL_HANDLE) vkFreeMemory(_device, defaultImageMemory_, nullptr);

		if (depthImageView_ != VK_NULL_HANDLE)
			vkDestroyImageView(_device, depthImageView_, nullptr);
		if (depthImage_ != VK_NULL_HANDLE)
			vkDestroyImage(_device, depthImage_, nullptr);
		if (depthImageMemory_ != VK_NULL_HANDLE)
			vkFreeMemory(_device, depthImageMemory_, nullptr);

		if (depthStagingBuffer_ != VK_NULL_HANDLE) { vkDestroyBuffer(_device, depthStagingBuffer_, nullptr); depthStagingBuffer_ = VK_NULL_HANDLE; }
		if (depthStagingMemory_ != VK_NULL_HANDLE) { vkFreeMemory(_device, depthStagingMemory_, nullptr); depthStagingMemory_ = VK_NULL_HANDLE; }

		if (depthVizImageView_ != VK_NULL_HANDLE) vkDestroyImageView(_device, depthVizImageView_, nullptr);
		if (depthVizImage_ != VK_NULL_HANDLE) vkDestroyImage(_device, depthVizImage_, nullptr);
		if (depthVizMemory_ != VK_NULL_HANDLE) vkFreeMemory(_device, depthVizMemory_, nullptr);
		if (depthVizSampler_ != VK_NULL_HANDLE) vkDestroySampler(_device, depthVizSampler_, nullptr);
		if (depthVizPool_ != VK_NULL_HANDLE) vkDestroyDescriptorPool(_device, depthVizPool_, nullptr);
		if (depthVizLayout_ != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(_device, depthVizLayout_, nullptr);

		if (_device != VK_NULL_HANDLE) vkDestroyDevice(_device, nullptr);
		if (_surface != VK_NULL_HANDLE) vkDestroySurfaceKHR(_instance, _surface, nullptr);
		if (_instance != VK_NULL_HANDLE) vkDestroyInstance(_instance, nullptr);
	}
};

}