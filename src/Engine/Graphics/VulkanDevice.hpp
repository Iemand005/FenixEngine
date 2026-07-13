#pragma once

#include <optional>


#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "RenderDevice.hpp"

#include "../window/IWindow.hpp"

#ifdef NDEBUG
constexpr bool kEnableValidationLayers = false;
#else
constexpr bool kEnableValidationLayers = true;
#endif

const std::vector<const char*> kValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> kDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

constexpr int kWindowWidth = 800;
constexpr int kWindowHeight = 600;
constexpr int kMaxFramesInFlight = 2;

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


class VulkanDevice : public RenderDevice {
public:
	void Init(fe::IWindow *window) override {
		createInstance(window);
		createSurface(window);
	}
    // VertexBuffer* CreateVertexBuffer(void* data, size_t size) override {
    //     return new VulkanVertexBuffer(data, size); // Uses vkCreateBuffer, vkBindBufferMemory
    // }

	void SubmitFrame() override {}	

private:
	VkInstance instance_ = VK_NULL_HANDLE;
	VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;

	void createInstance(fe::IWindow *window) {
        if (kEnableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error(
                "Validation layers requested but not available. "
                "Did you install vulkan-validationlayers(-dev)?");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "VkEngine";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        // uint32_t glfwExtensionCount = 0;
        // const char** glfwExtensions = window(&glfwExtensionCount);
		fe::VulkanExtensions vkExts = window->GetVulkanExtensions();
        std::vector<const char*> extensions(vkExts.extensions, vkExts.extensions + vkExts.extensionCount);

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        if (kEnableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
            createInfo.ppEnabledLayerNames = kValidationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan instance.");
        }
    }

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

	void createSurface(fe::IWindow *window) {
        // if (glfwCreateWindowSurface(instance_, window_, nullptr, &surface_) != VK_SUCCESS) {
        //     throw std::runtime_error("Failed to create window surface.");
        // }
		surface_ = (VkSurfaceKHR)window->CreateVulkanSurface(instance_);
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
            vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface_, &presentSupport);
            if (presentSupport) {
                indices.presentFamily = i;
            }
            if (indices.isComplete()) break;
        }

        return indices;
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
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 1000;
        else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) score += 100;
        score += static_cast<int>(props.limits.maxImageDimension2D);
        return score;
    }



	void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("No GPUs with Vulkan support found.");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

        int bestScore = -1;
        VkPhysicalDevice bestDevice = VK_NULL_HANDLE;

        for (const auto& dev : devices) {
            int score = rateDeviceSuitability(dev);
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(dev, &props);
            std::cout << "Found GPU: " << props.deviceName << " (score " << score << ")\n";

            if (score > bestScore) {
                bestScore = score;
                bestDevice = dev;
            }
        }

        if (bestDevice == VK_NULL_HANDLE || bestScore < 0) {
            throw std::runtime_error("No suitable GPU found.");
        }

        physicalDevice_ = bestDevice;
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(physicalDevice_, &props);
        std::cout << "Selected GPU: " << props.deviceName << "\n";
    }
};