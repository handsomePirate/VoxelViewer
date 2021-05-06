#include "VulkanFactory.hpp"
#include "Utils.hpp"
#include "Logger/Logger.hpp"
#include "Platform.hpp"
#include "Platform/Platform.hpp"
#include "Initializers.hpp"

VkInstance VulkanFactory::Instance::Create(const std::vector<const char*>& extensions,
	uint32_t apiVersion, const char* validationLayerName)
{
	VkApplicationInfo appInfo = VulkanInitializers::ApplicationInfo();
	appInfo.pApplicationName = "VoxelViewer";
	appInfo.pEngineName = "VoxelViewer";
	appInfo.apiVersion = apiVersion;

	auto instanceInitializer = VulkanInitializers::Instance(&appInfo);
	if (extensions.size() > 0)
	{
		instanceInitializer.enabledExtensionCount = (uint32_t)extensions.size();
		instanceInitializer.ppEnabledExtensionNames = extensions.data();
	}
	if (validationLayerName != "")
	{
		instanceInitializer.ppEnabledLayerNames = &validationLayerName;
		instanceInitializer.enabledLayerCount = 1;
	}

	VkInstance instance;
	VkResult result = vkCreateInstance(&instanceInitializer, nullptr, &instance);
	assert(result == VK_SUCCESS);
	return instance;
}

void VulkanFactory::Instance::Destroy(VkInstance instance)
{
	vkDestroyInstance(instance, nullptr);
}

void VulkanFactory::Device::Create(
	VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures& enabledFeatures,
	std::vector<const char*> extensions, DeviceInfo& output,
	VkQueueFlags requestedQueueTypes, bool debugMarkersEnabled)
{
	output.Properties = VulkanUtils::Device::GetPhysicalDeviceProperties(physicalDevice);
	output.Features = VulkanUtils::Device::GetPhysicalDeviceFeatures(physicalDevice);
	output.MemoryProperties = VulkanUtils::Device::GetPhysicalDeviceMemoryProperties(physicalDevice);
	output.QueueFamilyProperties = VulkanUtils::Device::GetQueueFamilyProperties(physicalDevice);
	output.DepthFormat = VulkanUtils::Device::GetSupportedDepthFormat(physicalDevice);

	assert(output.DepthFormat != VK_FORMAT_UNDEFINED);

	output.DebugMarkersEnabled = debugMarkersEnabled;
	std::vector<VkDeviceQueueCreateInfo> queueInitializers{};

	// Graphics queue.
	if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
	{
		auto queueInitializer = VulkanInitializers::Queue(output.QueueFamilyProperties, VK_QUEUE_GRAPHICS_BIT);
		output.QueueFamilyIndices.graphics = queueInitializer.queueFamilyIndex;
		queueInitializers.push_back(queueInitializer);
	}
	else
	{
		output.QueueFamilyIndices.graphics = UINT32_MAX;
	}

	// Compute queue.
	if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
	{
		auto queueInitializer = VulkanInitializers::Queue(output.QueueFamilyProperties, VK_QUEUE_COMPUTE_BIT);
		output.QueueFamilyIndices.compute = queueInitializer.queueFamilyIndex;
		if (output.QueueFamilyIndices.compute != output.QueueFamilyIndices.graphics)
		{
			queueInitializers.push_back(queueInitializer);
		}
	}
	else
	{
		output.QueueFamilyIndices.compute = output.QueueFamilyIndices.graphics;
	}

	// Transfer queue.
	if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
	{
		auto queueInitializer = VulkanInitializers::Queue(output.QueueFamilyProperties, VK_QUEUE_TRANSFER_BIT);
		output.QueueFamilyIndices.transfer = queueInitializer.queueFamilyIndex;
		if (output.QueueFamilyIndices.transfer != output.QueueFamilyIndices.graphics &&
			output.QueueFamilyIndices.transfer != output.QueueFamilyIndices.compute)
		{
			queueInitializers.push_back(queueInitializer);
		}
	}
	else
	{
		output.QueueFamilyIndices.transfer = output.QueueFamilyIndices.graphics;
	}

	VkDeviceCreateInfo deviceInitializer = VulkanInitializers::Device();
	deviceInitializer.queueCreateInfoCount = (uint32_t)queueInitializers.size();
	deviceInitializer.pQueueCreateInfos = queueInitializers.data();
	deviceInitializer.pEnabledFeatures = &enabledFeatures;
	output.EnabledFeatures = enabledFeatures;
	if (extensions.size() > 0)
	{
		deviceInitializer.enabledExtensionCount = (uint32_t)extensions.size();
		deviceInitializer.ppEnabledExtensionNames = extensions.data();
	}

	VkResult result = vkCreateDevice(physicalDevice, &deviceInitializer, nullptr, &output.Handle);

	if (result != VK_SUCCESS)
	{
		CoreLogger.Log(Core::LoggerSeverity::Fatal, "Failed to connect with the graphics driver.");
	}
}

void VulkanFactory::Device::Destroy(DeviceInfo& deviceInfo)
{
	// TODO: When there is a default command pool, destroy here.
	vkDestroyDevice(deviceInfo.Handle, nullptr);
}

VkCommandPool VulkanFactory::CommandPool::Create(VkDevice device, uint32_t queueIndex, VkCommandPoolCreateFlags flags)
{
	auto commandPoolInitializer = VulkanInitializers::CommandPool();
	commandPoolInitializer.queueFamilyIndex = queueIndex;
	commandPoolInitializer.flags = flags;
	VkCommandPool commandPool;
	vkCreateCommandPool(device, &commandPoolInitializer, nullptr, &commandPool);
	return commandPool;
}

void VulkanFactory::CommandPool::Destroy(VkDevice device, VkCommandPool commandPool)
{
	vkDestroyCommandPool(device, commandPool, nullptr);
}

VkSemaphore VulkanFactory::Semaphore::Create(VkDevice device)
{
	auto semaphoreInitializer = VulkanInitializers::Semaphore();
	VkSemaphore semaphore;
	VkResult result = vkCreateSemaphore(device, &semaphoreInitializer, nullptr, &semaphore);
	assert(result == VK_SUCCESS);
	return semaphore;
}

void VulkanFactory::Semaphore::Destroy(VkDevice device, VkSemaphore semaphore)
{
	vkDestroySemaphore(device, semaphore, nullptr);
}

VkFence VulkanFactory::Fence::Create(VkDevice device, VkFenceCreateFlags flags)
{
	auto fenceInitializer = VulkanInitializers::Fence();
	fenceInitializer.flags = flags;
	VkFence fence;
	VkResult result = vkCreateFence(device, &fenceInitializer, nullptr, &fence);
	assert(result == VK_SUCCESS);
	return fence;
}

void VulkanFactory::Fence::Destroy(VkDevice device, VkFence fence)
{
	vkDestroyFence(device, fence, nullptr);
}

VkSurfaceKHR VulkanFactory::Surface::Create(VkInstance instance, uint64_t windowHandle)
{
	return SurfaceFactory::Create(instance, windowHandle);
}

void VulkanFactory::Surface::Destroy(VkInstance instance, VkSurfaceKHR surface)
{
	vkDestroySurfaceKHR(instance, surface, nullptr);
}

VkQueue VulkanFactory::Queue::Get(VkDevice device, uint32_t queueFamily, uint32_t queueIndex)
{
	VkQueue queue;
	vkGetDeviceQueue(device, queueFamily, queueIndex, &queue);
	return queue;
}

void VulkanFactory::Swapchain::Create(
	uint32_t Width, uint32_t Height, VkSurfaceKHR surface, const Device::DeviceInfo& deviceInfo,
	SwapchainInfo& output, SwapchainInfo* oldSwapchainInfo)
{
	bool oldSwapchainProvided = oldSwapchainInfo != nullptr ? true : false;

	output.Extent = VulkanUtils::Surface::QueryExtent(Width, Height, deviceInfo.SurfaceCapabilities);

	uint32_t imageCount = VulkanUtils::Swapchain::QueryImageCount(deviceInfo.SurfaceCapabilities);

	auto swapchainInitializer = VulkanInitializers::Swapchain(output.Extent, surface);
	swapchainInitializer.imageFormat = deviceInfo.SurfaceFormat.format;
	swapchainInitializer.imageColorSpace = deviceInfo.SurfaceFormat.colorSpace;
	swapchainInitializer.compositeAlpha = deviceInfo.CompositeAlpha;
	swapchainInitializer.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainInitializer.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainInitializer.preTransform = deviceInfo.SurfaceTransform;
	swapchainInitializer.presentMode = deviceInfo.PresentMode;
	swapchainInitializer.oldSwapchain = oldSwapchainProvided ? oldSwapchainInfo->Handle : VK_NULL_HANDLE;
	swapchainInitializer.minImageCount = imageCount;

	// Enable transfer source on swap chain images if supported
	if (deviceInfo.SurfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
	{
		swapchainInitializer.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	// Enable transfer destination on swap chain images if supported
	if (deviceInfo.SurfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
	{
		swapchainInitializer.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	VkResult result = vkCreateSwapchainKHR(deviceInfo.Handle, &swapchainInitializer, nullptr, &output.Handle);

	if (result != VK_SUCCESS)
	{
		CoreLogger.Log(Core::LoggerSeverity::Fatal, "Failed to create swapchain!");
	}

	if (oldSwapchainProvided)
	{
		Destroy(deviceInfo, *oldSwapchainInfo);
	}

	output.Images = VulkanUtils::Swapchain::GetImages(deviceInfo.Handle, output.Handle);
	assert(output.Images.size() >= imageCount);

	output.ImageViews.resize(output.Images.size());
	for (int i = 0; i < output.Images.size(); ++i)
	{
		auto colorAttachmentViewInitializer = VulkanInitializers::ColorAttachmentView(
			output.Images[i], deviceInfo.SurfaceFormat.format);

		vkCreateImageView(deviceInfo.Handle, &colorAttachmentViewInitializer, nullptr, &output.ImageViews[i]);
	}
}

void VulkanFactory::Swapchain::Destroy(const Device::DeviceInfo& deviceInfo, SwapchainInfo& swapchainInfo)
{
	for (uint32_t i = 0; i < swapchainInfo.ImageViews.size(); i++)
	{
		vkDestroyImageView(deviceInfo.Handle, swapchainInfo.ImageViews[i], nullptr);
	}
	vkDestroySwapchainKHR(deviceInfo.Handle, swapchainInfo.Handle, nullptr);
}

void VulkanFactory::CommandBuffer::AllocatePrimary(VkDevice device, VkCommandPool commandPool,
	std::vector<VkCommandBuffer>& output, uint32_t bufferCount)
{
	output.resize(bufferCount);

	auto commandBufferInitializer = VulkanInitializers::CommandBufferAllocation(
		commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, bufferCount);

	vkAllocateCommandBuffers(device, &commandBufferInitializer, output.data());
}

VkCommandBuffer VulkanFactory::CommandBuffer::AllocatePrimary(VkDevice device, VkCommandPool commandPool)
{
	auto commandBufferInitializer = VulkanInitializers::CommandBufferAllocation(
		commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &commandBufferInitializer, &commandBuffer);

	return commandBuffer;
}

void VulkanFactory::CommandBuffer::Free(VkDevice device, VkCommandPool commandPool, std::vector<VkCommandBuffer>& buffers)
{
	vkFreeCommandBuffers(device, commandPool, (uint32_t)buffers.size(), buffers.data());
}

void VulkanFactory::CommandBuffer::Free(VkDevice device, VkCommandPool commandPool, VkCommandBuffer buffer)
{
	vkFreeCommandBuffers(device, commandPool, 1, &buffer);
}

void VulkanFactory::Image::Create(const Device::DeviceInfo& deviceInfo, VkFormat format,
	uint32_t Width, uint32_t Height, ImageInfo& output)
{
	auto imageInitializer = VulkanInitializers::Image(format);
	imageInitializer.extent = { Width, Height, 1 };
	imageInitializer.mipLevels = 1;
	imageInitializer.arrayLayers = 1;
	imageInitializer.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInitializer.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInitializer.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	VkResult result = vkCreateImage(deviceInfo.Handle, &imageInitializer, nullptr, &output.image);
	assert(result == VK_SUCCESS);

	VkMemoryRequirements memoryRequirements = VulkanUtils::Image::GetMemoryRequirements(deviceInfo.Handle, output.image);
	auto memoryAllocateInfo = VulkanInitializers::MemoryAllocation(
		memoryRequirements.size,
		VulkanUtils::Memory::GetTypeIndex(
			deviceInfo.MemoryProperties,
			memoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	);

	vkAllocateMemory(deviceInfo.Handle, &memoryAllocateInfo, nullptr, &output.memory);
	vkBindImageMemory(deviceInfo.Handle, output.image, output.memory, 0);

	auto depthAttachmentViewInitializer = VulkanInitializers::DepthAttachmentView(output.image, format);
	vkCreateImageView(deviceInfo.Handle, &depthAttachmentViewInitializer, nullptr, &output.view);
}

void VulkanFactory::Image::Destroy(VkDevice device, ImageInfo& imageInfo)
{
	vkDestroyImageView(device, imageInfo.view, nullptr);
	vkFreeMemory(device, imageInfo.memory, nullptr);
	vkDestroyImage(device, imageInfo.image, nullptr);
}

VkRenderPass VulkanFactory::RenderPass::Create(VkDevice device, VkFormat colorFormat, VkFormat depthFormat)
{
	const int attachmentCount = 2;
	VkAttachmentDescription attachments[attachmentCount];
	// Color attachment
	attachments[0] = {};
	attachments[0].format = colorFormat;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	// Depth attachment
	attachments[1] = {};
	attachments[1].format = depthFormat;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference{};
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthReference{};
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription{};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;
	subpassDescription.pDepthStencilAttachment = &depthReference;

	// Subpass dependencies for layout transitions
	const int dependencyCount = 2;
	VkSubpassDependency dependencies[dependencyCount];

	dependencies[0] = {};
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1] = {};
	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = attachmentCount;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = dependencyCount;
	renderPassInfo.pDependencies = dependencies;

	VkRenderPass output;
	VkResult result = vkCreateRenderPass(device, &renderPassInfo, nullptr, &output);

	return output;
}

void VulkanFactory::RenderPass::Destroy(VkDevice device, VkRenderPass renderPass)
{
	vkDestroyRenderPass(device, renderPass, nullptr);
}

VkFramebuffer VulkanFactory::Framebuffer::Create(VkDevice device, VkRenderPass renderPass, uint32_t Width, uint32_t Height,
	VkImageView colorView, VkImageView depthView)
{
	const int attachmentCount = 2;
	VkImageView attachments[attachmentCount];
	attachments[0] = colorView;
	attachments[1] = depthView;

	auto framebufferInitializer = VulkanInitializers::Framebuffer();
	framebufferInitializer.renderPass = renderPass;
	framebufferInitializer.attachmentCount = attachmentCount;
	framebufferInitializer.pAttachments = attachments;
	framebufferInitializer.width = Width;
	framebufferInitializer.height = Height;
	framebufferInitializer.layers = 1;

	VkFramebuffer framebuffer;
	VkResult result = vkCreateFramebuffer(device, &framebufferInitializer, nullptr, &framebuffer);
	assert(result == VK_SUCCESS);

	return framebuffer;
}

void VulkanFactory::Framebuffer::Destroy(VkDevice device, VkFramebuffer framebuffer)
{
	vkDestroyFramebuffer(device, framebuffer, nullptr);
}

VkDescriptorSetLayout VulkanFactory::Descriptor::CreateSetLayout(VkDevice device)
{
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings =
	{
		VulkanInitializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT, 0)
	};

	VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = VulkanInitializers::DescriptorSetLayout(
		setLayoutBindings.data(), (uint32_t)setLayoutBindings.size());

	VkDescriptorSetLayout descriptorSetLayout;
	VkResult result = vkCreateDescriptorSetLayout(device, &descriptorLayoutCreateInfo, nullptr, &descriptorSetLayout);
	assert(result == VK_SUCCESS);
	return descriptorSetLayout;
}

void VulkanFactory::Descriptor::DestroySetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout)
{
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}

VkPipeline VulkanFactory::Pipeline::CreateGraphics(VkDevice device, VkRenderPass renderPass,
	const Swapchain::SwapchainInfo& swapchain, VkShaderModule vertexShader, VkShaderModule fragmentShader, 
	VkPipelineLayout pipelineLayout, VkPipelineCache pipelineCache)
{
	auto inputAssemblyStateInitializer = VulkanInitializers::PipelineInputAssemblyState(
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

	auto rasterizationStateInitializer = VulkanInitializers::PipelineRasterizationState(
		VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

	auto blendAttachmentStateInitializer = VulkanInitializers::PipelineColorBlendAttachment(
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		VK_FALSE);

	auto colorBlendStateInitializer = VulkanInitializers::PipelineColorBlendState(
		&blendAttachmentStateInitializer, 1);

	auto depthStencilStateInitializer = VulkanInitializers::PipelineDepthStencilState(
		VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);

	VkViewport viewport = VulkanUtils::Pipeline::CreateViewport(swapchain.Width, swapchain.Height);
	VkRect2D scissor = VulkanUtils::Pipeline::CreateScissor(swapchain.Extent);
	auto viewportStateInitializer = VulkanInitializers::PipelineViewportState(viewport, scissor);

	auto multisampleStateInitializer = VulkanInitializers::PipelineMultisampleState(
		VK_SAMPLE_COUNT_1_BIT);

	std::vector<VkDynamicState> dynamicStateEnables = 
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	auto dynamicStateInitializer = VulkanInitializers::PipelineDynamicState(dynamicStateEnables.data(),
		(uint32_t)dynamicStateEnables.size());

	// Display pipeline
	const int shaderCount = 2;
	VkPipelineShaderStageCreateInfo shaderStages[shaderCount];
	shaderStages[0] = VulkanInitializers::PipelineShaderStage(vertexShader, VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = VulkanInitializers::PipelineShaderStage(fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT);

	// TODO: Put into initializers.
	VkPipelineVertexInputStateCreateInfo emptyInputState{};
	emptyInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	emptyInputState.vertexAttributeDescriptionCount = 0;
	emptyInputState.pVertexAttributeDescriptions = nullptr;
	emptyInputState.vertexBindingDescriptionCount = 0;
	emptyInputState.pVertexBindingDescriptions = nullptr;

	auto pipelineCreateInitializer = VulkanInitializers::GraphicsPipeline();
	pipelineCreateInitializer.renderPass = renderPass;
	pipelineCreateInitializer.layout = pipelineLayout;
	pipelineCreateInitializer.pVertexInputState = &emptyInputState;
	pipelineCreateInitializer.pInputAssemblyState = &inputAssemblyStateInitializer;
	pipelineCreateInitializer.pRasterizationState = &rasterizationStateInitializer;
	pipelineCreateInitializer.pColorBlendState = &colorBlendStateInitializer;
	pipelineCreateInitializer.pMultisampleState = &multisampleStateInitializer;
	pipelineCreateInitializer.pViewportState = &viewportStateInitializer;
	pipelineCreateInitializer.pDepthStencilState = &depthStencilStateInitializer;
	pipelineCreateInitializer.pDynamicState = &dynamicStateInitializer;
	pipelineCreateInitializer.stageCount = shaderCount;
	pipelineCreateInitializer.pStages = shaderStages;
	pipelineCreateInitializer.renderPass = renderPass;
	
	VkPipeline pipeline;
	vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInitializer, nullptr, &pipeline);
	return pipeline;
}

void VulkanFactory::Pipeline::Destroy(VkDevice device, VkPipeline pipeline)
{
	vkDestroyPipeline(device, pipeline, nullptr);
}

VkPipelineLayout VulkanFactory::Pipeline::CreateLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout)
{
	auto pipelineLayoutInitializer = VulkanInitializers::PipelineLayout(&descriptorSetLayout, 1);

	VkPipelineLayout pipelineLayout;
	vkCreatePipelineLayout(device, &pipelineLayoutInitializer, nullptr, &pipelineLayout);
	return pipelineLayout;
}

void VulkanFactory::Pipeline::DestroyLayout(VkDevice device, VkPipelineLayout pipelineLayout)
{
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

VkPipelineCache VulkanFactory::Pipeline::CreateCache(VkDevice device)
{
	auto pipelineCacheInitializer = VulkanInitializers::PipelineCache();
	VkPipelineCache pipelineCache;
	vkCreatePipelineCache(device, &pipelineCacheInitializer, nullptr, &pipelineCache);
	return pipelineCache;
}

void VulkanFactory::Pipeline::DestroyCache(VkDevice device, VkPipelineCache pipelineCache)
{
	vkDestroyPipelineCache(device, pipelineCache, nullptr);
}

VkShaderModule VulkanFactory::Shader::Create(VkDevice device, const std::string& path)
{
	std::vector<uint8_t> byteCode;

	{
		FILE* f;
		fopen_s(&f, path.c_str(), "rb");

		if (!f)
		{
			CoreLogger.Log(Core::LoggerSeverity::Error, "Failed to load shader code.");
			return VK_NULL_HANDLE;
		}

		fseek(f, 0, SEEK_END);
		const size_t fileSize = ftell(f);
		fseek(f, 0, SEEK_SET);
		byteCode.resize(fileSize);
		fread_s(byteCode.data(), fileSize, 1, fileSize, f);
		fclose(f);
	}

	auto shaderInitializer = VulkanInitializers::Shader();
	shaderInitializer.codeSize = (uint32_t)byteCode.size();
	shaderInitializer.pCode = (uint32_t* const)byteCode.data();

	VkShaderModule shaderModule;
	vkCreateShaderModule(device, &shaderInitializer, nullptr, &shaderModule);

	return shaderModule;
}

void VulkanFactory::Shader::Destroy(VkDevice device, VkShaderModule shader)
{
	vkDestroyShaderModule(device, shader, nullptr);
}
