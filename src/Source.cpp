#include "Core/Common.hpp"
#include "Core/Platform/Platform.hpp"
#include "Core/Logger/Logger.hpp"
#include "Core/Events/EventSystem.hpp"
#include "Vulkan/VulkanFactory.hpp"
#include "Vulkan/Utils.hpp"
#include "Vulkan/Initializers.hpp"
#include "Debug/Debug.hpp"
#include "Vulkan/Render.hpp"
#include "Core/Input/Input.hpp"
#include "ImGui/ImGui.hpp"
#include "Vulkan/ShaderCompiler.hpp"
#include "Vulkan/ShaderManager.hpp"
#include "HashDAG/OpenVDBUtils.hpp"
#include "HashDAG/HashDAG.hpp"
#include "HashDAG/Converter.hpp"
#include <imgui.h>
#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include <thread>

#include <iostream>

#pragma region Event logger
struct EventLogger
{
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
	const bool enableVulkanDebug = false;
#pragma endregion

#pragma region Singleton initialization
	volatile auto input = CoreInput;
	CoreLogger.SetTypes(Core::LoggerType::Both);
#pragma endregion
	
	//=========================== Testing event logging ==============================

#pragma region Event handling subscriptions
	EventLogger eventLogger;

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

#pragma region Validation layers and Debug utils
	if (enableVulkanDebug)
	{
		Debug::ValidationLayers::Start(instance);
		Debug::Utils::Start(instance);
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
	deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#pragma endregion

#pragma region Device
	VulkanFactory::Device::DeviceInfo deviceInfo;
	VulkanFactory::Device::Create(pickedDevice, requestedFeatures, deviceExtensions,
		deviceInfo, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);

	Debug::Utils::SetPhysicalDeviceName(deviceInfo.Handle, pickedDevice, deviceInfo.Properties.deviceName);
#pragma endregion

	//=========================== Window and swapchain setup =========================

#pragma region Window and surface
	uint32_t windowWidth = 1280;
	uint32_t windowHeight = 720;
	Core::Window* window = CorePlatform.GetNewWindow(CoreFilesystem.ExecutableName().c_str(), 50, 50,
		windowWidth, windowHeight, false);
	VkSurfaceKHR surface = VulkanFactory::Surface::Create("Win32 Window Surface", deviceInfo.Handle, instance, window->GetHandle());
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
	VulkanFactory::Swapchain::Create("VV Swapchain", deviceInfo, windowWidth, windowHeight, surface, swapchainInfo);
#pragma endregion

	//=========================== Rendering structures ===============================

#pragma region Command buffers
	VkCommandPool graphicsCommandPool = VulkanFactory::CommandPool::Create(
		"Graphics Command Pool", deviceInfo.Handle, deviceInfo.QueueFamilyIndices.graphics,
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	std::vector<VkCommandBuffer> drawCommandBuffers;
	drawCommandBuffers.resize(swapchainInfo.Images.size());
	VulkanFactory::CommandBuffer::AllocatePrimary(deviceInfo.Handle, graphicsCommandPool,
		drawCommandBuffers.data(), (uint32_t)drawCommandBuffers.size());
	for (int c = 0; c < drawCommandBuffers.size(); ++c)
	{
		assert(c < 1000);
		char commandBufferName[sizeof("Draw Command Buffer ") + 3 + sizeof(char)];
		sprintf_s(commandBufferName, "Draw Command Buffer %d", c);

		Debug::Utils::SetCommandBufferName(deviceInfo.Handle, drawCommandBuffers[c], commandBufferName);
	}

	VkCommandPool computeCommandPool = VulkanFactory::CommandPool::Create( "Compute Command Pool",
		deviceInfo.Handle, deviceInfo.QueueFamilyIndices.compute,
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	VkCommandBuffer computeCommandBuffer = VulkanFactory::CommandBuffer::AllocatePrimary("Compute Command Buffer",
		deviceInfo.Handle, computeCommandPool);
#pragma endregion

#pragma region Queues
	VkQueue graphicsQueue = VulkanFactory::Queue::Get("Graphics Queue",
		deviceInfo.Handle, deviceInfo.QueueFamilyIndices.graphics);

	VkQueue computeQueue = VulkanFactory::Queue::Get("Compute Queue",
		deviceInfo.Handle, deviceInfo.QueueFamilyIndices.compute);
#pragma endregion

#pragma region Synchronization primitives
	VkSemaphore renderSemaphore = VulkanFactory::Semaphore::Create("Render Semaphore", deviceInfo.Handle);
	VkSemaphore presentSemaphore = VulkanFactory::Semaphore::Create("Present Semaphore", deviceInfo.Handle);

	VkFence computeFence = VulkanFactory::Fence::Create("Compute Fence", deviceInfo.Handle, VK_FENCE_CREATE_SIGNALED_BIT);
#pragma endregion

#pragma region Framebuffer
	VulkanFactory::Image::ImageInfo depthStencil;
	VulkanFactory::Image::Create("Depth Stencil Image", deviceInfo, windowWidth, windowHeight, deviceInfo.DepthFormat, depthStencil);

	VkRenderPass renderPass = VulkanFactory::RenderPass::Create("VV Display Render Pass", deviceInfo.Handle,
		deviceInfo.SurfaceFormat.format, deviceInfo.DepthFormat);

	std::vector<VkFramebuffer> framebuffers;
	framebuffers.resize(swapchainInfo.Images.size());
	for (int f = 0; f < framebuffers.size(); ++f)
	{
		assert(f < 1000);
		char framebufferName[sizeof("VV Framebuffer ") + 3 + sizeof(char)];
		sprintf_s(framebufferName, "VV Framebuffer %d", f);

		framebuffers[f] = VulkanFactory::Framebuffer::Create(framebufferName, deviceInfo.Handle, renderPass,
			windowWidth, windowHeight, swapchainInfo.ImageViews[f], depthStencil.View);
	}
#pragma endregion

	//=========================== Setting up OpenVDB =================================

#pragma region OpenVDB init, grid loading, transformation to HashDAG
	openvdb::initialize();
	auto gridFile = CoreFilesystem.GetAbsolutePath("../../exampleData/example_task_0_solution.vdb");
	auto grid = OpenVDBUtils::LoadGrid(gridFile);
	auto hdStart = std::chrono::high_resolution_clock::now();
	HashDAG hd{};
	Converter::OpenVDBToDAG(grid, hd);
	auto hdEnd = std::chrono::high_resolution_clock::now();
	auto msCount = std::chrono::duration_cast<std::chrono::milliseconds>(hdEnd - hdStart).count();
	CoreLogInfo("Hash DAG created in %lld ms, (0, 0, 0) is %s", msCount, hd.IsActive({ 0, 0, 0 }) ? "active" : "not active");

	HashDAGGPUInfo uploadInfo;
	auto uploadStart = std::chrono::high_resolution_clock::now();
	hd.UploadToGPU(deviceInfo, graphicsCommandPool, graphicsQueue, uploadInfo);
	auto uploadEnd = std::chrono::high_resolution_clock::now();
	msCount = std::chrono::duration_cast<std::chrono::milliseconds>(uploadEnd - uploadStart).count();
	CoreLogInfo("Hash DAG uploaded in %lld ms", msCount);
#pragma endregion

	//=========================== Shaders and rendering structures ===================

#pragma region Shaders
	std::string vertexShaderPath = CoreFilesystem.GetAbsolutePath("../../src/Shaders/simple.vert.glsl");
	VkShaderModule vertexShader = Shader::Compiler::LoadShader(deviceInfo.Handle, vertexShaderPath);;
	std::string fragmentShaderPath = CoreFilesystem.GetAbsolutePath("../../src/Shaders/simple.frag.glsl");
	VkShaderModule fragmentShader = Shader::Compiler::LoadShader(deviceInfo.Handle, fragmentShaderPath);;
	std::string computeShaderPath = CoreFilesystem.GetAbsolutePath("../../src/Shaders/simple.comp.glsl");
	VkShaderModule computeShader = Shader::Compiler::LoadShader(deviceInfo.Handle, computeShaderPath);

	ShaderManager.AddShader(vertexShaderPath);
	ShaderManager.AddShader(fragmentShaderPath);
	ShaderManager.AddShader(computeShaderPath);
#pragma endregion

#pragma region Texture target
	const int targetWidth = windowWidth;
	const int targetHeight = windowHeight;
	VulkanFactory::Image::ImageInfo2 renderTarget;
	VulkanFactory::Image::Create("Compute Texture Target", deviceInfo, targetWidth, targetHeight, VK_FORMAT_R8G8B8A8_UNORM,
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
		VulkanInitializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3),
		VulkanInitializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
	};
	VkDescriptorPool descriptorPool = VulkanFactory::Descriptor::CreatePool("Compute and Display Descriptor Pool",
		deviceInfo.Handle, descriptorPoolSizes.data(), (uint32_t)descriptorPoolSizes.size(), 3);

	VkDescriptorSetLayoutBinding rasterizationLayoutBinding = VulkanInitializers::DescriptorSetLayoutBinding(
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
	VkDescriptorSetLayout rasterizationSetLayout = VulkanFactory::Descriptor::CreateSetLayout("Display Descriptor Set Layout",
		deviceInfo.Handle, &rasterizationLayoutBinding, 1);
	
	VkDescriptorSet rasterizationSet = VulkanFactory::Descriptor::AllocateSet(deviceInfo.Handle, descriptorPool,
		rasterizationSetLayout);
	Debug::Utils::SetDescriptorSetName(deviceInfo.Handle, rasterizationSet, "Display Descriptor Set");
	VulkanUtils::Descriptor::WriteImageSet(deviceInfo.Handle, rasterizationSet, &renderTarget.DescriptorImageInfo);

	std::vector<VkDescriptorSetLayoutBinding> computeLayoutBindings =
	{
		VulkanInitializers::DescriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0),
		VulkanInitializers::DescriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1),
		VulkanInitializers::DescriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 2),
		VulkanInitializers::DescriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 3),
		VulkanInitializers::DescriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 4),
	};
	VkDescriptorSetLayout computeSetLayout = VulkanFactory::Descriptor::CreateSetLayout("Compute Descriptor Set Layout",
		deviceInfo.Handle, computeLayoutBindings.data(), (uint32_t)computeLayoutBindings.size());

	VkDescriptorSet computeSet = VulkanFactory::Descriptor::AllocateSet(deviceInfo.Handle, descriptorPool,
		computeSetLayout);
	Debug::Utils::SetDescriptorSetName(deviceInfo.Handle, rasterizationSet, "Compute Descriptor Set");

	Camera camera({ 0, -512, 0 }, { 0, 1, 0 }, { 1, 0, 0 }, 35.f);

	TracingParameters tracingParameters;
	camera.GetTracingParameters(windowWidth, windowHeight, tracingParameters);
	tracingParameters.VoxelDetail = HTConstants::MAX_LEVEL_COUNT;
	tracingParameters.ColorScale = .01f;
	VulkanFactory::Buffer::BufferInfo tracingUniformBuffer;
	VulkanFactory::Buffer::Create("Tracing Uniform Buffer", deviceInfo, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(TracingParameters),
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, tracingUniformBuffer);
	VulkanUtils::Buffer::Copy(deviceInfo.Handle, tracingUniformBuffer.Memory, sizeof(TracingParameters), &tracingParameters);

	VkDescriptorBufferInfo storageBuffersDescriptorInfo[3] =
	{
		uploadInfo.PageTableStorageBuffer.DescriptorBufferInfo,
		uploadInfo.PagesStorageBuffer.DescriptorBufferInfo,
		uploadInfo.TreeRootsStorageBuffer.DescriptorBufferInfo
	};
	VulkanUtils::Descriptor::WriteComputeSet(deviceInfo.Handle, computeSet, 
		&renderTarget.DescriptorImageInfo, 1,
		storageBuffersDescriptorInfo, 3,
		&tracingUniformBuffer.DescriptorBufferInfo, 1);
#pragma endregion

#pragma region Pipelines
	VkPipelineLayout graphicsPipelineLayout = VulkanFactory::Pipeline::CreateLayout("Display Pipeline Layout",
		deviceInfo.Handle, rasterizationSetLayout);
	auto computeConstantRangeInitializer = VulkanInitializers::PushConstantRange(VK_SHADER_STAGE_COMPUTE_BIT,
		sizeof(HashDAGPushConstants), 0);
	VkPipelineLayout computePipelineLayout = VulkanFactory::Pipeline::CreateLayout("Trace Pipeline Layout",
		deviceInfo.Handle, computeSetLayout, &computeConstantRangeInitializer, 1);

	VkPipelineCache pipelineCache = VulkanFactory::Pipeline::CreateCache("VV General Pipeline Cache", deviceInfo.Handle);
	
	VkPipeline graphicsPipeline = VulkanFactory::Pipeline::CreateGraphics(deviceInfo.Handle, renderPass,
		vertexShader, fragmentShader, graphicsPipelineLayout, pipelineCache);
	VkPipeline computePipeline = VulkanFactory::Pipeline::CreateCompute(deviceInfo.Handle, computePipelineLayout,
		computeShader, pipelineCache);
#pragma endregion

#pragma region Compute commands
	VulkanUtils::CommandBuffer::Begin(computeCommandBuffer);

	vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
	vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeSet, 0, 0);
	HashDAGPushConstants computePushConstants{};
	computePushConstants.PageCount = uploadInfo.PageCount;
	computePushConstants.TreeCount = uploadInfo.TreeCount;
	vkCmdPushConstants(computeCommandBuffer, computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
		sizeof(HashDAGPushConstants), &computePushConstants);

	vkCmdDispatch(computeCommandBuffer, targetWidth / 16, targetHeight / 16, 1);

	VulkanUtils::CommandBuffer::End(computeCommandBuffer);
#pragma endregion

#pragma region GUI
	IMGUI_CHECKVERSION();
	GUI::Renderer::Init(window->GetHandle());
	int guiTargetWidth, guiTargetHeight;
	unsigned char* fontData;
	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = 1.f;
	io.Fonts->GetTexDataAsRGBA32(&fontData, &guiTargetWidth, &guiTargetHeight);
	
	// Font image attachment creation.
	VulkanFactory::Image::ImageInfo2 guiFontAttachment;
	VulkanFactory::Image::Create("GUI Font Attachment", deviceInfo, guiTargetWidth, guiTargetHeight, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, guiFontAttachment);

	// Copy of the image to the GPU using a staging buffer
	VkDeviceSize uploadSize = guiTargetWidth * guiTargetHeight * 4 * sizeof(char);
	VulkanFactory::Buffer::BufferInfo stagingBufferInfo;
	VulkanFactory::Buffer::Create("GUI Font Image Staging Buffer", deviceInfo, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, uploadSize,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBufferInfo);
	
	VulkanUtils::Buffer::Copy(deviceInfo.Handle, stagingBufferInfo.Memory, uploadSize, fontData);

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
	VkDescriptorPool guiDescriptorPool = VulkanFactory::Descriptor::CreatePool("GUI Descriptor Pool", deviceInfo.Handle,
		guiDescriptorPoolSizes.data(), (uint32_t)guiDescriptorPoolSizes.size(), 2);
	
	VkDescriptorSetLayoutBinding guiLayoutBinding = VulkanInitializers::DescriptorSetLayoutBinding(
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
	VkDescriptorSetLayout guiDescriptorSetLayout = VulkanFactory::Descriptor::CreateSetLayout("GUI Descriptor Set Layout",
		deviceInfo.Handle, &guiLayoutBinding, 1);

	VkDescriptorSet guiDescriptorSet = VulkanFactory::Descriptor::AllocateSet(deviceInfo.Handle, guiDescriptorPool,
		guiDescriptorSetLayout);
	Debug::Utils::SetDescriptorSetName(deviceInfo.Handle, guiDescriptorSet, "GUI Descriptor Set");
	VulkanUtils::Descriptor::WriteImageSet(deviceInfo.Handle, guiDescriptorSet, &guiFontAttachment.DescriptorImageInfo);

	// Shaders.
	std::string guiVertexShaderPath = CoreFilesystem.GetAbsolutePath("../../src/Shaders/ui.vert.glsl");
	VkShaderModule guiVertexShader = Shader::Compiler::LoadShader(deviceInfo.Handle, guiVertexShaderPath);
	std::string guiFragmentShaderPath = CoreFilesystem.GetAbsolutePath("../../src/Shaders/ui.frag.glsl");
	VkShaderModule guiFragmentShader = Shader::Compiler::LoadShader(deviceInfo.Handle, guiFragmentShaderPath);

	ShaderManager.AddShader(guiVertexShaderPath);
	ShaderManager.AddShader(guiFragmentShaderPath);

	// Pipeline.
	auto pushConstantRangeInitializer = VulkanInitializers::PushConstantRange(VK_SHADER_STAGE_VERTEX_BIT,
		sizeof(VulkanUtils::PushConstantBlock), 0);
	VkPipelineLayout guiPipelineLayout = VulkanFactory::Pipeline::CreateLayout("GUI Pipeline Layout", deviceInfo.Handle,
		guiDescriptorSetLayout, &pushConstantRangeInitializer, 1);

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

	uint16_t lastMouseX = 0;
	uint16_t lastMouseY = 0;

	while (!window->ShouldClose())
	{
		window->PollMessages();

		if (!window->IsMinimized())
		{
			auto before = std::chrono::high_resolution_clock::now();

			bool shouldResize = false;
			currentImageIndex = VulkanRender::PrepareFrame(renderingContext, windowData, shouldResize);

			if (!shouldResize)
			{
				renderingFrameData.CommandBuffer = drawCommandBuffers[currentImageIndex];
				renderingFrameData.ImageIndex = currentImageIndex;

				VulkanRender::RenderFrame(renderingContext, windowData, renderingFrameData, shouldResize);
			}

			if (!shouldResize)
			{
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

				const uint16_t mouseX = CoreInput.GetMouseX();
				const uint16_t mouseY = CoreInput.GetMouseY();
				const bool isMousePressed = CoreInput.IsMouseButtonPressed(Core::Input::MouseButtons::Left);

				const float timeDelta = renderDelta * .001f;

				if (isMousePressed && !GUI::Renderer::WantMouseCapture())
				{
					const int deltaX = int(lastMouseX) - int(mouseX);
					const int deltaY = int(lastMouseY) - int(mouseY);

					const float sensitivity = .1f;
					const float xMove = sensitivity * deltaX * timeDelta;
					const float yMove = sensitivity * deltaY * timeDelta;

					camera.Rotate({ 0, 0, 1 }, xMove);
					camera.RotateLocal({ 1, 0, 0 }, yMove);
				}

				if (!GUI::Renderer::WantKeyboardCapture())
				{
					bool forward = CoreInput.IsKeyPressed(Core::Input::Keys::W) || CoreInput.IsKeyPressed(Core::Input::Keys::Up);
					bool back = CoreInput.IsKeyPressed(Core::Input::Keys::S) || CoreInput.IsKeyPressed(Core::Input::Keys::Down);
					bool left = CoreInput.IsKeyPressed(Core::Input::Keys::A) || CoreInput.IsKeyPressed(Core::Input::Keys::Left);
					bool right = CoreInput.IsKeyPressed(Core::Input::Keys::D) || CoreInput.IsKeyPressed(Core::Input::Keys::Right);

					bool up = CoreInput.IsKeyPressed(Core::Input::Keys::R);
					bool down = CoreInput.IsKeyPressed(Core::Input::Keys::F);

					bool shift = CoreInput.IsKeyPressed(Core::Input::Keys::Shift);

					const float moveSensitivity = shift ? 120.f : 50.f;
					const float forwardDelta =
						((forward ? moveSensitivity : -moveSensitivity) +
							(back ? -moveSensitivity : moveSensitivity))
						* timeDelta;
					const float rightDelta =
						((right ? moveSensitivity : -moveSensitivity) +
							(left ? -moveSensitivity : moveSensitivity))
						* timeDelta;
					const float upDelta =
						((up ? moveSensitivity : -moveSensitivity) +
							(down ? -moveSensitivity : moveSensitivity))
						* timeDelta;

					camera.MoveLocal({ rightDelta, upDelta, forwardDelta });
				}

				camera.GetTracingParameters(windowWidth, windowHeight, tracingParameters);
				VulkanUtils::Buffer::Copy(deviceInfo.Handle, tracingUniformBuffer.Memory,
					sizeof(TracingParameters), &tracingParameters);

				lastMouseX = mouseX;
				lastMouseY = mouseY;

				if (CoreInput.IsKeyPressed(Core::Input::Keys::Escape))
				{
					CorePlatform.Quit();
				}

				bool updated = GUI::Renderer::Update(deviceInfo, guiVertexBuffer, guiIndexBuffer,
					window, renderDelta, fps, camera, tracingParameters);
			}
			
			if (shouldResize)
			{
				vkDeviceWaitIdle(deviceInfo.Handle);

				windowWidth = window->GetWidth();
				windowHeight = window->GetHeight();

				camera.GetTracingParameters(windowWidth, windowHeight, tracingParameters);
				VulkanUtils::Buffer::Copy(deviceInfo.Handle, tracingUniformBuffer.Memory,
					sizeof(TracingParameters), &tracingParameters);

				for (uint32_t f = 0; f < framebuffers.size(); ++f)
				{
					VulkanFactory::Framebuffer::Destroy(deviceInfo.Handle, framebuffers[f]);
				}
				VulkanFactory::Image::Destroy(deviceInfo.Handle, depthStencil);

				deviceInfo.SurfaceCapabilities = VulkanUtils::Surface::QueryCapabilities(pickedDevice, surface);

				VulkanFactory::Swapchain::SwapchainInfo oldSwapchainInfo = swapchainInfo;
				VulkanFactory::Swapchain::Create("VV Swapchain", deviceInfo, windowWidth, windowHeight, surface,
					swapchainInfo, &oldSwapchainInfo);
				windowData.Swapchain = swapchainInfo.Handle;

				VulkanFactory::Image::Create("Depth Stencil Image", deviceInfo, windowWidth, windowHeight,
					deviceInfo.DepthFormat, depthStencil);

				for (int f = 0; f < framebuffers.size(); ++f)
				{
					assert(f < 1000);
					char framebufferName[sizeof("VV Framebuffer ") + 3 + sizeof(char)];
					sprintf_s(framebufferName, "VV Framebuffer %d", f);

					framebuffers[f] = VulkanFactory::Framebuffer::Create(framebufferName, deviceInfo.Handle, renderPass,
						windowWidth, windowHeight, swapchainInfo.ImageViews[f], depthStencil.View);
				}

				commandBufferBuildData.Width = windowWidth;
				commandBufferBuildData.Height = windowHeight;

				auto now = std::chrono::high_resolution_clock::now();
				auto renderDelta = std::chrono::duration<float, std::milli>(now - before).count();

				bool updated = GUI::Renderer::Update(deviceInfo, guiVertexBuffer, guiIndexBuffer,
					window, renderDelta, fps, camera, tracingParameters);
			}
			
			{
				const auto& shaderList = ShaderManager.GetShaderList();

				for (int s = 0; s < shaderList.size(); ++s)
				{
					if (shaderList[s].shouldReload)
					{
						if (shaderList[s].Name == "simple.vert.glsl")
						{
							VkShaderModule newShader = Shader::Compiler::LoadShader(deviceInfo.Handle, shaderList[s].Path);
							if (newShader != VK_NULL_HANDLE)
							{
								VulkanFactory::Pipeline::Destroy(deviceInfo.Handle, graphicsPipeline);
								VulkanFactory::Shader::Destroy(deviceInfo.Handle, vertexShader);
								vertexShader = newShader;
								graphicsPipeline = VulkanFactory::Pipeline::CreateGraphics(deviceInfo.Handle, renderPass,
									vertexShader, fragmentShader, graphicsPipelineLayout, pipelineCache);
								commandBufferBuildData.Pipeline = graphicsPipeline;
								CoreLogInfo("Reloaded shader %s", shaderList[s].Name.c_str());
							}
							ShaderManager.SignalShaderReloaded(s);
							break;
						}
						if (shaderList[s].Name == "simple.frag.glsl")
						{
							VkShaderModule newShader = Shader::Compiler::LoadShader(deviceInfo.Handle, shaderList[s].Path);
							if (newShader != VK_NULL_HANDLE)
							{
								VulkanFactory::Pipeline::Destroy(deviceInfo.Handle, graphicsPipeline);
								VulkanFactory::Shader::Destroy(deviceInfo.Handle, fragmentShader);
								fragmentShader = newShader;
								graphicsPipeline = VulkanFactory::Pipeline::CreateGraphics(deviceInfo.Handle, renderPass,
									vertexShader, fragmentShader, graphicsPipelineLayout, pipelineCache);
								commandBufferBuildData.Pipeline = graphicsPipeline;
								CoreLogInfo("Reloaded shader %s", shaderList[s].Name.c_str());
							}
							ShaderManager.SignalShaderReloaded(s);
							break;
						}
						if (shaderList[s].Name == "simple.comp.glsl")
						{
							vkDeviceWaitIdle(deviceInfo.Handle);
							VkShaderModule newShader = Shader::Compiler::LoadShader(deviceInfo.Handle, shaderList[s].Path);
							if (newShader != VK_NULL_HANDLE)
							{
								VulkanFactory::Pipeline::Destroy(deviceInfo.Handle, computePipeline);
								VulkanFactory::Shader::Destroy(deviceInfo.Handle, computeShader);
								computeShader = newShader;
								computePipeline = VulkanFactory::Pipeline::CreateCompute(deviceInfo.Handle,
									computePipelineLayout, computeShader, pipelineCache);
								VulkanUtils::CommandBuffer::Begin(computeCommandBuffer);

								vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
								vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeSet, 0, 0);

								vkCmdDispatch(computeCommandBuffer, targetWidth / 16, targetHeight / 16, 1);

								VulkanUtils::CommandBuffer::End(computeCommandBuffer);

								CoreLogInfo("Reloaded shader %s", shaderList[s].Name.c_str());
							}
							ShaderManager.SignalShaderReloaded(s);
							break;
						}
						if (shaderList[s].Name == "ui.vert.glsl")
						{
							VkShaderModule newShader = Shader::Compiler::LoadShader(deviceInfo.Handle, shaderList[s].Path);
							if (newShader != VK_NULL_HANDLE)
							{
								VulkanFactory::Pipeline::Destroy(deviceInfo.Handle, guiPipeline);
								VulkanFactory::Shader::Destroy(deviceInfo.Handle, guiVertexShader);
								guiVertexShader = newShader;
								guiPipeline = VulkanFactory::Pipeline::CreateGuiGraphics(deviceInfo.Handle, renderPass,
									guiVertexShader, guiFragmentShader, guiPipelineLayout, pipelineCache);
								guiCommandBufferBuildData.Pipeline = guiPipeline;
								CoreLogInfo("Reloaded shader %s", shaderList[s].Name.c_str());
							}
							ShaderManager.SignalShaderReloaded(s);
							break;
						}
						if (shaderList[s].Name == "ui.frag.glsl")
						{
							VkShaderModule newShader = Shader::Compiler::LoadShader(deviceInfo.Handle, shaderList[s].Path);
							if (newShader != VK_NULL_HANDLE)
							{
								VulkanFactory::Pipeline::Destroy(deviceInfo.Handle, guiPipeline);
								VulkanFactory::Shader::Destroy(deviceInfo.Handle, guiFragmentShader);
								guiFragmentShader = newShader;
								guiPipeline = VulkanFactory::Pipeline::CreateGuiGraphics(deviceInfo.Handle, renderPass,
									guiVertexShader, guiFragmentShader, guiPipelineLayout, pipelineCache);
								guiCommandBufferBuildData.Pipeline = guiPipeline;
								CoreLogInfo("Reloaded shader %s", shaderList[s].Name.c_str());
							}
							ShaderManager.SignalShaderReloaded(s);
							break;
						}
					}
				}
			}

			for (int f = 0; f < framebuffers.size(); ++f)
			{
				commandBufferBuildData.Framebuffer = framebuffers[f];
				VulkanFactory::CommandBuffer::BuildDraw(drawCommandBuffers[f],
					commandBufferBuildData, guiCommandBufferBuildData);
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

	GUI::Renderer::Shutdown();
#pragma endregion

#pragma region Vulkan deactivation

	VulkanFactory::Pipeline::Destroy(deviceInfo.Handle, computePipeline);
	VulkanFactory::Pipeline::Destroy(deviceInfo.Handle, graphicsPipeline);
	
	VulkanFactory::Pipeline::DestroyCache(deviceInfo.Handle, pipelineCache);

	VulkanFactory::Pipeline::DestroyLayout(deviceInfo.Handle, computePipelineLayout);
	VulkanFactory::Pipeline::DestroyLayout(deviceInfo.Handle, graphicsPipelineLayout);

	VulkanFactory::Buffer::Destroy(deviceInfo, tracingUniformBuffer);

	VulkanFactory::Descriptor::DestroySetLayout(deviceInfo.Handle, computeSetLayout);
	VulkanFactory::Descriptor::DestroySetLayout(deviceInfo.Handle, rasterizationSetLayout);
	VulkanFactory::Descriptor::DestroyPool(deviceInfo.Handle, descriptorPool);

	VulkanFactory::Image::Destroy(deviceInfo.Handle, renderTarget);

	VulkanFactory::Shader::Destroy(deviceInfo.Handle, computeShader);
	VulkanFactory::Shader::Destroy(deviceInfo.Handle, fragmentShader);
	VulkanFactory::Shader::Destroy(deviceInfo.Handle, vertexShader);

	VulkanFactory::Buffer::Destroy(deviceInfo, uploadInfo.TreeRootsStorageBuffer);
	VulkanFactory::Buffer::Destroy(deviceInfo, uploadInfo.PagesStorageBuffer);
	VulkanFactory::Buffer::Destroy(deviceInfo, uploadInfo.PageTableStorageBuffer);

	for (auto& framebuffer : framebuffers)
	{
		VulkanFactory::Framebuffer::Destroy(deviceInfo.Handle, framebuffer);
	}
	
	VulkanFactory::RenderPass::Destroy(deviceInfo.Handle, renderPass);
	
	VulkanFactory::Image::Destroy(deviceInfo.Handle, depthStencil);

	VulkanFactory::Fence::Destroy(deviceInfo.Handle, computeFence);

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