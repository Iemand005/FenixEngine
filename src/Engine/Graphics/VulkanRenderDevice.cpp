
#include "VulkanDevice.hpp"

using namespace fe;

void VulkanDevice::CreateInstance() {
	if (kEnableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error(
			"Validation layers requested but not available. "
			"Did you install vulkan-validationlayers(-dev)?");
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "FenixEngine";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "FenixEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

	fe::VulkanExtensions vkExts = window->GetVulkanExtensions();
	std::vector<const char*> extensions(vkExts.extensions, vkExts.extensions + vkExts.extensionCount);
	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (kEnableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
		createInfo.ppEnabledLayerNames = kValidationLayers.data();
	} else createInfo.enabledLayerCount = 0;

	if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS)
		throw std::runtime_error("Failed to create Vulkan instance.");
}

// void VkPhysicalDeviceUnifiedImageLayoutsFeaturesKHR
void VulkanDevice::CreateSurface(const IWindow *window) {
	_surface = (VkSurfaceKHR)window->CreateVulkanSurface(_instance);
}

void VulkanDevice::PickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
	if (deviceCount == 0)
		throw std::runtime_error("No GPUs with Vulkan support found.");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

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

	if (bestDevice == VK_NULL_HANDLE || bestScore < 0)
		throw std::runtime_error("No suitable GPU found.");

	_physicalDevice = bestDevice;
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(_physicalDevice, &props);
	_deviceName = props.deviceName;
	std::cout << "Selected GPU: " << _deviceName << "\n";
}