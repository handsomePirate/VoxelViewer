#include "VulkanFactory.hpp"
#include "Utils.hpp"
#include "Logger/Logger.hpp"
#include "Platform/Platform.hpp"

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
	if (extensions.size() > 0)
	{
		instanceCreateInfo.enabledExtensionCount = (uint32_t)extensions.size();
		instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
	}
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

void VulkanFactory::Instance::DestroyInstance(VkInstance instance)
{
	vkDestroyInstance(instance, nullptr);
}

void VulkanFactory::Device::Init(VkPhysicalDevice physicalDevice)
{
	PhysicalDevice = physicalDevice;
	Properties = VulkanUtils::Device::GetPhysicalDeviceProperties(physicalDevice);
	Features = VulkanUtils::Device::GetPhysicalDeviceFeatures(physicalDevice);
	MemoryProperties = VulkanUtils::Device::GetPhysicalDeviceMemoryProperties(physicalDevice);
	QueueFamilyProperties = VulkanUtils::Device::GetQueueFamilyProperties(physicalDevice);
}

void VulkanFactory::Device::Create(VkPhysicalDeviceFeatures& enabledFeatures,
	std::vector<const char*> extensions, VkQueueFlags requestedQueueTypes, bool debugMarkersEnabled)
{
	DebugMarkersEnabled = debugMarkersEnabled;
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

	// Graphics queue.
	if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
	{
		auto queueInfo = GetQueueCreateInfo(QueueFamilyIndices.graphics, VK_QUEUE_GRAPHICS_BIT);
		queueCreateInfos.push_back(queueInfo);
	}
	else
	{
		QueueFamilyIndices.graphics = UINT32_MAX;
	}

	// Compute queue.
	if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
	{
		auto queueInfo = GetQueueCreateInfo(QueueFamilyIndices.compute, VK_QUEUE_GRAPHICS_BIT);
		if (QueueFamilyIndices.compute != QueueFamilyIndices.graphics)
		{
			queueCreateInfos.push_back(queueInfo);
		}
	}
	else
	{
		QueueFamilyIndices.compute = UINT32_MAX;
	}

	// Transfer queue.
	if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
	{
		auto queueInfo = GetQueueCreateInfo(QueueFamilyIndices.transfer, VK_QUEUE_TRANSFER_BIT);
		if (QueueFamilyIndices.transfer != QueueFamilyIndices.graphics &&
			QueueFamilyIndices.transfer != QueueFamilyIndices.compute)
		{
			queueCreateInfos.push_back(queueInfo);
		}
	}
	else
	{
		QueueFamilyIndices.transfer = UINT32_MAX;
	}

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.pEnabledFeatures = &enabledFeatures;
	EnabledFeatures = enabledFeatures;
	if (extensions.size() > 0)
	{
		deviceCreateInfo.enabledExtensionCount = (uint32_t)extensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = extensions.data();
	}

	VkResult result = vkCreateDevice(PhysicalDevice, &deviceCreateInfo, nullptr, &Handle);

	if (result != VK_SUCCESS)
	{
		CoreLogger.Log(Core::LoggerSeverity::Fatal, "Failed to connect with the graphics driver.");
		CorePlatform.Quit();
	}

	// TODO: Create default command pool for graphics queue.
}

void VulkanFactory::Device::Destroy()
{
	// TODO: When there is a default command pool, destroy here.
	vkDestroyDevice(Handle, nullptr);
}

uint32_t VulkanFactory::Device::GetQueueFamilyIndex(VkQueueFlagBits queueFlags) const
{
	// Dedicated queue for compute.
	// Try to find a queue family index that supports compute but not graphics.
	if (queueFlags & VK_QUEUE_COMPUTE_BIT)
	{
		for (uint32_t f = 0; f < (uint32_t)QueueFamilyProperties.size(); ++f)
		{
			if ((QueueFamilyProperties[f].queueFlags & queueFlags) && 
				((QueueFamilyProperties[f].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
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
		for (uint32_t f = 0; f < (uint32_t)QueueFamilyProperties.size(); ++f)
		{
			if ((QueueFamilyProperties[f].queueFlags & queueFlags) && 
				((QueueFamilyProperties[f].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && 
				((QueueFamilyProperties[f].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
			{
				return f;
				break;
			}
		}
	}

	// For other queue types or if no separate compute queue is present, return the first one to support the requested flags.
	for (uint32_t f = 0; f < (uint32_t)QueueFamilyProperties.size(); ++f)
	{
		if (QueueFamilyProperties[f].queueFlags & queueFlags)
		{
			return f;
			break;
		}
	}

	CoreLogger.Log(Core::LoggerSeverity::Error, "Could not find matching queue family index.");
	return UINT32_MAX;
}

VkDeviceQueueCreateInfo VulkanFactory::Device::GetQueueCreateInfo(uint32_t& index, VkQueueFlags queueType) const
{
	const float defaultQueuePriority = 0.f;

	index = GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
	VkDeviceQueueCreateInfo queueInfo{};
	queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfo.queueFamilyIndex = QueueFamilyIndices.graphics;
	queueInfo.queueCount = 1;
	queueInfo.pQueuePriorities = &defaultQueuePriority;

	return queueInfo;
}
