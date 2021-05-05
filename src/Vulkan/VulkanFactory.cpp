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

void VulkanFactory::Device::Create(
	VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures& enabledFeatures,
	std::vector<const char*> extensions, DeviceInfo& output,
	VkQueueFlags requestedQueueTypes, bool debugMarkersEnabled)
{
	output.Properties = VulkanUtils::Device::GetPhysicalDeviceProperties(physicalDevice);
	output.Features = VulkanUtils::Device::GetPhysicalDeviceFeatures(physicalDevice);
	output.MemoryProperties = VulkanUtils::Device::GetPhysicalDeviceMemoryProperties(physicalDevice);
	output.QueueFamilyProperties = VulkanUtils::Device::GetQueueFamilyProperties(physicalDevice);
	output.DepthFormat = VulkanUtils::Device::GetSupportedDepthFormat(physicalDevice);

	assert(output.DepthFormat != VK_FORMAT_UNDEFINED);

	output.DebugMarkersEnabled = debugMarkersEnabled;
	std::vector<VkDeviceQueueCreateInfo> queueInitializers{};

	// Graphics queue.
	if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
	{
		auto queueInitializer = VulkanInitializers::Queue(output.QueueFamilyProperties, VK_QUEUE_GRAPHICS_BIT);
		output.QueueFamilyIndices.graphics = queueInitializer.queueFamilyIndex;
		queueInitializers.push_back(queueInitializer);
	}
	else
	{
		output.QueueFamilyIndices.graphics = UINT32_MAX;
	}

	// Compute queue.
	if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
	{
		auto queueInitializer = VulkanInitializers::Queue(output.QueueFamilyProperties, VK_QUEUE_COMPUTE_BIT);
		output.QueueFamilyIndices.compute = queueInitializer.queueFamilyIndex;
		if (output.QueueFamilyIndices.compute != output.QueueFamilyIndices.graphics)
		{
			queueInitializers.push_back(queueInitializer);
		}
	}
	else
	{
		output.QueueFamilyIndices.compute = output.QueueFamilyIndices.graphics;
	}

	// Transfer queue.
	if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
	{
		auto queueInitializer = VulkanInitializers::Queue(output.QueueFamilyProperties, VK_QUEUE_TRANSFER_BIT);
		output.QueueFamilyIndices.transfer = queueInitializer.queueFamilyIndex;
		if (output.QueueFamilyIndices.transfer != output.QueueFamilyIndices.graphics &&
			output.QueueFamilyIndices.transfer != output.QueueFamilyIndices.compute)
		{
			queueInitializers.push_back(queueInitializer);
		}
	}
	else
	{
		output.QueueFamilyIndices.transfer = output.QueueFamilyIndices.graphics;
	}

	VkDeviceCreateInfo deviceInitializer = VulkanInitializers::Device();
	deviceInitializer.queueCreateInfoCount = (uint32_t)queueInitializers.size();
	deviceInitializer.pQueueCreateInfos = queueInitializers.data();
	deviceInitializer.pEnabledFeatures = &enabledFeatures;
	output.EnabledFeatures = enabledFeatures;
	if (extensions.size() > 0)
	{
		deviceInitializer.enabledExtensionCount = (uint32_t)extensions.size();
		deviceInitializer.ppEnabledExtensionNames = extensions.data();
	}

	VkResult result = vkCreateDevice(physicalDevice, &deviceInitializer, nullptr, &output.Handle);

	if (result != VK_SUCCESS)
	{
		CoreLogger.Log(Core::LoggerSeverity::Fatal, "Failed to connect with the graphics driver.");
	}
}

void VulkanFactory::Device::Destroy(DeviceInfo& deviceInfo)
{
	// TODO: When there is a default command pool, destroy here.
	vkDestroyDevice(deviceInfo.Handle, nullptr);
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

void VulkanFactory::Swapchain::Create(
	uint32_t width, uint32_t height, VkSurfaceKHR surface, const Device::DeviceInfo& deviceInfo,
	SwapchainInfo& output, SwapchainInfo* oldSwapchainInfo)
{
	bool oldSwapchainProvided = oldSwapchainInfo != nullptr ? true : false;

	output.Extent = VulkanUtils::Surface::QueryExtent(width, height, deviceInfo.SurfaceCapabilities);

	uint32_t imageCount = VulkanUtils::Swapchain::QueryImageCount(deviceInfo.SurfaceCapabilities);

	auto swapchainInitializer = VulkanInitializers::Swapchain(output.Extent, surface);
	swapchainInitializer.imageFormat = deviceInfo.SurfaceFormat.format;
	swapchainInitializer.imageColorSpace = deviceInfo.SurfaceFormat.colorSpace;
	swapchainInitializer.compositeAlpha = deviceInfo.CompositeAlpha;
	swapchainInitializer.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainInitializer.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainInitializer.preTransform = deviceInfo.SurfaceTransform;
	swapchainInitializer.presentMode = deviceInfo.PresentMode;
	swapchainInitializer.oldSwapchain = oldSwapchainProvided ? oldSwapchainInfo->Handle : VK_NULL_HANDLE;
	swapchainInitializer.minImageCount = imageCount;

	// Enable transfer source on swap chain images if supported
	if (deviceInfo.SurfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
	{
		swapchainInitializer.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	// Enable transfer destination on swap chain images if supported
	if (deviceInfo.SurfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
	{
		swapchainInitializer.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	VkResult result = vkCreateSwapchainKHR(deviceInfo.Handle, &swapchainInitializer, nullptr, &output.Handle);

	if (result != VK_SUCCESS)
	{
		CoreLogger.Log(Core::LoggerSeverity::Fatal, "Failed to create swapchain!");
	}

	if (oldSwapchainProvided)
	{
		Destroy(deviceInfo, *oldSwapchainInfo);
	}

	output.Images = VulkanUtils::Swapchain::GetImages(deviceInfo.Handle, output.Handle);
	assert(output.Images.size() >= imageCount);

	output.ImageViews.resize(output.Images.size());
	for (int i = 0; i < output.Images.size(); ++i)
	{
		auto colorAttachmentViewInitializer = VulkanInitializers::ColorAttachmentView(
			output.Images[i], deviceInfo.SurfaceFormat.format);

		vkCreateImageView(deviceInfo.Handle, &colorAttachmentViewInitializer, nullptr, &output.ImageViews[i]);
	}
}

void VulkanFactory::Swapchain::Destroy(const Device::DeviceInfo& deviceInfo, SwapchainInfo& swapchainInfo)
{
	for (uint32_t i = 0; i < swapchainInfo.ImageViews.size(); i++)
	{
		vkDestroyImageView(deviceInfo.Handle, swapchainInfo.ImageViews[i], nullptr);
	}
	vkDestroySwapchainKHR(deviceInfo.Handle, swapchainInfo.Handle, nullptr);
}

void VulkanFactory::CommandBuffer::AllocatePrimary(VkDevice device, VkCommandPool commandPool,
	std::vector<VkCommandBuffer>& output, uint32_t bufferCount)
{
	output.resize(bufferCount);

	auto commandBufferInitializer = VulkanInitializers::CommandBufferAllocation(
		commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, bufferCount);

	vkAllocateCommandBuffers(device, &commandBufferInitializer, output.data());
}

VkCommandBuffer VulkanFactory::CommandBuffer::AllocatePrimary(VkDevice device, VkCommandPool commandPool)
{
	auto commandBufferInitializer = VulkanInitializers::CommandBufferAllocation(
		commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &commandBufferInitializer, &commandBuffer);
}

void VulkanFactory::CommandBuffer::Free(VkDevice device, VkCommandPool commandPool, std::vector<VkCommandBuffer>& buffers)
{
	vkFreeCommandBuffers(device, commandPool, buffers.size(), buffers.data());
}

void VulkanFactory::CommandBuffer::Free(VkDevice device, VkCommandPool commandPool, VkCommandBuffer buffer)
{
	vkFreeCommandBuffers(device, commandPool, 1, &buffer);
}
