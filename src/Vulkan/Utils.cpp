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
		return {};
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
		return false;
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
	return 0;
}

uint32_t VulkanUtils::Queue::GetQueueFamilyIndex(const std::vector<VkQueueFamilyProperties>& queueProperties, VkQueueFlags queueFlags)
{
	// Dedicated queue for compute.
	// Try to find a queue family index that supports compute but not graphics.
	if (queueFlags & VK_QUEUE_COMPUTE_BIT)
	{
		for (uint32_t f = 0; f < (uint32_t)queueProperties.size(); ++f)
		{
			if ((queueProperties[f].queueFlags & queueFlags) &&
				((queueProperties[f].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
			{
				return f;
				break;
			}
		}
	}

	// Dedicated queue for transfer.
	// Try to find a queue family index that supports transfer but not graphics or compute.
	if (queueFlags & VK_QUEUE_TRANSFER_BIT)
	{
		for (uint32_t f = 0; f < (uint32_t)queueProperties.size(); ++f)
		{
			if ((queueProperties[f].queueFlags & queueFlags) &&
				((queueProperties[f].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) &&
				((queueProperties[f].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
			{
				return f;
				break;
			}
		}
	}

	// For other queue types or if no separate compute queue is present, return the first one to support the requested flags.
	for (uint32_t f = 0; f < (uint32_t)queueProperties.size(); ++f)
	{
		if (queueProperties[f].queueFlags & queueFlags)
		{
			return f;
			break;
		}
	}

	CoreLogger.Log(Core::LoggerSeverity::Error, "Could not find matching queue family index.");
	return UINT32_MAX;
}

VkSurfaceFormatKHR VulkanUtils::Surface::QueryFormat(VkPhysicalDevice device, VkSurfaceKHR surface)
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

VkSurfaceCapabilitiesKHR VulkanUtils::Surface::QueryCapabilities(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);
	return capabilities;
}

VkExtent2D VulkanUtils::Surface::QueryExtent(uint32_t width, uint32_t height, VkSurfaceCapabilitiesKHR surfaceCapabilities)
{
	if (surfaceCapabilities.currentExtent.width != UINT32_MAX)
		return surfaceCapabilities.currentExtent;

	VkExtent2D actualExtent = { (uint32_t)(width), (uint32_t)(height) };

	actualExtent.width = std::max<uint32_t>(surfaceCapabilities.minImageExtent.width,
		std::min<uint32_t>(surfaceCapabilities.maxImageExtent.width, actualExtent.width));
	actualExtent.height = std::max<uint32_t>(surfaceCapabilities.minImageExtent.height,
		std::min<uint32_t>(surfaceCapabilities.maxImageExtent.height, actualExtent.height));

	return actualExtent;
}

VkSurfaceTransformFlagBitsKHR VulkanUtils::Surface::QueryTransform(VkSurfaceCapabilitiesKHR surfaceCapabilities)
{
	VkSurfaceTransformFlagBitsKHR transform;
	if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		// We prefer a non-rotated transform
		transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{
		transform = surfaceCapabilities.currentTransform;
	}

	return transform;
}

VkPresentModeKHR VulkanUtils::Swapchain::QueryPresentMode(VkPhysicalDevice device, VkSurfaceKHR surface, bool vSync)
{
	if (vSync)
	{
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
		&presentModeCount, presentModes.data());

	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (auto&& presentMode : presentModes)
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentMode;
		}
		if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			bestMode = presentMode;
		}
	}

	return bestMode;
}

uint32_t VulkanUtils::Swapchain::QueryImageCount(VkSurfaceCapabilitiesKHR surfaceCapabilities)
{
	uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
	if ((surfaceCapabilities.maxImageCount > 0) && (imageCount > surfaceCapabilities.maxImageCount))
	{
		imageCount = surfaceCapabilities.maxImageCount;
	}
	return imageCount;
}

VkCompositeAlphaFlagBitsKHR VulkanUtils::Swapchain::QueryCompositeAlpha(VkSurfaceCapabilitiesKHR surfaceCapabilities)
{
	std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags =
	{
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
	};

	for (auto& compositeAlphaFlag : compositeAlphaFlags)
	{
		if (surfaceCapabilities.supportedCompositeAlpha & compositeAlphaFlag)
		{
			return compositeAlphaFlag;
		};
	}

	return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
}

std::vector<VkImage> VulkanUtils::Swapchain::GetImages(VkDevice device, VkSwapchainKHR swapchain)
{
	uint32_t imageCount;
	VkResult result = vkGetSwapchainImagesKHR(device, swapchain, &imageCount, NULL);
	assert(result == VK_SUCCESS);

	// Get the swap chain images
	std::vector<VkImage> images;
	images.resize(imageCount);
	result = vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data());
	assert(result == VK_SUCCESS);

	return images;
}
