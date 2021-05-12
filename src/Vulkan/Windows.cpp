#include "Platform.hpp"

#ifdef PLATFORM_WINDOWS

#include <Windows.h>
#include "Core/Platform/Platform.hpp"
#include <vulkan/vulkan_win32.h>

VkSurfaceKHR SurfaceFactory::Create(VkInstance instance, uint64_t windowHandle)
{
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hinstance = (HINSTANCE)CorePlatform.GetProgramID();
	surfaceCreateInfo.hwnd = (HWND)windowHandle;
	VkSurfaceKHR surface;
	VkResult result = vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
	return surface;
}

#endif
