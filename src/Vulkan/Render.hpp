#pragma once
#include "Core/Common.hpp"
#include "Vulkan/VulkanFactory.hpp"

struct Context
{
	VkDevice Device;
	VkQueue Queue;
	// TODO: Populate with more data if necessary.
};

struct WindowData
{
	VkSwapchainKHR Swapchain;
	VkSemaphore RenderSemaphore;
	VkSemaphore PresentSemaphore;
	// TODO: Add data for window resizing, etc.
};

struct FrameData
{
	VkCommandBuffer CommandBuffer;
	VkFence Fence;
	uint32_t ImageIndex;
	// TODO: Add data for command buffer recording, too.
};

class VulkanRender
{
public:
	static uint32_t PrepareFrame(const Context& context, const WindowData& windowData, bool& shouldResize);
	static void RenderFrame(const Context& context, const WindowData& windowData, const FrameData& frameData, bool& shouldResize);
	static void ComputeFrame(const Context& context, const FrameData& frameData);
};
