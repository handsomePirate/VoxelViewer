#include "Core/Common.hpp"
#include "Core/Platform/Platform.hpp"
#include "Core/Logger/Logger.hpp"
#include "Core/Events/EventSystem.hpp"
#include "Vulkan/VulkanFactory.hpp"
#include "Vulkan/Utils.hpp"
#include "Vulkan/Initializers.hpp"
#include "Debug/Debug.hpp"
#include "Vulkan/Render.hpp"
#include "ImGui/ImGui.hpp"
#include <imgui.h>
#include <vector>
#include <string>
#include <functional>
#include <chrono>

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

		CoreLogTrace("Button pressed; data = %s", keyName.c_str());
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

		CoreLogTrace("Mouse pressed at (%hu, %hu) with %s button",
			context.data.u16[0], context.data.u16[1], whichButtonString.c_str());
		return true;
	}

	bool WindowResized(Core::EventCode code, Core::EventData context)
	{
		CoreLogTrace("Window resized to (%hu, %hu)",
			context.data.u16[0], context.data.u16[1]);
		return true;
	}

	bool VulkanValidation(Core::EventCode code, Core::EventData context)
	{
		const VkDebugUtilsMessengerCallbackDataEXT* callbackData =
			(const VkDebugUtilsMessengerCallbackDataEXT*)context.data.u64[0];

		switch ((VkDebugUtilsMessageSeverityFlagBitsEXT)context.data.u32[2])
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			CoreLogTrace("%s", callbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			CoreLogInfo("%s", callbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			CoreLogWarn("%s", callbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			CoreLogError("%s", callbackData->pMessage);
			break;
		}

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
	}

	VkPhysicalDevice pickedDevice = VulkanUtils::Device::PickDevice(physicalDevices);
#pragma endregion

	//=========================== Creating logical device ============================

#pragma region Device features and extensions
	VkPhysicalDeviceFeatures requestedFeatures{};
	std::vector<const char*> deviceExtensions;
	if (useSwapchain)
	{
		deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	}
#pragma endregion

#pragma region Device
	VulkanFactory::Device::DeviceInfo deviceInfo;
	VulkanFactory::Device::Create(pickedDevice,requestedFeatures, deviceExtensions,
		deviceInfo, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
#pragma endregion

	//=========================== Window and swapchain setup =========================

#pragma region Window and surface
	uint32_t windowWidth = 1280;
	uint32_t windowHeight = 720;
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
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, renderTarget);

	auto subresourceRangeInitializer = VulkanInitializers::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
	VulkanUtils::Image::TransitionLayout(deviceInfo.Handle, renderTarget.Image, renderTarget.DescriptorImageInfo.imageLayout,
		VK_IMAGE_LAYOUT_GENERAL, subresourceRangeInitializer, graphicsCommandPool, graphicsQueue);
	renderTarget.DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
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
	
	VkPipeline graphicsPipeline = VulkanFactory::Pipeline::CreateGraphics(deviceInfo.Handle, renderPass,
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

#pragma region GUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	int guiTargetWidth, guiTargetHeight;
	unsigned char* fontData;
	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = 1.f;
	io.Fonts->GetTexDataAsRGBA32(&fontData, &guiTargetWidth, &guiTargetHeight);
	
	// Font image attachment creation.
	VulkanFactory::Image::ImageInfo2 guiFontAttachment;
	VulkanFactory::Image::Create(deviceInfo, guiTargetWidth, guiTargetHeight, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, guiFontAttachment);

	// Copy of the image to the GPU using a staging buffer
	VkDeviceSize uploadSize = guiTargetWidth * guiTargetHeight * 4 * sizeof(char);
	VulkanFactory::Buffer::BufferInfo stagingBufferInfo;
	VulkanFactory::Buffer::Create(deviceInfo, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, uploadSize,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBufferInfo);
	
	VulkanUtils::Buffer::Copy(deviceInfo.Handle, stagingBufferInfo.Memory, 0, uploadSize, fontData);

	auto guiSubresourceRangeInitializer = VulkanInitializers::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

	// TODO: Share command buffers.
	// Image layout transition for copy.
	VulkanUtils::Image::TransitionLayout(deviceInfo.Handle, guiFontAttachment.Image, guiFontAttachment.DescriptorImageInfo.imageLayout,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, guiSubresourceRangeInitializer, graphicsCommandPool, graphicsQueue,
		VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	guiFontAttachment.DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	// Image copy from staging buffer.
	VulkanUtils::Image::Copy(deviceInfo.Handle, stagingBufferInfo.DescriptorBufferInfo.buffer, guiFontAttachment.Image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, guiTargetWidth, guiTargetHeight, VK_IMAGE_ASPECT_COLOR_BIT,
		graphicsCommandPool, graphicsQueue);

	VulkanFactory::Buffer::Destroy(deviceInfo, stagingBufferInfo);

	// Image layout transition for shaders.
	VulkanUtils::Image::TransitionLayout(deviceInfo.Handle, guiFontAttachment.Image, guiFontAttachment.DescriptorImageInfo.imageLayout,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, guiSubresourceRangeInitializer, graphicsCommandPool, graphicsQueue,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	guiFontAttachment.DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	
	// Descriptors.
	std::vector<VkDescriptorPoolSize> guiDescriptorPoolSizes =
	{
		VulkanInitializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
	};
	VkDescriptorPool guiDescriptorPool = VulkanFactory::Descriptor::CreatePool(deviceInfo.Handle,
		guiDescriptorPoolSizes.data(), (uint32_t)guiDescriptorPoolSizes.size(), 2);
	
	VkDescriptorSetLayoutBinding guiLayoutBinding = VulkanInitializers::DescriptorSetLayoutBinding(
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
	VkDescriptorSetLayout guiDescriptorSetLayout = VulkanFactory::Descriptor::CreateSetLayout(deviceInfo.Handle,
		&guiLayoutBinding, 1);

	VkDescriptorSet guiDescriptorSet = VulkanFactory::Descriptor::AllocateSet(deviceInfo.Handle, guiDescriptorPool,
		guiDescriptorSetLayout);
	VulkanUtils::Descriptor::WriteImageSet(deviceInfo.Handle, guiDescriptorSet, &guiFontAttachment.DescriptorImageInfo);

	// Shaders.
	std::string guiVertexShaderPath = CoreFilesystem.GetAbsolutePath("ui.vert.glsl.spv");
	VkShaderModule guiVertexShader = VulkanFactory::Shader::Create(deviceInfo.Handle, guiVertexShaderPath);
	std::string guiFragmentShaderPath = CoreFilesystem.GetAbsolutePath("ui.frag.glsl.spv");
	VkShaderModule guiFragmentShader = VulkanFactory::Shader::Create(deviceInfo.Handle, guiFragmentShaderPath);

	// Pipeline.
	auto pushConstantRangeInitializer = VulkanInitializers::PushConstantRange(VK_SHADER_STAGE_VERTEX_BIT,
		sizeof(VulkanUtils::PushConstantBlock), 0);
	VkPipelineLayout guiPipelineLayout = VulkanFactory::Pipeline::CreateLayout(deviceInfo.Handle, guiDescriptorSetLayout,
		&pushConstantRangeInitializer, 1);

	VkPipeline guiPipeline = VulkanFactory::Pipeline::CreateGuiGraphics(deviceInfo.Handle, renderPass,
		guiVertexShader, guiFragmentShader, guiPipelineLayout, pipelineCache);

	VulkanFactory::Buffer::BufferInfo guiVertexBuffer{};
	VulkanFactory::Buffer::BufferInfo guiIndexBuffer{};
	VulkanUtils::PushConstantBlock pushConstantBlock{};
#pragma endregion

#pragma region Draw commands
	VulkanFactory::CommandBuffer::BuildData commandBufferBuildData{};
	commandBufferBuildData.Width = windowWidth;
	commandBufferBuildData.Height = windowHeight;
	commandBufferBuildData.RenderPass = renderPass;
	commandBufferBuildData.Target = renderTarget.Image;
	commandBufferBuildData.PipelineLayout = graphicsPipelineLayout;
	commandBufferBuildData.DescriptorSet = rasterizationSet;
	commandBufferBuildData.Pipeline = graphicsPipeline;

	VulkanFactory::CommandBuffer::GuiBuildData guiCommandBufferBuildData{};
	guiCommandBufferBuildData.PipelineLayout = guiPipelineLayout;
	guiCommandBufferBuildData.Pipeline = guiPipeline;
	guiCommandBufferBuildData.DescriptorSet = guiDescriptorSet;
	guiCommandBufferBuildData.PushConstantBlock = &pushConstantBlock;
	guiCommandBufferBuildData.VertexBuffer = &guiVertexBuffer;
	guiCommandBufferBuildData.IndexBuffer = &guiIndexBuffer;

	for (int i = 0; i < (int)drawCommandBuffers.size(); ++i)
	{
		commandBufferBuildData.Framebuffer = framebuffers[i];
		VulkanFactory::CommandBuffer::BuildDraw(drawCommandBuffers[i], commandBufferBuildData, guiCommandBufferBuildData);
	}
	
#pragma endregion

	//=========================== Rendering and message loop =========================

#pragma region Render loop
	uint32_t currentImageIndex = 0;
	
	Context renderingContext;
	renderingContext.Device = deviceInfo.Handle;
	renderingContext.Queue = graphicsQueue;

	Context computeContext;
	computeContext.Device = deviceInfo.Handle;
	computeContext.Queue = computeQueue;

	WindowData windowData;
	windowData.Swapchain = swapchainInfo.Handle;
	windowData.PresentSemaphore = presentSemaphore;
	windowData.RenderSemaphore = renderSemaphore;

	FrameData renderingFrameData;
	FrameData computeFrameData{};

	int frameCounterPerSecond = 0;
	auto last = std::chrono::high_resolution_clock::now();
	float fps = 0.f;

	while (!window->ShouldClose())
	{
		window->PollMessages();

		if (!window->IsMinimized())
		{
			auto before = std::chrono::high_resolution_clock::now();

			currentImageIndex = VulkanRender::PrepareFrame(renderingContext, windowData);

			renderingFrameData.CommandBuffer = drawCommandBuffers[currentImageIndex];
			renderingFrameData.Fence = fences[currentImageIndex];
			renderingFrameData.ImageIndex = currentImageIndex;

			VulkanRender::RenderFrame(renderingContext, windowData, renderingFrameData);

			computeFrameData.CommandBuffer = computeCommandBuffer;
			computeFrameData.Fence = computeFence;

			VulkanRender::ComputeFrame(computeContext, computeFrameData);

			// TODO: Update UBOs when there are some.

			auto now = std::chrono::high_resolution_clock::now();
			auto renderDelta = std::chrono::duration<float, std::milli>(now - before).count();

			auto millisecondCount = std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count();
			if (millisecondCount > 1000)
			{
				fps = frameCounterPerSecond * (1000.f / (float)millisecondCount);
				frameCounterPerSecond = 0;
				last = now;
			}

			++frameCounterPerSecond;

			bool updated = GUI::Renderer::Update(deviceInfo, guiVertexBuffer, guiIndexBuffer,
				(float)windowWidth, (float)windowHeight, renderDelta, fps);
			if (updated)
			{
				CoreLogTrace("Updating command buffers. Fps == %f", fps);
				for (int i = 0; i < (int)drawCommandBuffers.size(); ++i)
				{
					commandBufferBuildData.Framebuffer = framebuffers[i];
					VulkanFactory::CommandBuffer::BuildDraw(drawCommandBuffers[i], commandBufferBuildData, guiCommandBufferBuildData);
				}
			}
		}
	}
	vkDeviceWaitIdle(deviceInfo.Handle);
#pragma endregion
	
	//=========================== Destroying Vulkan objects ==========================

#pragma region GUI Vulkan deactivation
	VulkanFactory::Buffer::Destroy(deviceInfo, guiVertexBuffer);
	VulkanFactory::Buffer::Destroy(deviceInfo, guiIndexBuffer);

	VulkanFactory::Pipeline::Destroy(deviceInfo.Handle, guiPipeline);

	VulkanFactory::Pipeline::DestroyLayout(deviceInfo.Handle, guiPipelineLayout);

	VulkanFactory::Shader::Destroy(deviceInfo.Handle, guiFragmentShader);
	VulkanFactory::Shader::Destroy(deviceInfo.Handle, guiVertexShader);

	VulkanFactory::Descriptor::DestroySetLayout(deviceInfo.Handle, guiDescriptorSetLayout);
	VulkanFactory::Descriptor::DestroyPool(deviceInfo.Handle, guiDescriptorPool);
	
	VulkanFactory::Image::Destroy(deviceInfo.Handle, guiFontAttachment);
#pragma endregion

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