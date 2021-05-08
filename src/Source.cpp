#include "Common.hpp"
#include "Platform/Platform.hpp"
#include "Logger/Logger.hpp"
#include "Events/EventSystem.hpp"
#include "Vulkan/VulkanFactory.hpp"
#include "Vulkan/Utils.hpp"
#include "Vulkan/Initializers.hpp"
#include "Debug/Debug.hpp"
#include "Vulkan/Render.hpp"
#include <vector>
#include <string>
#include <functional>

#include <iostream>

#pragma region Event logger
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
#pragma endregion

int main(int argc, char* argv[])
{
#pragma region Program options
	const bool enableVulkanDebug = true;
	const bool useSwapchain = true;
#pragma endregion

	//=========================== Testing event logging ==============================

#pragma region Event handling subscriptions
	EventLogger eventLogger;

	auto keyPressFnc = std::bind(&EventLogger::KeyPressed, &eventLogger, std::placeholders::_1, std::placeholders::_2);
	CoreEventSystem.SubscribeToEvent(Core::EventCode::KeyPressed, keyPressFnc, &eventLogger);
	auto mousePressFnc = std::bind(&EventLogger::MousePressed, &eventLogger, std::placeholders::_1, std::placeholders::_2);
	CoreEventSystem.SubscribeToEvent(Core::EventCode::MouseButtonPressed, mousePressFnc, &eventLogger);
	auto resizeFnc = std::bind(&EventLogger::WindowResized, &eventLogger, std::placeholders::_1, std::placeholders::_2);
	CoreEventSystem.SubscribeToEvent(Core::EventCode::WindowResized, resizeFnc, &eventLogger);
	auto vulkanValidationFnc = std::bind(&EventLogger::VulkanValidation, &eventLogger, std::placeholders::_1, std::placeholders::_2);
	CoreEventSystem.SubscribeToEvent(Core::EventCode::VulkanValidation, vulkanValidationFnc, &eventLogger);
#pragma endregion

	//=========================== Setting up Vulkan Instance =========================

#pragma region Instance extensions and layers
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
#pragma endregion

#pragma region Instance
	VulkanUtils::Instance::CheckExtensionsPresent(vulkanExtensions);

	VkInstance instance = VulkanFactory::Instance::Create(
		vulkanExtensions, VK_MAKE_VERSION(1, 2, 0), validationLayer);
#pragma endregion

#pragma region Validation layers
	if (enableVulkanDebug)
	{
		Debug::ValidationLayers::Start(instance);
	}
#pragma endregion

	//=========================== Enumerating and selecting physical devices =========

#pragma region Physical device selection
	std::vector<VkPhysicalDevice> physicalDevices;
	physicalDevices = VulkanUtils::Device::EnumeratePhysicalDevices(instance);

	for (int d = 0; d < physicalDevices.size(); ++d)
	{
		auto deviceProperties = VulkanUtils::Device::GetPhysicalDeviceProperties(physicalDevices[d]);

		CoreLogger.Log(Core::LoggerSeverity::Debug, "Found device (%i): %s", d, deviceProperties.deviceName);
	}

	VkPhysicalDevice pickedDevice = VulkanUtils::Device::PickDevice(physicalDevices);
#pragma endregion

	//=========================== Creating logical device ============================

#pragma region Device features and extensions
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
#pragma endregion

#pragma region Device
	VulkanFactory::Device::DeviceInfo deviceInfo;
	VulkanFactory::Device::Create(pickedDevice,requestedFeatures, deviceExtensions,
		deviceInfo, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, debugMarkersEnabled);

	CoreLogger.Log(Core::LoggerSeverity::Debug, "Picked %s", deviceInfo.Properties.deviceName);
#pragma endregion

#pragma region Debug markers
	if (enableVulkanDebug && debugMarkersEnabled)
	{
		Debug::Markers::Setup(deviceInfo.Handle);
	}
#pragma endregion

	//=========================== Window and swapchain setup =========================

#pragma region Window and surface
	uint32_t windowWidth = 640;
	uint32_t windowHeight = 480;
	Core::Window* window = CorePlatform.GetNewWindow(CoreFilesystem.ExecutableName().c_str(), 50, 50, windowWidth, windowHeight);
	VkSurfaceKHR surface = VulkanFactory::Surface::Create(instance, window->GetHandle());
#pragma endregion

#pragma region Device info completion
	deviceInfo.SurfaceFormat = VulkanUtils::Surface::QueryFormat(pickedDevice, surface);
	deviceInfo.SurfaceCapabilities = VulkanUtils::Surface::QueryCapabilities(pickedDevice, surface);
	deviceInfo.SurfaceTransform = VulkanUtils::Surface::QueryTransform(deviceInfo.SurfaceCapabilities);
	deviceInfo.CompositeAlpha = VulkanUtils::Swapchain::QueryCompositeAlpha(deviceInfo.SurfaceCapabilities);

	deviceInfo.QueueFamilyIndices.present = VulkanUtils::Device::GetPresentQueueIndex(
		pickedDevice, surface, deviceInfo.QueueFamilyIndices.graphics);

	// TODO: This is not ideal, but Sascha Willems examples assume the same thing (tested on a lot of machines,
	// therefore it is probably okay). SW examples also are fine with graphics and compute queue being the same
	// one without getting a second one from that family.
	assert(deviceInfo.QueueFamilyIndices.graphics == deviceInfo.QueueFamilyIndices.present);

	deviceInfo.PresentMode = VulkanUtils::Swapchain::QueryPresentMode(pickedDevice, surface);
#pragma endregion

#pragma region Swapchain
	VulkanFactory::Swapchain::SwapchainInfo swapchainInfo;
	VulkanFactory::Swapchain::Create(deviceInfo, windowWidth, windowHeight, surface, swapchainInfo);
#pragma endregion

	//=========================== Rendering structures ===============================

#pragma region Command buffers
	VkCommandPool graphicsCommandPool = VulkanFactory::CommandPool::Create(
		deviceInfo.Handle, deviceInfo.QueueFamilyIndices.graphics,
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	std::vector<VkCommandBuffer> drawCommandBuffers;
	VulkanFactory::CommandBuffer::AllocatePrimary(deviceInfo.Handle, graphicsCommandPool,
		drawCommandBuffers, (uint32_t)swapchainInfo.Images.size());

	VkCommandPool computeCommandPool = VulkanFactory::CommandPool::Create(
		deviceInfo.Handle, deviceInfo.QueueFamilyIndices.compute,
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	VkCommandBuffer computeCommandBuffer = VulkanFactory::CommandBuffer::AllocatePrimary(
		deviceInfo.Handle, computeCommandPool);
#pragma endregion

#pragma region Queues
	VkQueue graphicsQueue = VulkanFactory::Queue::Get(
		deviceInfo.Handle, deviceInfo.QueueFamilyIndices.graphics);

	VkQueue computeQueue = VulkanFactory::Queue::Get(
		deviceInfo.Handle, deviceInfo.QueueFamilyIndices.compute);
#pragma endregion

#pragma region Synchronization primitives
	VkSemaphore renderSemaphore = VulkanFactory::Semaphore::Create(deviceInfo.Handle);
	VkSemaphore presentSemaphore = VulkanFactory::Semaphore::Create(deviceInfo.Handle);
	std::vector<VkFence> fences;
	fences.resize(swapchainInfo.Images.size());
	for (auto& fence : fences)
	{
		fence = VulkanFactory::Fence::Create(deviceInfo.Handle, VK_FENCE_CREATE_SIGNALED_BIT);
	}

	VkFence computeFence = VulkanFactory::Fence::Create(deviceInfo.Handle, VK_FENCE_CREATE_SIGNALED_BIT);

	VkPipelineStageFlags pipelineStageWait = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	auto submitInfo = VulkanInitializers::SubmitInfo();
	submitInfo.pWaitDstStageMask = &pipelineStageWait;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &presentSemaphore;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &renderSemaphore;
#pragma endregion

#pragma region Framebuffer
	VulkanFactory::Image::ImageInfo depthStencil;
	VulkanFactory::Image::Create(deviceInfo, windowWidth, windowHeight, deviceInfo.DepthFormat, depthStencil);

	VkRenderPass renderPass = VulkanFactory::RenderPass::Create(deviceInfo.Handle,
		deviceInfo.SurfaceFormat.format, deviceInfo.DepthFormat);

	std::vector<VkFramebuffer> framebuffers;
	framebuffers.resize(swapchainInfo.Images.size());
	for (int f = 0; f < framebuffers.size(); ++f)
	{
		framebuffers[f] = VulkanFactory::Framebuffer::Create(deviceInfo.Handle, renderPass,
			windowWidth, windowHeight, swapchainInfo.ImageViews[f], depthStencil.View);
	}
#pragma endregion
	
	// TODO: UI.

	//=========================== Shaders and rendering structures ===================

#pragma region Shaders
	std::string vertexShaderPath = CoreFilesystem.GetAbsolutePath("simple.vert.glsl.spv");
	VkShaderModule vertexShader = VulkanFactory::Shader::Create(deviceInfo.Handle, vertexShaderPath);
	std::string fragmentShaderPath = CoreFilesystem.GetAbsolutePath("simple.frag.glsl.spv");
	VkShaderModule fragmentShader = VulkanFactory::Shader::Create(deviceInfo.Handle, fragmentShaderPath);
	std::string computeShaderPath = CoreFilesystem.GetAbsolutePath("simple.comp.glsl.spv");
	VkShaderModule computeShader = VulkanFactory::Shader::Create(deviceInfo.Handle, computeShaderPath);
#pragma endregion

	// TODO: Storage & Uniform buffers.


#pragma region Texture target
	const int targetWidth = 2048;
	const int targetHeight = 2048;
	VulkanFactory::Image::ImageInfo2 renderTarget;
	VulkanFactory::Image::Create(deviceInfo, targetWidth, targetHeight, VK_FORMAT_R8G8B8A8_UNORM,
		graphicsCommandPool, graphicsQueue, renderTarget);
#pragma endregion

#pragma region Descriptors
	std::vector<VkDescriptorPoolSize> descriptorPoolSizes =
	{
		VulkanInitializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1),
		VulkanInitializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1),
		VulkanInitializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1),
	};
	VkDescriptorPool descriptorPool = VulkanFactory::Descriptor::CreatePool(deviceInfo.Handle,
		descriptorPoolSizes.data(), (uint32_t)descriptorPoolSizes.size(), 3);

	VkDescriptorSetLayoutBinding rasterizationLayoutBinding = VulkanInitializers::DescriptorSetLayoutBinding(
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
	VkDescriptorSetLayout rasterizationSetLayout = VulkanFactory::Descriptor::CreateSetLayout(deviceInfo.Handle,
		&rasterizationLayoutBinding, 1);
	
	VkDescriptorSet rasterizationSet = VulkanFactory::Descriptor::AllocateSet(deviceInfo.Handle, descriptorPool,
		rasterizationSetLayout);
	VulkanUtils::Descriptor::WriteImageSet(deviceInfo.Handle, rasterizationSet, &renderTarget.DescriptorImageInfo);

	std::vector<VkDescriptorSetLayoutBinding> computeLayoutBindings =
	{
		VulkanInitializers::DescriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0),
		//VulkanInitializers::DescriptorSetLayoutBinding(
		//	VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1),
	};
	VkDescriptorSetLayout computeSetLayout = VulkanFactory::Descriptor::CreateSetLayout(deviceInfo.Handle,
		computeLayoutBindings.data(), (uint32_t)computeLayoutBindings.size());

	VkDescriptorSet computeSet = VulkanFactory::Descriptor::AllocateSet(deviceInfo.Handle, descriptorPool,
		computeSetLayout);
	VulkanUtils::Descriptor::WriteComputeSet(deviceInfo.Handle, computeSet, &renderTarget.DescriptorImageInfo/*, storageBufferInfo*/);
#pragma endregion

#pragma region Pipelines
	VkPipelineLayout graphicsPipelineLayout = VulkanFactory::Pipeline::CreateLayout(deviceInfo.Handle, rasterizationSetLayout);
	VkPipelineLayout computePipelineLayout = VulkanFactory::Pipeline::CreateLayout(deviceInfo.Handle, computeSetLayout);

	VkPipelineCache pipelineCache = VulkanFactory::Pipeline::CreateCache(deviceInfo.Handle);
	
	VkPipeline graphicsPipeline = VulkanFactory::Pipeline::CreateGraphics(deviceInfo.Handle, renderPass, swapchainInfo,
		vertexShader, fragmentShader, graphicsPipelineLayout, pipelineCache);
	VkPipeline computePipeline = VulkanFactory::Pipeline::CreateCompute(deviceInfo.Handle, computePipelineLayout,
		computeShader, pipelineCache);
#pragma endregion

#pragma region Compute commands
	VulkanUtils::CommandBuffer::Begin(computeCommandBuffer);

	vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
	vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeSet, 0, 0);

	vkCmdDispatch(computeCommandBuffer, targetWidth / 16, targetHeight / 16, 1);

	VulkanUtils::CommandBuffer::End(computeCommandBuffer);
#pragma endregion

#pragma region Draw commands
	VkClearColorValue clearColor;
	clearColor.float32[0] = 1.f;
	clearColor.float32[1] = 0.f;
	clearColor.float32[2] = 0.f;
	clearColor.float32[3] = 1.f;

	VkClearValue clearValues[2];
	clearValues[0].color = clearColor;
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = VulkanInitializers::RenderPassBeginning(renderPass, windowWidth, windowHeight);
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;

	for (int32_t i = 0; i < drawCommandBuffers.size(); ++i)
	{
		// Set target frame buffer
		renderPassBeginInfo.framebuffer = framebuffers[i];

		VulkanUtils::CommandBuffer::Begin(drawCommandBuffers[i]);

		// Image memory barrier to make sure that compute shader writes are finished before sampling from the texture
		VkImageMemoryBarrier imageMemoryBarrier = {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageMemoryBarrier.image = renderTarget.Image;
		imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		vkCmdPipelineBarrier(drawCommandBuffers[i],
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

		vkCmdBeginRenderPass(drawCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = VulkanUtils::Pipeline::CreateViewport(windowWidth, windowWidth, 0.0f, 1.0f);
		vkCmdSetViewport(drawCommandBuffers[i], 0, 1, &viewport);

		VkRect2D scissor = VulkanUtils::Pipeline::CreateScissor(swapchainInfo.Extent);
		vkCmdSetScissor(drawCommandBuffers[i], 0, 1, &scissor);

		// Display ray traced image generated by compute shader as a full screen quad
		// Quad vertices are generated in the vertex shader
		vkCmdBindDescriptorSets(drawCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout,
			0, 1, &rasterizationSet, 0, NULL);
		vkCmdBindPipeline(drawCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
		vkCmdDraw(drawCommandBuffers[i], 3, 1, 0, 0);

		//drawUI(drawCommandBuffers[i]);

		vkCmdEndRenderPass(drawCommandBuffers[i]);

		VulkanUtils::CommandBuffer::End(drawCommandBuffers[i]);
	}
#pragma endregion

	//=========================== Rendering and message loop =========================

#pragma region Render loop
	uint32_t currentBuffer = 0;
	while (!window->ShouldClose())
	{
		VulkanRender::PrepareFrame(deviceInfo.Handle, swapchainInfo.Handle, presentSemaphore, currentBuffer);

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCommandBuffers[currentBuffer];
		VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		assert(result == VK_SUCCESS);

		VulkanRender::SubmitFrame(graphicsQueue, swapchainInfo.Handle, renderSemaphore, currentBuffer);

		vkWaitForFences(deviceInfo.Handle, 1, &computeFence, VK_TRUE, UINT64_MAX);
		vkResetFences(deviceInfo.Handle, 1, &computeFence);

		VkSubmitInfo computeSubmitInfo = VulkanInitializers::SubmitInfo();
		computeSubmitInfo.commandBufferCount = 1;
		computeSubmitInfo.pCommandBuffers = &computeCommandBuffer;

		result = vkQueueSubmit(computeQueue, 1, &computeSubmitInfo, computeFence);
		assert(result == VK_SUCCESS);

		window->PollMessages();

		currentBuffer = (currentBuffer + 1) % (uint32_t)drawCommandBuffers.size();
	}
	vkDeviceWaitIdle(deviceInfo.Handle);
#pragma endregion
	
	//=========================== Destroying Vulkan objects ==========================

#pragma region Vulkan deactivation

	VulkanFactory::Pipeline::Destroy(deviceInfo.Handle, computePipeline);
	VulkanFactory::Pipeline::Destroy(deviceInfo.Handle, graphicsPipeline);
	
	VulkanFactory::Pipeline::DestroyCache(deviceInfo.Handle, pipelineCache);

	VulkanFactory::Pipeline::DestroyLayout(deviceInfo.Handle, computePipelineLayout);
	VulkanFactory::Pipeline::DestroyLayout(deviceInfo.Handle, graphicsPipelineLayout);

	VulkanFactory::Descriptor::DestroySetLayout(deviceInfo.Handle, computeSetLayout);
	VulkanFactory::Descriptor::DestroySetLayout(deviceInfo.Handle, rasterizationSetLayout);
	VulkanFactory::Descriptor::DestroyPool(deviceInfo.Handle, descriptorPool);

	VulkanFactory::Image::Destroy(deviceInfo.Handle, renderTarget);

	VulkanFactory::Shader::Destroy(deviceInfo.Handle, computeShader);
	VulkanFactory::Shader::Destroy(deviceInfo.Handle, fragmentShader);
	VulkanFactory::Shader::Destroy(deviceInfo.Handle, vertexShader);

	for (auto& framebuffer : framebuffers)
	{
		VulkanFactory::Framebuffer::Destroy(deviceInfo.Handle, framebuffer);
	}
	
	VulkanFactory::RenderPass::Destroy(deviceInfo.Handle, renderPass);
	
	VulkanFactory::Image::Destroy(deviceInfo.Handle, depthStencil);

	VulkanFactory::Fence::Destroy(deviceInfo.Handle, computeFence);
	for (auto& fence : fences)
	{
		VulkanFactory::Fence::Destroy(deviceInfo.Handle, fence);
	}

	VulkanFactory::Swapchain::Destroy(deviceInfo, swapchainInfo);

	VulkanFactory::Surface::Destroy(instance, surface);
	CorePlatform.DeleteWindow(window);

	VulkanFactory::Semaphore::Destroy(deviceInfo.Handle, presentSemaphore);
	VulkanFactory::Semaphore::Destroy(deviceInfo.Handle, renderSemaphore);

	VulkanFactory::CommandBuffer::Free(deviceInfo.Handle, computeCommandPool, computeCommandBuffer);
	VulkanFactory::CommandPool::Destroy(deviceInfo.Handle, computeCommandPool);

	VulkanFactory::CommandBuffer::Free(deviceInfo.Handle, graphicsCommandPool, drawCommandBuffers);
	VulkanFactory::CommandPool::Destroy(deviceInfo.Handle, graphicsCommandPool);

	VulkanFactory::Device::Destroy(deviceInfo);
	if (enableVulkanDebug)
	{
		Debug::ValidationLayers::Shutdown(instance);
	}
	VulkanFactory::Instance::Destroy(instance);
#pragma endregion

	return 0;
}