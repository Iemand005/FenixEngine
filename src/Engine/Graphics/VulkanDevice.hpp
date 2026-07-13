#pragma once

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

class VulkanDevice : public RenderDevice {
public:
	void Init(fe::IWindow *window) override {
		createInstance(window);
	}
    // VertexBuffer* CreateVertexBuffer(void* data, size_t size) override {
    //     return new VulkanVertexBuffer(data, size); // Uses vkCreateBuffer, vkBindBufferMemory
    // }

	void SubmitFrame() override {}	

private:
	VkInstance instance_ = VK_NULL_HANDLE;

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
};