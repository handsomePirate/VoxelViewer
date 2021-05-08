#pragma once
#include "Common.hpp"
#include "VulkanFactory.hpp"

class VulkanRender
{
public:
	static void PrepareFrame(VkDevice device, VkSwapchainKHR swapchain, VkSemaphore presentSemaphore, uint32_t currentBuffer);
	static void SubmitFrame(VkQueue queue, VkSwapchainKHR swapchain, VkSemaphore renderSemaphore, uint32_t currentBuffer);
};
