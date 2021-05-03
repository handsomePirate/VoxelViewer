#include "Utils.hpp"
#include "Platform/Platform.hpp"
#include "Logger/Logger.hpp"

bool VulkanUtils::Instance::CheckExtensionsPresent(const std::vector<const char*>& extensions)
{
	// List available extensions.
	uint32_t availableExtensionCount;
	vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.data());

	// Try to find the required extensions in the list of available ones.
	for (auto&& extension : extensions)
	{
		bool found = false;
		for (auto&& availableExtension : availableExtensions)
		{
			if (strcmp(availableExtension.extensionName, extension) == 0)
			{
				found = true;
				break;
			}
		}
		if (!found)
			return false;
	}
	return true;
}

std::vector<VkPhysicalDevice> VulkanUtils::Device::EnumeratePhysicalDevices(VkInstance instance)
{
	// Get all available devices.
	uint32_t deviceCount;
	VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (result == VK_ERROR_INITIALIZATION_FAILED || deviceCount == 0)
	{
		CoreLogger.Log(Core::LoggerSeverity::Fatal, "Failed to find any GPUs with Vulkan support.");
		CorePlatform.Quit();
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	return devices;
}

VkPhysicalDeviceProperties VulkanUtils::Device::GetPhysicalDeviceProperties(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(device, &properties);
	return properties;
}

VkPhysicalDeviceFeatures VulkanUtils::Device::GetPhysicalDeviceFeatures(VkPhysicalDevice device)
{
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(device, &features);
	return features;
}

VkPhysicalDeviceMemoryProperties VulkanUtils::Device::GetPhysicalDeviceMemoryProperties(VkPhysicalDevice device)
{
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);
	return memoryProperties;
}

std::vector<VkQueueFamilyProperties> VulkanUtils::Device::GetQueueFamilyProperties(VkPhysicalDevice device)
{
	std::vector<VkQueueFamilyProperties> result;
	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	assert(queueFamilyCount > 0);
	result.resize(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, result.data());
	return result;
}

bool VulkanUtils::Device::CheckExtensionsSupported(VkPhysicalDevice device, const std::vector<const char*>& extensions)
{
	uint32_t extensionCount = 0;
	VkResult result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	if (result != VK_SUCCESS || extensionCount == 0)
	{
		CoreLogger.Log(Core::LoggerSeverity::Fatal, "Failed to enumerate GPU devices.");
		CorePlatform.Quit();
	}

	// Get available extensions.
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	// Compare them to requested ones.
	for (const auto& extension : extensions)
	{
		bool found = false;
		for (auto&& availableExtension : availableExtensions)
		{
			if (strcmp(availableExtension.extensionName, extension) == 0)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			return false;
		}
	}
	return true;
}

int VulkanUtils::Device::RateDevice(VkPhysicalDevice device)
{
	// TODO: Fill this in with a better algorithm (queue requirements, etc.).
	VkPhysicalDeviceProperties deviceProperties = GetPhysicalDeviceProperties(device);
	return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 1 : 0;
}

VkPhysicalDevice VulkanUtils::Device::PickDevice(const std::vector<VkPhysicalDevice>& devices)
{
	std::vector<int> devicePoints;
	devicePoints.resize(devices.size());

	for (int d = 0; d < devices.size(); ++d)
	{
		devicePoints[d] = RateDevice(devices[d]);
	}

	int maxD = 0;
	for (int d = 0; d < devices.size(); ++d)
	{
		if (devicePoints[d] > devicePoints[maxD])
		{
			maxD = d;
		}
	}
	return devices[maxD];
}
