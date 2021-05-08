#include "Render.hpp"
#include "Initializers.hpp"
#include "Logger/Logger.hpp"

void VulkanRender::PrepareFrame(VkDevice device, VkSwapchainKHR swapchain, VkSemaphore presentSemaphore,
	uint32_t currentBuffer)
{
	// Acquire the next image from the swap chain
	VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, presentSemaphore, VK_NULL_HANDLE, &currentBuffer);
	// Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
	if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR))
	{
		CoreLogger.Log(Core::LoggerSeverity::Warn, "Should resize Vulkan structures.");
	}
	else
	{
		assert(result == VK_SUCCESS);
	}
}

void VulkanRender::SubmitFrame(VkQueue queue, VkSwapchainKHR swapchain, VkSemaphore renderSemaphore, uint32_t currentBuffer)
{
	auto presentInitializer = VulkanInitializers::Present(1, &swapchain, &currentBuffer);
	presentInitializer.waitSemaphoreCount = 1;
	presentInitializer.pWaitSemaphores = &renderSemaphore;
	VkResult result = vkQueuePresentKHR(queue, &presentInitializer);
	if (!((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR)))
	{
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			// Swap chain is no longer compatible with the surface and needs to be recreated
			CoreLogger.Log(Core::LoggerSeverity::Warn, "Should resize Vulkan structures.");
			return;
		}
		else
		{
			assert(result == VK_SUCCESS);
		}
	}
	result = vkQueueWaitIdle(queue);
	assert(result == VK_SUCCESS);
}
