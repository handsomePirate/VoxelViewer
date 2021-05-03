#pragma once
#include "Common.hpp"
#include <vector>

class VulkanFactory
{
public:
	class Instance
	{
	public:
		static VkInstance CreateInstance(const std::vector<const char*>& extensions, 
			uint32_t apiVersion, const char* validationLayerName = "");
		static void DestroyInstance(VkInstance instance);
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

		bool DebugMarkersEnabled;

		struct DeviceQueueFamilyIndices
		{
			uint32_t graphics;
			uint32_t compute;
			uint32_t transfer;
		} QueueFamilyIndices;
	private:
		uint32_t GetQueueFamilyIndex(VkQueueFlagBits queueFlags) const;
		VkDeviceQueueCreateInfo GetQueueCreateInfo(uint32_t& index, VkQueueFlags queueType) const;
	};
};
