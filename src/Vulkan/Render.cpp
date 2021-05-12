#include "Render.hpp"
#include "Initializers.hpp"
#include "Core/Logger/Logger.hpp"

inline VkSubmitInfo PrepareSubmitInfo(bool semaphores = true);
void PresentFrame(VkQueue queue, VkSwapchainKHR swapchain, VkSemaphore renderSemaphore, uint32_t currentImageIndex);

uint32_t VulkanRender::PrepareFrame(const Context& context, const WindowData& windowData)
{
	// Acquire the next image from the swap chain
	uint32_t currentImageIndex;
	VkResult result = vkAcquireNextImageKHR(context.Device, windowData.Swapchain, UINT64_MAX, windowData.PresentSemaphore,
		VK_NULL_HANDLE, &currentImageIndex);
	// Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
	if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR))
	{
		CoreLogWarn("Should resize Vulkan structures.");
	}
	else
	{
		assert(result == VK_SUCCESS);
	}
	return currentImageIndex;
}

void VulkanRender::RenderFrame(const Context& context, const WindowData& windowData, const FrameData& frameData)
{
	static VkPipelineStageFlags pipelineStageWait = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	static auto submitInfo = PrepareSubmitInfo();
	submitInfo.pWaitDstStageMask = &pipelineStageWait;
	submitInfo.pWaitSemaphores = &windowData.PresentSemaphore;
	submitInfo.pSignalSemaphores = &windowData.RenderSemaphore;
	submitInfo.pCommandBuffers = &frameData.CommandBuffer;
	
	VkResult result = vkQueueSubmit(context.Queue, 1, &submitInfo, VK_NULL_HANDLE);
	assert(result == VK_SUCCESS);

	PresentFrame(context.Queue, windowData.Swapchain, windowData.RenderSemaphore, frameData.ImageIndex);
}

void VulkanRender::ComputeFrame(const Context& context, const FrameData& frameData)
{
	static auto submitInfo = PrepareSubmitInfo(false);
	submitInfo.pCommandBuffers = &frameData.CommandBuffer;

	vkWaitForFences(context.Device, 1, &frameData.Fence, VK_TRUE, UINT64_MAX);
	vkResetFences(context.Device, 1, &frameData.Fence);

	VkSubmitInfo computeSubmitInfo = VulkanInitializers::SubmitInfo();
	computeSubmitInfo.commandBufferCount = 1;
	computeSubmitInfo.pCommandBuffers = &frameData.CommandBuffer;

	VkResult result = vkQueueSubmit(context.Queue, 1, &computeSubmitInfo, frameData.Fence);
	assert(result == VK_SUCCESS);
}

inline VkSubmitInfo PrepareSubmitInfo(bool semaphores)
{
	auto submitInfo = VulkanInitializers::SubmitInfo();
	if (semaphores)
	{
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.signalSemaphoreCount = 1;
	}
	submitInfo.commandBufferCount = 1;
	return submitInfo;
}

void PresentFrame(VkQueue queue, VkSwapchainKHR swapchain, VkSemaphore renderSemaphore, uint32_t currentImageIndex)
{
	auto presentInitializer = VulkanInitializers::Present(1, &swapchain, &currentImageIndex);
	presentInitializer.waitSemaphoreCount = 1;
	presentInitializer.pWaitSemaphores = &renderSemaphore;
	VkResult result = vkQueuePresentKHR(queue, &presentInitializer);
	if (!((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR)))
	{
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			// Swap chain is no longer compatible with the surface and needs to be recreated
			CoreLogWarn("Should resize Vulkan structures.");
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
