#include "Common.hpp"
#include "Platform/Platform.hpp"
#include "Logger/Logger.hpp"
#include "Events/EventSystem.hpp"
#include "Vulkan/VulkanFactory.hpp"
#include "Vulkan/Utils.hpp"
#include "Debug/Debug.hpp"
#include <vector>
#include <string>
#include <functional>

#include <iostream>

struct EventLogger
{
	bool KeyPressed(Core::EventCode code, Core::EventData context)
	{
		std::string keyName;

		switch (context.data.u16[0])
		{
		case (uint16_t)Core::Key::Ascii:
			keyName = (unsigned char)context.data.u16[1];
			break;
		case (uint16_t)Core::Key::Space:
			keyName = "Space";
			break;
		case (uint16_t)Core::Key::Enter:
			keyName = "Enter";
			break;
		case (uint16_t)Core::Key::Escape:
			keyName = "Esc";
			CorePlatform.Quit();
			break;
		case (uint16_t)Core::Key::Arrow:
			static std::string arrowDirections[] = { "left", "right", "up", "down" };
			keyName = "Arrow " + arrowDirections[context.data.u16[1]];
			break;
		}

		CoreLogger.Log(Core::LoggerSeverity::Trace, "Button pressed; data = %s", keyName.c_str());
		return true;
	}

	bool MousePressed(Core::EventCode code, Core::EventData context)
	{
		std::string whichButtonString;

		switch (context.data.u8[5])
		{
		case 0:
			whichButtonString = "left";
			break;
		case 1:
			whichButtonString = "right";
			break;
		}

		CoreLogger.Log(Core::LoggerSeverity::Trace, "Mouse pressed at (%hu, %hu) with %s button", 
			context.data.u16[0], context.data.u16[1], whichButtonString.c_str());
		return true;
	}

	bool WindowResized(Core::EventCode code, Core::EventData context)
	{
		CoreLogger.Log(Core::LoggerSeverity::Trace, "Window resized to (%hu, %hu)",
			context.data.u16[0], context.data.u16[1]);
		return true;
	}

	bool VulkanValidation(Core::EventCode code, Core::EventData context)
	{
		Core::LoggerSeverity severity;
		switch ((VkDebugUtilsMessageSeverityFlagBitsEXT)context.data.u32[2])
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			severity = Core::LoggerSeverity::Trace;
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			severity = Core::LoggerSeverity::Info;
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			severity = Core::LoggerSeverity::Warn;
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			severity = Core::LoggerSeverity::Error;
			break;
		}

		const VkDebugUtilsMessengerCallbackDataEXT* callbackData = 
			(const VkDebugUtilsMessengerCallbackDataEXT*)context.data.u64[0];

		CoreLogger.Log(severity, "%s", callbackData->pMessage);
		return true;
	}
};

int main(int argc, char* argv[])
{
	const bool enableVulkanDebug = true;
	const bool useSwapchain = true;

	//=========================== Testing event logging ==============================

	EventLogger eventLogger;

	auto keyPressFnc = std::bind(&EventLogger::KeyPressed, &eventLogger, std::placeholders::_1, std::placeholders::_2);
	CoreEventSystem.SubscribeToEvent(Core::EventCode::KeyPressed, keyPressFnc, &eventLogger);
	auto mousePressFnc = std::bind(&EventLogger::MousePressed, &eventLogger, std::placeholders::_1, std::placeholders::_2);
	CoreEventSystem.SubscribeToEvent(Core::EventCode::MouseButtonPressed, mousePressFnc, &eventLogger);
	auto resizeFnc = std::bind(&EventLogger::WindowResized, &eventLogger, std::placeholders::_1, std::placeholders::_2);
	CoreEventSystem.SubscribeToEvent(Core::EventCode::WindowResized, resizeFnc, &eventLogger);
	auto vulkanValidationFnc = std::bind(&EventLogger::VulkanValidation, &eventLogger, std::placeholders::_1, std::placeholders::_2);
	CoreEventSystem.SubscribeToEvent(Core::EventCode::VulkanValidation, vulkanValidationFnc, &eventLogger);

	//=========================== Setting up Vulkan Instance =========================

	std::vector<const char*> vulkanExtensions =
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
		CorePlatform.GetVulkanSurfacePlatformExtension()
	};

	const char* validationLayer = enableVulkanDebug ? "VK_LAYER_KHRONOS_validation" : "";
	if (enableVulkanDebug)
	{
		vulkanExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		assert(Debug::CheckValidationLayerPresent(validationLayer));
	}

	VulkanUtils::Instance::CheckExtensionsPresent(vulkanExtensions);

	VkInstance instance = VulkanFactory::Instance::CreateInstance(
		vulkanExtensions, VK_MAKE_VERSION(1, 2, 0), validationLayer);

	if (enableVulkanDebug)
	{
		Debug::StartInstanceValidationLayers(instance);
	}

	//=========================== Enumerating and selecting physical devices =========

	std::vector<VkPhysicalDevice> physicalDevices;
	physicalDevices = VulkanUtils::Device::EnumeratePhysicalDevices(instance);

	for (int d = 0; d < physicalDevices.size(); ++d)
	{
		auto deviceProperties = VulkanUtils::Device::GetPhysicalDeviceProperties(physicalDevices[d]);

		CoreLogger.Log(Core::LoggerSeverity::Debug, "Found device (%i): %s", d, deviceProperties.deviceName);
	}

	VkPhysicalDevice pickedDevice = VulkanUtils::Device::PickDevice(physicalDevices);
	VulkanFactory::Device device;
	device.Init(pickedDevice);

	VkPhysicalDeviceFeatures requestedFeatures{};
	std::vector<const char*> deviceExtensions;
	bool debugMarkersEnabled = false;
	if (enableVulkanDebug)
	{
		deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
		debugMarkersEnabled = VulkanUtils::Device::CheckExtensionsSupported(pickedDevice, deviceExtensions);
		if (!debugMarkersEnabled)
		{
			deviceExtensions.clear();
		}
	}
	if (useSwapchain)
	{
		deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	}

	// TODO: Check if we will need a transfer queue.
	device.Create(requestedFeatures, deviceExtensions,
		VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, debugMarkersEnabled);

	CoreLogger.Log(Core::LoggerSeverity::Debug, "Picked %s", device.Properties.deviceName);

	//=========================== Creating logical device ============================

	//=========================== Testing window system ==============================

	std::vector<Core::Window*> windows;
	int activeWindows = 0;
	for (int i = 0; i < Core::MaxWindows; ++i)
	{
		std::string windowName = "Window" + std::to_string(i + 1);
		windows.push_back(CorePlatform.GetNewWindow(windowName.c_str(), 50, 50, 640, 480));
		if (windows[i])
		{
			++activeWindows;
		}
	}
	while (activeWindows != 0)
	{
		for (int i = 0; i < windows.size(); ++i)
		{
			if (windows[i])
			{
				windows[i]->PollMessages();
				if (windows[i]->ShouldClose())
				{
					CorePlatform.DeleteWindow(windows[i]);
					windows[i] = nullptr;
					--activeWindows;
				}
			}
		}
	}
	
	//=========================== Destroying Vulkan objects ==========================

	device.Destroy();
	if (enableVulkanDebug)
	{
		Debug::ShutdownInstanceValidationLayers(instance);
	}
	VulkanFactory::Instance::DestroyInstance(instance);

	return 0;
}