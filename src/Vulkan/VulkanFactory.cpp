#include "VulkanFactory.hpp"

bool VulkanFactory::Instance::CheckExtensionsPresent(const std::vector<const char*>& extensions)
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

VkInstance VulkanFactory::Instance::CreateInstance(const std::vector<const char*>& extensions,
	uint32_t apiVersion, const char* validationLayerName)
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "VoxelViewer";
	appInfo.pEngineName = "VoxelViewer";
	appInfo.apiVersion = apiVersion;

	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = NULL;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.enabledExtensionCount = (uint32_t)extensions.size();
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
	if (validationLayerName != "")
	{
		instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
		instanceCreateInfo.enabledLayerCount = 1;
	}

	VkInstance instance;
	VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
	assert(result == VK_SUCCESS);
	return instance;
}
