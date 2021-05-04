#include "VulkanFactory.hpp"
#include "Utils.hpp"
#include "Logger/Logger.hpp"
#include "Platform.hpp"
#include "Platform/Platform.hpp"
#include "Initializers.hpp"

VkInstance VulkanFactory::Instance::Create(const std::vector<const char*>& extensions,
	uint32_t apiVersion, const char* validationLayerName)
{
	VkApplicationInfo appInfo = VulkanInitializers::ApplicationInfo();
	appInfo.pApplicationName = "VoxelViewer";
	appInfo.pEngineName = "VoxelViewer";
	appInfo.apiVersion = apiVersion;

	auto instanceInitializer = VulkanInitializers::Instance(&appInfo);
	if (extensions.size() > 0)
	{
		instanceInitializer.enabledExtensionCount = (uint32_t)extensions.size();
		instanceInitializer.ppEnabledExtensionNames = extensions.data();
	}
	if (validationLayerName != "")
	{
		instanceInitializer.ppEnabledLayerNames = &validationLayerName;
		instanceInitializer.enabledLayerCount = 1;
	}

	VkInstance instance;
	VkResult result = vkCreateInstance(&instanceInitializer, nullptr, &instance);
	assert(result == VK_SUCCESS);
	return instance;
}

void VulkanFactory::Instance::Destroy(VkInstance instance)
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
	DepthFormat = VulkanUtils::Device::GetSupportedDepthFormat(physicalDevice);

	assert(DepthFormat != VK_FORMAT_UNDEFINED);
}

void VulkanFactory::Device::Create(VkPhysicalDeviceFeatures& enabledFeatures,
	std::vector<const char*> extensions, VkQueueFlags requestedQueueTypes, bool debugMarkersEnabled)
{
	DebugMarkersEnabled = debugMarkersEnabled;
	std::vector<VkDeviceQueueCreateInfo> queueInitializers{};

	// Graphics queue.
	if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
	{
		auto queueInitializer = GetQueueInitializer(QueueFamilyIndices.graphics, VK_QUEUE_GRAPHICS_BIT);
		queueInitializers.push_back(queueInitializer);
	}
	else
	{
		QueueFamilyIndices.graphics = UINT32_MAX;
	}

	// Compute queue.
	if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
	{
		auto queueInitializer = GetQueueInitializer(QueueFamilyIndices.compute, VK_QUEUE_GRAPHICS_BIT);
		if (QueueFamilyIndices.compute != QueueFamilyIndices.graphics)
		{
			queueInitializers.push_back(queueInitializer);
		}
	}
	else
	{
		QueueFamilyIndices.compute = QueueFamilyIndices.graphics;
	}

	// Transfer queue.
	if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
	{
		auto queueInitializer = GetQueueInitializer(QueueFamilyIndices.transfer, VK_QUEUE_TRANSFER_BIT);
		if (QueueFamilyIndices.transfer != QueueFamilyIndices.graphics &&
			QueueFamilyIndices.transfer != QueueFamilyIndices.compute)
		{
			queueInitializers.push_back(queueInitializer);
		}
	}
	else
	{
		QueueFamilyIndices.transfer = QueueFamilyIndices.graphics;
	}

	VkDeviceCreateInfo deviceInitializer = VulkanInitializers::Device();
	deviceInitializer.queueCreateInfoCount = (uint32_t)queueInitializers.size();
	deviceInitializer.pQueueCreateInfos = queueInitializers.data();
	deviceInitializer.pEnabledFeatures = &enabledFeatures;
	EnabledFeatures = enabledFeatures;
	if (extensions.size() > 0)
	{
		deviceInitializer.enabledExtensionCount = (uint32_t)extensions.size();
		deviceInitializer.ppEnabledExtensionNames = extensions.data();
	}

	VkResult result = vkCreateDevice(PhysicalDevice, &deviceInitializer, nullptr, &Handle);

	if (result != VK_SUCCESS)
	{
		CoreLogger.Log(Core::LoggerSeverity::Fatal, "Failed to connect with the graphics driver.");
		CorePlatform.Quit();
	}
}

void VulkanFactory::Device::Destroy()
{
	// TODO: When there is a default command pool, destroy here.
	vkDestroyDevice(Handle, nullptr);
}

uint32_t VulkanFactory::Device::GetQueueFamilyIndex(VkQueueFlags queueFlags) const
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

VkDeviceQueueCreateInfo VulkanFactory::Device::GetQueueInitializer(uint32_t& index, VkQueueFlags queueType) const
{
	const float defaultQueuePriority = 0.f;

	index = GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
	VkDeviceQueueCreateInfo queueInfo = VulkanInitializers::Queue(defaultQueuePriority);
	queueInfo.queueFamilyIndex = index;

	return queueInfo;
}

VkCommandPool VulkanFactory::CommandPool::Create(VkDevice device, uint32_t queueIndex, VkCommandPoolCreateFlags flags)
{
	auto commandPoolInitializer = VulkanInitializers::CommandPool();
	commandPoolInitializer.queueFamilyIndex = queueIndex;
	commandPoolInitializer.flags = flags;
	VkCommandPool commandPool;
	vkCreateCommandPool(device, &commandPoolInitializer, nullptr, &commandPool);
	return commandPool;
}

void VulkanFactory::CommandPool::Destroy(VkDevice device, VkCommandPool commandPool)
{
	vkDestroyCommandPool(device, commandPool, nullptr);
}

VkSemaphore VulkanFactory::Semaphore::Create(VkDevice device)
{
	auto semaphoreInitializer = VulkanInitializers::Semaphore();
	VkSemaphore semaphore;
	VkResult result = vkCreateSemaphore(device, &semaphoreInitializer, nullptr, &semaphore);
	assert(result == VK_SUCCESS);
	return semaphore;
}

void VulkanFactory::Semaphore::Destroy(VkDevice device, VkSemaphore semaphore)
{
	vkDestroySemaphore(device, semaphore, nullptr);
}

VkFence VulkanFactory::Fence::Create(VkDevice device, VkFenceCreateFlags flags)
{
	auto fenceInitializer = VulkanInitializers::Fence();
	fenceInitializer.flags = flags;
	VkFence fence;
	VkResult result = vkCreateFence(device, &fenceInitializer, nullptr, &fence);
	assert(result == VK_SUCCESS);
	return fence;
}

void VulkanFactory::Fence::Destroy(VkDevice device, VkFence fence)
{
	vkDestroyFence(device, fence, nullptr);
}

VkSurfaceKHR VulkanFactory::Surface::Create(VkInstance instance, uint64_t windowHandle)
{
	return SurfaceFactory::Create(instance, windowHandle);
}

void VulkanFactory::Surface::Destroy(VkInstance instance, VkSurfaceKHR surface)
{
	vkDestroySurfaceKHR(instance, surface, nullptr);
}

VkQueue VulkanFactory::Queue::Get(VkDevice device, uint32_t queueFamily, uint32_t queueIndex)
{
	VkQueue queue;
	vkGetDeviceQueue(device, queueFamily, queueIndex, &queue);
	return queue;
}
