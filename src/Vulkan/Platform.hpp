#pragma once
#include "Common.hpp"

class SurfaceFactory
{
public:
	static VkSurfaceKHR Create(VkInstance instance, uint64_t windowHandle);
};
