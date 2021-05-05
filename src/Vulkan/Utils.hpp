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
	};
	class Memory
	{
	public:
		static uint32_t GetTypeIndex(VkPhysicalDeviceMemoryProperties memoryProperties,
			uint32_t filter, VkMemoryPropertyFlags requiredProperties);
	};
};
