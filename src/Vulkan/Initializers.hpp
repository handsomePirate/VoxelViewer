#pragma once
#include "Common.hpp"
#include "Utils.hpp"

namespace VulkanInitializers
{
	inline VkApplicationInfo ApplicationInfo()
	{
		VkApplicationInfo applicationInfo{};
		applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		return applicationInfo;
	}

	inline VkInstanceCreateInfo Instance(const VkApplicationInfo* appInfo)
	{
		VkInstanceCreateInfo instanceCreateInfo{};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pApplicationInfo = appInfo;
		return instanceCreateInfo;
	}

	inline VkDeviceQueueCreateInfo Queue(const std::vector<VkQueueFamilyProperties>& queueProperties, 
		VkQueueFlags flags, const float defaultPriority = 0.f)
	{
		const float defaultQueuePriority = 0.f;

		uint32_t index = VulkanUtils::Queue::GetQueueFamilyIndex(queueProperties, VK_QUEUE_GRAPHICS_BIT);

		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &defaultPriority;
		queueCreateInfo.queueFamilyIndex = index;
		return queueCreateInfo;
	}

	inline VkDeviceCreateInfo Device()
	{
		VkDeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		return deviceCreateInfo;
	}

	inline VkCommandPoolCreateInfo CommandPool()
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo{};
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		return commandPoolCreateInfo;
	}

	inline VkSemaphoreCreateInfo Semaphore()
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo{};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		return semaphoreCreateInfo;
	}

	inline VkFenceCreateInfo Fence()
	{
		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		return fenceCreateInfo;
	}

	inline VkSubmitInfo SubmitInfo()
	{
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		return submitInfo;
	}

	inline VkSwapchainCreateInfoKHR Swapchain(VkExtent2D extent, VkSurfaceKHR surface)
	{
		VkSwapchainCreateInfoKHR swapchainInfo{};
		swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainInfo.clipped = VK_TRUE;
		swapchainInfo.imageArrayLayers = 1;
		swapchainInfo.imageExtent = extent;
		swapchainInfo.surface = surface;
		return swapchainInfo;
	}

	inline VkImageViewCreateInfo ImageView(VkImage image)
	{
		VkImageViewCreateInfo imageView = {};
		imageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageView.image = image;
		return imageView;
	}

	inline VkImageViewCreateInfo ColorAttachmentView(VkImage image, VkFormat colorFormat)
	{
		VkImageViewCreateInfo colorAttachmentView = ImageView(image);
		
		colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		colorAttachmentView.format = colorFormat;
		colorAttachmentView.components =
		{
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A
		};
		colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorAttachmentView.subresourceRange.levelCount = 1;
		colorAttachmentView.subresourceRange.layerCount = 1;
		colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;

		return colorAttachmentView;
	}

	inline VkCommandBufferAllocateInfo CommandBufferAllocation(VkCommandPool commandPool,
		VkCommandBufferLevel level, uint32_t bufferCount)
	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandPool = commandPool;
		commandBufferAllocateInfo.level = level;
		commandBufferAllocateInfo.commandBufferCount = bufferCount;
		return commandBufferAllocateInfo;
	}
};
