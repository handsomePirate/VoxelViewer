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

	inline VkImageViewCreateInfo ImageView(VkImage image, VkFormat format)
	{
		VkImageViewCreateInfo imageView = {};
		imageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageView.image = image;
		imageView.format = format;
		return imageView;
	}

	inline VkImageViewCreateInfo ColorAttachmentView(VkImage image, VkFormat format)
	{
		VkImageViewCreateInfo colorAttachmentView = ImageView(image, format);
		
		colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
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

	inline VkImageViewCreateInfo DepthAttachmentView(VkImage image, VkFormat format)
	{
		VkImageViewCreateInfo depthAttachmentView = ImageView(image, format);

		depthAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		depthAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		depthAttachmentView.subresourceRange.levelCount = 1;
		depthAttachmentView.subresourceRange.layerCount = 1;
		depthAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;

		if (format >= VK_FORMAT_D16_UNORM_S8_UINT)
		{
			depthAttachmentView.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		return depthAttachmentView;
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

	inline VkImageCreateInfo Image(VkFormat format)
	{
		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		return imageCreateInfo;
	}

	inline VkMemoryAllocateInfo MemoryAllocation(VkDeviceSize size, uint32_t typeIndex)
	{
		VkMemoryAllocateInfo memoryAllocateInfo{};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.allocationSize = size;
		memoryAllocateInfo.memoryTypeIndex = typeIndex;
		return memoryAllocateInfo;
	}

	inline VkFramebufferCreateInfo Framebuffer()
	{
		VkFramebufferCreateInfo framebufferCreateInfo{};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		return framebufferCreateInfo;
	}

	inline VkPipelineCacheCreateInfo PipelineCache()
	{
		VkPipelineCacheCreateInfo pipelineCacheInfo{};
		pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		return pipelineCacheInfo;
	}
};
