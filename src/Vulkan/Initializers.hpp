#pragma once
#include "Common.hpp"

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

	inline VkDeviceQueueCreateInfo Queue(const float defaultPriority)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &defaultPriority;
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
};
