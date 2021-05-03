#pragma once
#include "Common.hpp"
#include <vector>

class VulkanUtils
{
public:
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
	};
};
