#pragma once
#include "Common.hpp"
#include <vector>

class VulkanFactory
{
public:
	class Instance
	{
	public:
		static VkInstance Create(const std::vector<const char*>& extensions, 
			uint32_t apiVersion, const char* validationLayerName = "");
		static void Destroy(VkInstance instance);
	};
	class Device
	{
	public:
		void Init(VkPhysicalDevice physicalDevice);
		void Create(VkPhysicalDeviceFeatures& enabledFeatures, std::vector<const char*> extensions,
			VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, bool debugMarkersEnabled = false);
		void Destroy();
		
		VkDevice Handle = VK_NULL_HANDLE;

		VkPhysicalDevice PhysicalDevice;
		VkPhysicalDeviceFeatures EnabledFeatures;

		VkPhysicalDeviceProperties Properties;
		VkPhysicalDeviceFeatures Features;
		VkPhysicalDeviceMemoryProperties  MemoryProperties;
		std::vector<VkQueueFamilyProperties> QueueFamilyProperties;

		VkFormat DepthFormat;

		bool DebugMarkersEnabled;

		struct DeviceQueueFamilyIndices
		{
			uint32_t graphics;
			uint32_t compute;
			uint32_t transfer;
		} QueueFamilyIndices;
	private:
		uint32_t GetQueueFamilyIndex(VkQueueFlags queueFlags) const;
		VkDeviceQueueCreateInfo GetQueueInitializer(uint32_t& index, VkQueueFlags queueType) const;
	};
	class CommandPool
	{
	public:
		static VkCommandPool Create(VkDevice device, uint32_t queueIndex, VkCommandPoolCreateFlags flags = 0);
		static void Destroy(VkDevice device, VkCommandPool commandPool);
	};
	class Semaphore
	{
	public:
		static VkSemaphore Create(VkDevice device);
		static void Destroy(VkDevice device, VkSemaphore semaphore);
	};
	class Fence
	{
	public:
		static VkFence Create(VkDevice device, VkFenceCreateFlags flags = 0);
		static void Destroy(VkDevice device, VkFence fence);
	};
	class Surface
	{
	public:
		static VkSurfaceKHR Create(VkInstance instance, uint64_t windowHandle);
		static void Destroy(VkInstance instance, VkSurfaceKHR surface);
	};
};
