#pragma once
#include "Common.hpp"
#include <vector>

// TODO: Large Vulkan structs need to be handled by reference.

namespace VulkanUtils
{
	class Instance
	{
	public:
		static bool CheckExtensionsPresent(const std::vector<const char*>& extensions);
	};
	class Device
	{
	public:
		static std::vector<VkPhysicalDevice> EnumeratePhysicalDevices(VkInstance instance);
		static VkPhysicalDeviceProperties GetPhysicalDeviceProperties(VkPhysicalDevice device);
		static VkPhysicalDeviceFeatures GetPhysicalDeviceFeatures(VkPhysicalDevice device);
		static VkFormatProperties GetPhysicalDeviceFormatProperties(VkPhysicalDevice device, VkFormat format);
		static VkPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties(VkPhysicalDevice device);
		static std::vector<VkQueueFamilyProperties> GetQueueFamilyProperties(VkPhysicalDevice device);
		static bool CheckExtensionsSupported(VkPhysicalDevice device, const std::vector<const char*>& extensions);
		static int RateDevice(VkPhysicalDevice device);
		static VkPhysicalDevice PickDevice(const std::vector<VkPhysicalDevice>& devices);

		static VkFormat GetSupportedDepthFormat(VkPhysicalDevice device);

		static uint32_t GetPresentQueueIndex(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t graphicsIndex);
	};
	class Queue
	{
	public:
		static uint32_t GetQueueFamilyIndex(const std::vector<VkQueueFamilyProperties>& queueProperties, VkQueueFlags queueFlags);
		static void Submit(VkCommandBuffer commandBuffer, VkQueue queue);
		static void WaitIdle(VkQueue queue);
	};
	class CommandBuffer
	{
	public:
		static void Begin(VkCommandBuffer commandBuffer);
		static void End(VkCommandBuffer commandBuffer);
	};
	class Surface
	{
	public:
		static VkSurfaceFormatKHR QueryFormat(VkPhysicalDevice device, VkSurfaceKHR surface);
		static VkSurfaceCapabilitiesKHR QueryCapabilities(VkPhysicalDevice device, VkSurfaceKHR surface);
		static VkExtent2D QueryExtent(uint32_t width, uint32_t height, VkSurfaceCapabilitiesKHR surfaceCapabilities);
		static VkSurfaceTransformFlagBitsKHR QueryTransform(VkSurfaceCapabilitiesKHR surfaceCapabilities);
	};
	class Swapchain
	{
	public:
		static VkPresentModeKHR QueryPresentMode(VkPhysicalDevice device, VkSurfaceKHR surface, bool vSync = false);
		static uint32_t QueryImageCount(VkSurfaceCapabilitiesKHR surfaceCapabilities);
		static VkCompositeAlphaFlagBitsKHR QueryCompositeAlpha(VkSurfaceCapabilitiesKHR surfaceCapabilities);
		static std::vector<VkImage> GetImages(VkDevice device, VkSwapchainKHR swapchain);
	};
	class Image
	{
	public:
		static inline VkMemoryRequirements GetMemoryRequirements(VkDevice device, VkImage image)
		{
			VkMemoryRequirements memoryRequirements;
			vkGetImageMemoryRequirements(device, image, &memoryRequirements);
			return  memoryRequirements;
		}
		static void TransitionLayout(VkCommandBuffer cmdbuffer, VkImage image,
			VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
			VkImageSubresourceRange subresourceRange,
			VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	};
	class Buffer
	{
	public:
		static void Copy(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, void* source);
		static void Copy(VkDevice device, VkBuffer source, VkBuffer destination, VkDeviceSize size,
			VkCommandPool commandPool, VkQueue queue, VkDeviceSize sourceOffset = 0, VkDeviceSize destinationOffset = 0);
	};
	class Descriptor
	{
	public:
		static void WriteImageSet(VkDevice device, VkDescriptorSet descriptorSet,
			VkDescriptorImageInfo* imageInfo);
		static void WriteComputeSet(VkDevice device, VkDescriptorSet descriptorSet, VkDescriptorImageInfo* renderTargetInfo/*,
			VkDescriptorBufferInfo* storageBufferInfo*/);
	};
	class Memory
	{
	public:
		static uint32_t GetTypeIndex(VkPhysicalDeviceMemoryProperties memoryProperties,
			uint32_t filter, VkMemoryPropertyFlags requiredProperties);
		static VkDeviceMemory AllocateBuffer(VkDevice device, VkPhysicalDeviceMemoryProperties deviceMemoryProperties,
			VkBuffer buffer, VkMemoryPropertyFlags memoryProperties);
		static VkDeviceMemory AllocateImage(VkDevice device, VkPhysicalDeviceMemoryProperties deviceMemoryProperties,
			VkImage image, VkMemoryPropertyFlags memoryProperties);
		static void* Map(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size,
			VkMemoryMapFlags flags = 0);
		static void Unmap(VkDevice device, VkDeviceMemory memory);
	};
	class Pipeline
	{
	public:
		static inline VkViewport CreateViewport(uint32_t width, uint32_t height, float minDepth, float maxDepth)
		{
			VkViewport viewport;
			viewport.x = 0;
			viewport.y = 0;
			viewport.width = (float)width;
			viewport.height = (float)height;
			viewport.minDepth = minDepth;
			viewport.maxDepth = maxDepth;
			return viewport;
		}

		static inline VkRect2D CreateScissor(VkExtent2D extent)
		{
			VkRect2D scissor;
			scissor.offset = { 0, 0 };
			scissor.extent = extent;
			return scissor;
		}
	};
};
