
#include "VulkanDevice.hpp"
#ifndef FE_EXCLUDE_OPENXR
#include "../vr/OpenXR.hpp"
#endif

using namespace fe;

void VulkanDevice::CreateInstance(IWindow *window) {
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
	std::vector<std::string> extensionNames;
	extensionNames.reserve(vkExts.extensionCount + 4);
	for (uint32_t i = 0; i < vkExts.extensionCount; ++i)
		extensionNames.emplace_back(vkExts.extensions[i]);
	extensionNames.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

#ifndef FE_EXCLUDE_OPENXR
	if (preferOpenXRDevice_) {
		OpenXR::VulkanRequirements requirements;
		if (OpenXR::GetVulkanRequirements(requirements)) {
			openXRRequirementsAvailable_ = true;
			const auto hasExtension = [&extensionNames](const std::string& name) {
				return std::find(extensionNames.begin(), extensionNames.end(), name) != extensionNames.end();
			};
			for (const auto& extension : requirements.instanceExtensions) {
				if (!hasExtension(extension))
					extensionNames.push_back(extension);
			}
			openXRDeviceExtensions_ = std::move(requirements.deviceExtensions);

			const auto toVkVersion = [](uint64_t version) {
				return VK_MAKE_API_VERSION(0, XR_VERSION_MAJOR(version),
					XR_VERSION_MINOR(version), XR_VERSION_PATCH(version));
			};
			const uint32_t minimumVersion = toVkVersion(requirements.minApiVersion);
			const uint32_t maximumVersion = toVkVersion(requirements.maxApiVersion);
			if (appInfo.apiVersion > maximumVersion)
				appInfo.apiVersion = maximumVersion;
			if (appInfo.apiVersion < minimumVersion)
				throw std::runtime_error("OpenXR runtime requires an unsupported Vulkan API version.");
		} else {
			fe::LogWarning("Could not query OpenXR Vulkan requirements before Vulkan initialization");
		}
	}
#endif

	std::vector<const char*> extensions;
	extensions.reserve(extensionNames.size());
	for (const auto& extension : extensionNames)
		extensions.push_back(extension.c_str());

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
// void VulkanDevice::CreateSurface(IWindow *window) {
// 	auto surface  = (VkSurfaceKHR)window->CreateVulkanSurface(_instance);
// 	RegisterWindow(const IWindow *window)
// }

void VulkanDevice::PickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
	if (deviceCount == 0)
		throw std::runtime_error("No GPUs with Vulkan support found.");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

	VkPhysicalDevice openXRDevice = VK_NULL_HANDLE;
#ifndef FE_EXCLUDE_OPENXR
	if (preferOpenXRDevice_ && openXRRequirementsAvailable_) {
		void* runtimeDevice = nullptr;
		if (OpenXR::GetVulkanGraphicsDevice(_instance, runtimeDevice))
			openXRDevice = reinterpret_cast<VkPhysicalDevice>(runtimeDevice);
	}
#endif

	int bestScore = -1;
	VkPhysicalDevice bestDevice = VK_NULL_HANDLE;

	for (const auto& dev : devices) {
		if (openXRDevice != VK_NULL_HANDLE && dev != openXRDevice)
			continue;
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
		if (openXRDevice != VK_NULL_HANDLE)
			throw std::runtime_error("The OpenXR-selected Vulkan GPU cannot present to the application window.");
		throw std::runtime_error("No suitable GPU found.");
	}

	_physicalDevice = bestDevice;
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(_physicalDevice, &props);
	_deviceName = props.deviceName;
	std::cout << "Selected GPU: " << _deviceName << "\n";
}