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
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> imageViews;
	std::vector<VkFramebuffer> framebuffers;
	VkFormat imageFormat = VK_FORMAT_UNDEFINED;
	VkExtent2D extent{};

	VkImage depthImage = VK_NULL_HANDLE;
	VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
	VkImageView depthImageView = VK_NULL_HANDLE;

	VkBuffer depthStagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory depthStagingMemory = VK_NULL_HANDLE;

	std::vector<VkCommandBuffer> commandBuffers;

	uint32_t currentImageIndex = 0;
	bool renderPassActive = false;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
};

struct UniformBufferObject {
	glm::mat4 view;
	glm::mat4 proj;
};

// The four pipeline variants a single vertex format needs: normal winding,
// reversed (clockwise) winding, alpha-blended and blend+reversed-winding.
struct PipelineSet {
	VkPipeline normal = VK_NULL_HANDLE;
	VkPipeline cw = VK_NULL_HANDLE;
	VkPipeline transparent = VK_NULL_HANDLE;
	VkPipeline cwTransparent = VK_NULL_HANDLE;
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

	void SetShaderPaths(VertexFormat format, const std::string& vertPath, const std::string& fragPath);

	void Init(IWindow *window) override;

	void SubmitFrame() override;

	void SubmitFrame(const IWindow *window) override;

	void RegisterWindow(IWindow* window) override;

	void UnregisterWindow(const IWindow* window) override;

	void Clear() override;

	void Clear(const IWindow *window);

	void SetClearColor(float r, float g, float b, float a = 1) override;

	void Resize(int width, int height) override;

	VkDescriptorSet GetOrCreateTextureDescriptorSet(VkImageView imageView, VkSampler sampler);

	void createFrameDescriptorSets();

	void BeginFrame() override;

	void DrawMesh(const IGPUBuffers* buffers, const fe::IGPUTexture* texture = nullptr) override;

	void DrawIndirect(VkBuffer vertexBuffer, VkBuffer indexBuffer,
		VkBuffer indirectBuffer, VkDeviceSize indirectOffset,
		uint32_t drawCount, uint32_t stride,
		VkDescriptorSet descriptorSet);

	void UpdateIndirectDescriptorSet(VkDescriptorSet set, VkImageView textureView, VkSampler sampler);

	std::unique_ptr<IGPUBuffers> CreateGPUBuffers() override;

	std::unique_ptr<fe::IGPUTexture> CreateGPUTexture() override;

	void UploadBuffers(IGPUBuffers* buffers,
		const void* vertices, size_t vertexStride, size_t vertexCount,
		const uint32_t* indices, uint32_t indexCount,
		const std::vector<fe::VertexAttribute>& layout = {}) override;

	void UploadTexture(fe::IGPUTexture* texture,
		const std::string& path, fe::TextureScaling scaling = fe::TextureScaling::Linear) override;

	void UploadTexture(fe::IGPUTexture* texture,
		const fe::ImageData& image, fe::TextureScaling scaling = fe::TextureScaling::Linear) override;

	void UploadTextureArray(fe::IGPUTexture* texture,
		const std::vector<std::string>& paths, fe::TextureScaling scaling = fe::TextureScaling::Linear) override;

	VkInstance GetInstance() const;

	VkPhysicalDevice GetPhysicalDevice() const;

	VkDevice GetDevice() const;

	VkQueue GetGraphicsQueue() const;

	const char* GetDeviceName() const override;

	size_t GetWindowCount() const override;

	uint32_t GetGraphicsQueueFamily() const;

	VkRenderPass GetRenderPass() const;

	VkImage GetColorAttachmentImage(uint64_t handle) const;

	VkCommandPool GetCommandPool() const;

	VkDescriptorPool GetDescriptorPool() const;

	VkCommandBuffer GetCurrentCommandBuffer() const;

	size_t GetSwapChainImageCount() const;

	VkPipeline GetGraphicsPipeline(VertexFormat format = VertexFormat::Standard, bool transparent = false, bool cw = false) const;

	const glm::mat4& GetViewMatrix() const;

	const glm::mat4& GetProjectionMatrix() const;

	static void SetPreferIntegratedGPU(bool v);

	VkPipelineLayout GetPipelineLayout() const;

	VkDescriptorSetLayout GetDescriptorSetLayout() const;

	uint32_t GetCurrentFrame() const;

	uint32_t GetDrawCallCount() const;

	void SetModelMatrix(const glm::mat4& m);

	void SetViewMatrix(const glm::mat4& v);

	void SetProjectionMatrix(const glm::mat4& p);

	void SetMat4(const char* name, const glm::mat4& value) override;

	void SetVec3(const char* name, const glm::vec3& value) override;

	void SetFrontFace(bool ccw) override;

	void SetTransparentMode(bool enabled) override;

	void SetVSync(bool enabled) override;

	bool IsVSyncEnabled() const override;

	bool ReadDepthBuffer(std::vector<float>& outDepths, int& outW, int& outH) override;

	void* UploadToImGui(const unsigned char* rgba, int w, int h) override;

	uint64_t CreateFramebuffer(uint64_t nativeImage, uint32_t w, uint32_t h, uint32_t layer = 0, uint64_t depthFormat = 0, uint64_t colorFormat = 0) override;

	void DestroyFramebuffer(uint64_t fb) override;

	void BeginVRFrame() override;

	void BeginEyeFrame(uint64_t fb, uint32_t w, uint32_t h) override;

	void EndEyeFrame() override;

	void EndVRFrame() override;

	void BeginExternalFrame(uint64_t fb, uint32_t w, uint32_t h) override;

	void EndExternalFrame() override;

	bool IsVulkan() const override;

	void SetActiveWindow(IWindow* window) override;

	uint64_t CreateColorAttachment(uint32_t w, uint32_t h) override;

	void DestroyColorAttachment(uint64_t handle) override;

	void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

	uint64_t GetSwapchainFormat() const override;

private:
	void setupWindowResources(IWindow* window);

	void cleanupWindowResources(VulkanWindowResources& res);

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

	VkRenderPass getOrCreateXrRenderPass(VkFormat colorFormat, VkFormat depthFormat);

	// XR/external rendering resources
	VkRenderPass xrRenderPass_ = VK_NULL_HANDLE;
	VkFormat xrRenderPassFormat_ = VK_FORMAT_UNDEFINED;

	std::string vertShaderPath_;
	std::string fragShaderPath_;
	std::string vertShaderArrayPath_;
	std::string fragShaderArrayPath_;
	std::string vertShaderPackedPath_;
	std::string fragShaderPackedPath_;
	std::string _deviceName;
	static inline bool preferIntegratedGPU_ = false;

	VkInstance _instance = VK_NULL_HANDLE;
	std::vector<IWindow*> registeredWindows;
	std::unordered_map<const IWindow*, VulkanWindowResources> windowRegistry;
	const IWindow* currentWindow_ = nullptr;
	VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
	VkDevice _device = VK_NULL_HANDLE;
	VkQueue graphicsQueue_ = VK_NULL_HANDLE;
	VkQueue presentQueue_ = VK_NULL_HANDLE;
	uint32_t graphicsQueueFamily_ = 0;

	VkFormat swapChainImageFormat_ = VK_FORMAT_UNDEFINED;

	VkRenderPass renderPass_ = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
	std::array<PipelineSet, 3> pipelineSets_{};

	bool transparentMode_ = false;
	bool vsyncEnabled_ = true;

	std::vector<VkBuffer> uniformBuffers_;
	std::vector<VkDeviceMemory> uniformBuffersMemory_;
	std::vector<void*> uniformBuffersMapped_;

	VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> descriptorSets_;

	VkImage defaultImage_ = VK_NULL_HANDLE;
	VkDeviceMemory defaultImageMemory_ = VK_NULL_HANDLE;
	VkImageView defaultImageView_ = VK_NULL_HANDLE;
	VkSampler defaultSampler_ = VK_NULL_HANDLE;

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

	std::vector<VkFence> inFlightFences_;
	uint32_t currentFrame_ = 0;
	uint32_t drawCallCount_ = 0;

	glm::mat4 currentModel_ = glm::mat4(1.0f);
	glm::mat4 currentView_ = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0), glm::vec3(0,1,0));
	glm::mat4 currentProj_ = glm::mat4(1.0f);
	glm::vec4 currentObjectColor_ = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	bool reverseWinding_ = false;

	void CreateInstance(IWindow *window);

	bool checkValidationLayerSupport();

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice dev, VkSurfaceKHR surface);

	bool checkDeviceExtensionSupport(VkPhysicalDevice dev);

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice dev, VkSurfaceKHR surface);

	int rateDeviceSuitability(VkPhysicalDevice dev);

	void PickPhysicalDevice();

	void createLogicalDevice();

	void createSwapChain(IWindow *window);

	std::vector<const IWindow*> GetWindows() override;

	VkSwapchainKHR createSwapChain(VkSurfaceKHR surface, VulkanWindowResources& res);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes, bool vsync);

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	void createImageViews(VulkanWindowResources& res);

	void createRenderPass();

	VkFormat findSupportedDepthFormat();

	VkFormat findDepthFormat();

	void createDescriptorSetLayout();

	static std::vector<char> readFile(const std::string& path);

	VkShaderModule createShaderModule(const std::vector<char>& code);

	void createGraphicsPipeline(const std::string& vertPath, const std::string& fragPath,
								VertexFormat format, VkPipeline& outPipeline,
								VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
								VkBool32 depthWriteEnable = VK_TRUE,
								VkBool32 enableBlend = VK_FALSE);

	void createPipelineSet(VertexFormat format);

	void createDepthResources(VulkanWindowResources& res);

	void createImage(uint32_t width, uint32_t height, VkFormat format,
					VkImageTiling tiling, VkImageUsageFlags usage,
					VkMemoryPropertyFlags properties,
					VkImage& image, VkDeviceMemory& imageMemory);

	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	void createFramebuffers(VulkanWindowResources& res);

	void createCommandPool();

	void createCommandBuffers(VulkanWindowResources& res);

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
					VkMemoryPropertyFlags properties,
					VkBuffer& buffer, VkDeviceMemory& bufferMemory);

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

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

	void createUniformBuffers();

	void createDefaultTexture();

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	void createDescriptorPool();

	void createDescriptorSets();

	void createSyncObjects();

	void createPerWindowSemaphores(VulkanWindowResources& res);

	void updateUniformBuffer(uint32_t currentImage);

	void cleanupSwapChain(VulkanWindowResources& res);

	void recreateSwapChain(const IWindow *window);

	// Shared frame plumbing (SubmitFrame / Clear single- and multi-window paths)
	void recordEndOfFrame(VulkanWindowResources& res);
	void readbackDepthStaging(VulkanWindowResources& res);
	bool acquireImage(VulkanWindowResources& res, const IWindow* window, bool waitFenceAfterRecreate);
	void beginWindowFrame(VulkanWindowResources& res);
	void submitAndPresent(const std::vector<const IWindow*>& windows);

	void cleanup();
};

}