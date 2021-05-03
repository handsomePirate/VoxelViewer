#pragma once
#include "Common.hpp"
#include <vector>

class VulkanFactory
{
public:
	class Instance
	{
	public:
		static bool CheckExtensionsPresent(const std::vector<const char*>& extensions);
		static VkInstance CreateInstance(const std::vector<const char*>& extensions, 
			uint32_t apiVersion, const char* validationLayerName = "");
	};
private:
};
