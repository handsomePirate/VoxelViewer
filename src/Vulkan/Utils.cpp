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

VkFormat VulkanUtils::Device::GetSupportedDepthFormat(VkPhysicalDevice device)
{
	// Since all depth formats may be optional, we need to find a suitable depth format to use
			// Start with the highest precision packed format
	std::vector<VkFormat> depthFormats = {
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM
	};

	for (auto& format : depthFormats)
	{
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(device, format, &formatProperties);
		// Format must support depth stencil attachment for optimal tiling
		if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			return format;
		}
	}

	return VK_FORMAT_UNDEFINED;
}

uint32_t VulkanUtils::Device::GetPresentQueueIndex(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t graphicsIndex)
{
	uint32_t queueCount;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, NULL);
	assert(queueCount >= 1);

	std::vector<VkQueueFamilyProperties> queueProperties;
	queueProperties.resize(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, queueProperties.data());

	std::vector<VkBool32> supportsPresent;
	supportsPresent.resize(queueCount);
	for (uint32_t i = 0; i < queueCount; i++)
	{
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supportsPresent[i]);
	}

	if (supportsPresent[graphicsIndex])
	{
		return graphicsIndex;
	}

	for (uint32_t i = 0; i < queueCount; ++i)
	{
		if (supportsPresent[i])
		{
			CoreLogger.Log(Core::LoggerSeverity::Warn, "Present queue doesn't match graphics queue.");
			return i;
		}
	}

	CoreLogger.Log(Core::LoggerSeverity::Fatal, "Couldn't find a present queue!");
	CorePlatform.Quit();
	return 0;
}

VkSurfaceFormatKHR VulkanUtils::Device::QuerySurfaceFormat(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	// Get list of supported surface formats
	uint32_t formatCount;
	VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, NULL);
	assert(formatCount > 0 && result == VK_SUCCESS);

	std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, surfaceFormats.data());

	VkSurfaceFormatKHR surfaceFormat;

	// If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
	// there is no preferered format, so we assume VK_FORMAT_B8G8R8A8_UNORM
	if ((formatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED))
	{
		surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
		surfaceFormat.colorSpace = surfaceFormats[0].colorSpace;
	}
	else
	{
		// iterate over the list of available surface format and
		// check for the presence of VK_FORMAT_B8G8R8A8_UNORM
		for (auto&& surfaceFormat : surfaceFormats)
		{
			if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
			{
				surfaceFormat.format = surfaceFormat.format;
				surfaceFormat.colorSpace = surfaceFormat.colorSpace;
				return surfaceFormat;
			}
		}

		// in case VK_FORMAT_B8G8R8A8_UNORM is not available
		// select the first available color format
		surfaceFormat.format = surfaceFormats[0].format;
		surfaceFormat.colorSpace = surfaceFormats[0].colorSpace;
	}
	return surfaceFormat;
}
