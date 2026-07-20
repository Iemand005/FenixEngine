
#include "VulkanDevice.hpp"

void VulkanDevice::pickPhysicalDevice() {
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
	deviceName_ = props.deviceName;
	std::cout << "Selected GPU: " << deviceName_ << "\n";
}