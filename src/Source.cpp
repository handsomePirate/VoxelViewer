#include "Common.hpp"
#include "Platform/Platform.hpp"
#include "Logger/Logger.hpp"
#include "Events/EventSystem.hpp"
#include "Vulkan/VulkanFactory.hpp"
#include "Vulkan/Utils.hpp"
#include "Vulkan/Initializers.hpp"
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
		Core::LoggerSeverity severity = Core::LoggerSeverity::Debug;
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
		assert(Debug::ValidationLayers::CheckLayerPresent(validationLayer));
	}

	VulkanUtils::Instance::CheckExtensionsPresent(vulkanExtensions);

	VkInstance instance = VulkanFactory::Instance::Create(
		vulkanExtensions, VK_MAKE_VERSION(1, 2, 0), validationLayer);

	if (enableVulkanDebug)
	{
		Debug::ValidationLayers::Start(instance);
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

	//=========================== Creating logical device ============================

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

	VulkanFactory::Device::DeviceInfo deviceInfo;
	VulkanFactory::Device::Create(pickedDevice,requestedFeatures, deviceExtensions,
		deviceInfo, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, debugMarkersEnabled);

	CoreLogger.Log(Core::LoggerSeverity::Debug, "Picked %s", deviceInfo.Properties.deviceName);

	if (enableVulkanDebug && debugMarkersEnabled)
	{
		Debug::Markers::Setup(deviceInfo.Handle);
	}

	VkCommandPool graphicsCommandPool = VulkanFactory::CommandPool::Create(
		deviceInfo.Handle, deviceInfo.QueueFamilyIndices.graphics,
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	VkSemaphore renderSemaphore = VulkanFactory::Semaphore::Create(deviceInfo.Handle);
	VkSemaphore presentSemaphore = VulkanFactory::Semaphore::Create(deviceInfo.Handle);

	VkPipelineStageFlags pipelineStageWait = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	auto submitInfo = VulkanInitializers::SubmitInfo();
	submitInfo.pWaitDstStageMask = &pipelineStageWait;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &presentSemaphore;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &renderSemaphore;

	//=========================== Window and swapchain setup =========================

	uint32_t windowWidth = 640;
	uint32_t windowHeight = 480;
	Core::Window* window = CorePlatform.GetNewWindow("VoxelViewer", 50, 50, windowWidth, windowHeight);
	VkSurfaceKHR surface = VulkanFactory::Surface::Create(instance, window->GetHandle());

	deviceInfo.SurfaceFormat = VulkanUtils::Surface::QueryFormat(pickedDevice, surface);
	deviceInfo.SurfaceCapabilities = VulkanUtils::Surface::QueryCapabilities(pickedDevice, surface);
	deviceInfo.SurfaceTransform = VulkanUtils::Surface::QueryTransform(deviceInfo.SurfaceCapabilities);
	deviceInfo.CompositeAlpha = VulkanUtils::Swapchain::QueryCompositeAlpha(deviceInfo.SurfaceCapabilities);

	deviceInfo.QueueFamilyIndices.present = VulkanUtils::Device::GetPresentQueueIndex(
		pickedDevice, surface, deviceInfo.QueueFamilyIndices.graphics);

	deviceInfo.PresentMode = VulkanUtils::Swapchain::QueryPresentMode(pickedDevice, surface);

	// TODO: This is not ideal, but Sascha Willems examples assume the same thing (tested on a lot of machines,
	// therefore it is probably okay). SW examples also are fine with graphics and compute queue being the same
	// one without getting a second one from that family.
	assert(deviceInfo.QueueFamilyIndices.graphics == deviceInfo.QueueFamilyIndices.present);

	VkQueue graphicsQueue = VulkanFactory::Queue::Get(
		deviceInfo.Handle, deviceInfo.QueueFamilyIndices.graphics);

	VkQueue computeQueue = VulkanFactory::Queue::Get(
		deviceInfo.Handle, deviceInfo.QueueFamilyIndices.compute);

	VulkanFactory::Swapchain::SwapchainInfo swapchainInfo;
	VulkanFactory::Swapchain::Create(windowWidth, windowHeight, surface, deviceInfo, swapchainInfo);

	//=========================== Rendering structures ===============================

	std::vector<VkCommandBuffer> drawCommandBuffers;
	VulkanFactory::CommandBuffer::AllocatePrimary(
		deviceInfo.Handle, graphicsCommandPool, drawCommandBuffers, swapchainInfo.Images.size());

	std::vector<VkFence> fences;
	fences.resize(swapchainInfo.Images.size());
	for (auto& fence : fences)
	{
		fence = VulkanFactory::Fence::Create(deviceInfo.Handle, VK_FENCE_CREATE_SIGNALED_BIT);
	}

	// TODO: Depth & Stencil.

	// TODO: Render pass.

	// TODO: Pipeline cache.

	// TODO: Framebuffer.
	
	// TODO: UI.

	//=========================== Shaders and rendering structures ===================

	// TODO: Shaders.

	// TODO: Storage & Uniform buffers.

	// TODO: Texture target.

	// TODO: Descriptors.

	// TODO: Pipelines.

	// TODO: Compute functionality.

	// TODO: Build command buffers.

	//=========================== Rendering and message loop =========================

	while (!window->ShouldClose())
	{
		window->PollMessages();
	}
	
	//=========================== Destroying Vulkan objects ==========================

	VulkanFactory::Swapchain::Destroy(deviceInfo, swapchainInfo);

	VulkanFactory::Surface::Destroy(instance, surface);
	CorePlatform.DeleteWindow(window);

	VulkanFactory::Semaphore::Destroy(deviceInfo.Handle, presentSemaphore);
	VulkanFactory::Semaphore::Destroy(deviceInfo.Handle, renderSemaphore);

	VulkanFactory::CommandPool::Destroy(deviceInfo.Handle, graphicsCommandPool);

	VulkanFactory::Device::Destroy(deviceInfo);
	if (enableVulkanDebug)
	{
		Debug::ValidationLayers::Shutdown(instance);
	}
	VulkanFactory::Instance::Destroy(instance);

	return 0;
}