#include "VulkanFactory.hpp"
//#include "Utils.hpp"
#include "Core/Logger/Logger.hpp"
#include "Platform.hpp"
#include "Core/Platform/Platform.hpp"
#include "Initializers.hpp"
#include "ImGui/ImGui.hpp"
#include "Debug/Debug.hpp"
#include <imgui.h>

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
	VkQueueFlags requestedQueueTypes)
{
	output.PhysicalDevice = physicalDevice;
	output.Properties = VulkanUtils::Device::GetPhysicalDeviceProperties(physicalDevice);
	output.Features = VulkanUtils::Device::GetPhysicalDeviceFeatures(physicalDevice);
	output.MemoryProperties = VulkanUtils::Device::GetPhysicalDeviceMemoryProperties(physicalDevice);
	output.QueueFamilyProperties = VulkanUtils::Device::GetQueueFamilyProperties(physicalDevice);
	output.DepthFormat = VulkanUtils::Device::GetSupportedDepthFormat(physicalDevice);

	assert(output.DepthFormat != VK_FORMAT_UNDEFINED);

	std::vector<VkDeviceQueueCreateInfo> queueInitializers{};

	const float defaultPriority = 0.f;
	// Graphics queue.
	if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
	{
		auto queueInitializer = VulkanInitializers::Queue(output.QueueFamilyProperties, VK_QUEUE_GRAPHICS_BIT);
		queueInitializer.pQueuePriorities = &defaultPriority;
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
		queueInitializer.pQueuePriorities = &defaultPriority;
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
		queueInitializer.pQueuePriorities = &defaultPriority;
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
		CoreLogFatal("Failed to connect with the graphics driver.");
	}
}

void VulkanFactory::Device::Destroy(DeviceInfo& deviceInfo)
{
	// TODO: When there is a default command pool, destroy here.
	vkDestroyDevice(deviceInfo.Handle, nullptr);
}

VkCommandPool VulkanFactory::CommandPool::Create(const char* name, VkDevice device, uint32_t queueIndex,
	VkCommandPoolCreateFlags flags)
{
	auto commandPoolInitializer = VulkanInitializers::CommandPool();
	commandPoolInitializer.queueFamilyIndex = queueIndex;
	commandPoolInitializer.flags = flags;
	VkCommandPool commandPool;
	vkCreateCommandPool(device, &commandPoolInitializer, nullptr, &commandPool);
	Debug::Utils::SetCommandPoolName(device, commandPool, name);
	return commandPool;
}

void VulkanFactory::CommandPool::Destroy(VkDevice device, VkCommandPool commandPool)
{
	vkDestroyCommandPool(device, commandPool, nullptr);
}

VkSemaphore VulkanFactory::Semaphore::Create(const char* name, VkDevice device)
{
	auto semaphoreInitializer = VulkanInitializers::Semaphore();
	VkSemaphore semaphore;
	VkResult result = vkCreateSemaphore(device, &semaphoreInitializer, nullptr, &semaphore);
	assert(result == VK_SUCCESS);
	Debug::Utils::SetSemaphoreName(device, semaphore, name);
	return semaphore;
}

void VulkanFactory::Semaphore::Destroy(VkDevice device, VkSemaphore semaphore)
{
	vkDestroySemaphore(device, semaphore, nullptr);
}

VkFence VulkanFactory::Fence::Create(const char* name, VkDevice device, VkFenceCreateFlags flags)
{
	auto fenceInitializer = VulkanInitializers::Fence();
	fenceInitializer.flags = flags;
	VkFence fence;
	VkResult result = vkCreateFence(device, &fenceInitializer, nullptr, &fence);
	Debug::Utils::SetFenceName(device, fence, name);
	assert(result == VK_SUCCESS);
	return fence;
}

void VulkanFactory::Fence::Destroy(VkDevice device, VkFence fence)
{
	vkDestroyFence(device, fence, nullptr);
}

VkSurfaceKHR VulkanFactory::Surface::Create(const char* name, VkDevice device, VkInstance instance, uint64_t windowHandle)
{
	VkSurfaceKHR surface = SurfaceFactory::Create(instance, windowHandle);
	Debug::Utils::SetSurfaceName(device, surface, name);
	return surface;
}

void VulkanFactory::Surface::Destroy(VkInstance instance, VkSurfaceKHR surface)
{
	vkDestroySurfaceKHR(instance, surface, nullptr);
}

VkQueue VulkanFactory::Queue::Get(const char* name, VkDevice device, uint32_t queueFamily, uint32_t queueIndex)
{
	VkQueue queue;
	vkGetDeviceQueue(device, queueFamily, queueIndex, &queue);
	Debug::Utils::SetQueueName(device, queue, name);
	return queue;
}

void VulkanFactory::Swapchain::Create(const char* name, const Device::DeviceInfo& deviceInfo,
	uint32_t width, uint32_t height, VkSurfaceKHR surface,
	SwapchainInfo& output, SwapchainInfo* oldSwapchainInfo)
{
	bool oldSwapchainProvided = oldSwapchainInfo != nullptr ? true : false;

	output.Extent = VulkanUtils::Surface::QueryExtent(width, height, deviceInfo.SurfaceCapabilities);

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
	Debug::Utils::SetSwapchainName(deviceInfo.Handle, output.Handle, name);

	if (result != VK_SUCCESS)
	{
		CoreLogFatal("Failed to create swapchain!");
		assert(false);
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

void VulkanFactory::Buffer::Create(const char* name, const Device::DeviceInfo& deviceInfo, VkBufferUsageFlags usage,
	VkDeviceSize size, VkMemoryPropertyFlags memoryProperties, BufferInfo& output)
{
	output.DescriptorBufferInfo.offset = 0;
	output.DescriptorBufferInfo.range = VK_WHOLE_SIZE;
	output.Size = size;

	auto bufferInitializer = VulkanInitializers::Buffer(usage, size);

	VkResult result = vkCreateBuffer(deviceInfo.Handle, &bufferInitializer, nullptr, &output.DescriptorBufferInfo.buffer);
	assert(result == VK_SUCCESS);
	Debug::Utils::SetBufferName(deviceInfo.Handle, output.DescriptorBufferInfo.buffer, name);

	output.Memory = VulkanUtils::Memory::AllocateBuffer(deviceInfo.Handle, deviceInfo.MemoryProperties,
		output.DescriptorBufferInfo.buffer, memoryProperties);

	result = vkBindBufferMemory(deviceInfo.Handle, output.DescriptorBufferInfo.buffer, output.Memory,
		output.DescriptorBufferInfo.offset);
	assert(result == VK_SUCCESS);
}

void VulkanFactory::Buffer::Destroy(const Device::DeviceInfo& deviceInfo, BufferInfo& bufferInfo)
{
	vkFreeMemory(deviceInfo.Handle, bufferInfo.Memory, nullptr);
	vkDestroyBuffer(deviceInfo.Handle, bufferInfo.DescriptorBufferInfo.buffer, nullptr);
}

void VulkanFactory::CommandBuffer::AllocatePrimary(VkDevice device, VkCommandPool commandPool,
	VkCommandBuffer* output, uint32_t bufferCount)
{
	auto commandBufferInitializer = VulkanInitializers::CommandBufferAllocation(
		commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, bufferCount);

	vkAllocateCommandBuffers(device, &commandBufferInitializer, output);
}

VkCommandBuffer VulkanFactory::CommandBuffer::AllocatePrimary(const char* name, VkDevice device, VkCommandPool commandPool)
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

void VulkanFactory::CommandBuffer::BuildDraw(VkCommandBuffer commandBuffer, const BuildData& data, const GuiBuildData& guiData)
{
	VkClearColorValue clearColor;
	clearColor.float32[0] = 1.f;
	clearColor.float32[1] = 0.f;
	clearColor.float32[2] = 0.f;
	clearColor.float32[3] = 1.f;

	VkClearValue clearValues[2];
	clearValues[0].color = clearColor;
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = VulkanInitializers::RenderPassBeginning(
		data.RenderPass, data.Width, data.Height);
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.framebuffer = data.Framebuffer;

	VulkanUtils::CommandBuffer::Begin(commandBuffer);

	// Image memory barrier to make sure that compute shader writes are finished before sampling from the texture
	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	imageMemoryBarrier.image = data.Target;
	imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = VulkanUtils::Pipeline::CreateViewport(data.Width, data.Height, 0.0f, 1.0f);
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor = VulkanUtils::Pipeline::CreateScissor(VkExtent2D{ data.Width, data.Height });
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	// Display ray traced image generated by compute shader as a full screen quad
	// Quad vertices are generated in the vertex shader
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, data.PipelineLayout,
		0, 1, &data.DescriptorSet, 0, NULL);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, data.Pipeline);
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	GUI::Renderer::Draw(commandBuffer, guiData.Pipeline, guiData.PipelineLayout, guiData.DescriptorSet,
		guiData.PushConstantBlock, *guiData.VertexBuffer, *guiData.IndexBuffer);

	vkCmdEndRenderPass(commandBuffer);

	VulkanUtils::CommandBuffer::End(commandBuffer);
}

void VulkanFactory::Image::Create(const char* name, const Device::DeviceInfo& deviceInfo,
	uint32_t width, uint32_t height, VkFormat format, ImageInfo& output)
{
	auto imageInitializer = VulkanInitializers::Image(format);
	imageInitializer.extent = { width, height, 1 };
	imageInitializer.mipLevels = 1;
	imageInitializer.arrayLayers = 1;
	imageInitializer.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInitializer.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInitializer.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	VkResult result = vkCreateImage(deviceInfo.Handle, &imageInitializer, nullptr, &output.Image);
	assert(result == VK_SUCCESS);
	Debug::Utils::SetImageName(deviceInfo.Handle, output.Image, name);

	output.Memory = VulkanUtils::Memory::AllocateImage(deviceInfo.Handle, deviceInfo.MemoryProperties,
		output.Image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	result = vkBindImageMemory(deviceInfo.Handle, output.Image, output.Memory, 0);
	assert(result == VK_SUCCESS);

	auto depthAttachmentViewInitializer = VulkanInitializers::DepthAttachmentView(output.Image, format);
	vkCreateImageView(deviceInfo.Handle, &depthAttachmentViewInitializer, nullptr, &output.View);
}

void VulkanFactory::Image::Destroy(VkDevice device, ImageInfo& imageInfo)
{
	vkDestroyImageView(device, imageInfo.View, nullptr);
	vkFreeMemory(device, imageInfo.Memory, nullptr);
	vkDestroyImage(device, imageInfo.Image, nullptr);
}

void VulkanFactory::Image::Create(const char* name, const Device::DeviceInfo& deviceInfo, uint32_t width, uint32_t height,
	VkFormat format, VkImageUsageFlags usage, ImageInfo2& output)
{
	VkFormatProperties formatProperties = VulkanUtils::Device::GetPhysicalDeviceFormatProperties(
		deviceInfo.PhysicalDevice, format);
	// Check if requested image format supports image storage operations
	assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT);

	VkImageCreateInfo imageCreateInfo = VulkanInitializers::Image(format);
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent = { width, height, 1 };
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	// Image will be sampled in the fragment shader and used as storage target in the compute shader
	imageCreateInfo.usage = usage;
	imageCreateInfo.flags = 0;

	output.DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkResult result = vkCreateImage(deviceInfo.Handle, &imageCreateInfo, nullptr, &output.Image);
	assert(result == VK_SUCCESS);
	Debug::Utils::SetImageName(deviceInfo.Handle, output.Image, name);

	output.Memory = VulkanUtils::Memory::AllocateImage(deviceInfo.Handle, deviceInfo.MemoryProperties,
		output.Image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	result = vkBindImageMemory(deviceInfo.Handle, output.Image, output.Memory, 0);
	assert(result == VK_SUCCESS);

	// Create sampler
	auto samplerInitializer = VulkanInitializers::Sampler();
	samplerInitializer.magFilter = VK_FILTER_LINEAR;
	samplerInitializer.minFilter = VK_FILTER_LINEAR;
	samplerInitializer.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInitializer.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInitializer.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInitializer.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInitializer.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	result = vkCreateSampler(deviceInfo.Handle, &samplerInitializer, nullptr, &output.DescriptorImageInfo.sampler);
	assert(result == VK_SUCCESS);

	// Create image view
	VkImageViewCreateInfo imageViewInitializer = VulkanInitializers::ImageView(output.Image, format);
	imageViewInitializer.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewInitializer.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	result = vkCreateImageView(deviceInfo.Handle, &imageViewInitializer, nullptr, &output.DescriptorImageInfo.imageView);
	assert(result == VK_SUCCESS);
}

void VulkanFactory::Image::Destroy(VkDevice device, ImageInfo2& imageInfo)
{
	vkDestroyImageView(device, imageInfo.DescriptorImageInfo.imageView, nullptr);
	vkDestroySampler(device, imageInfo.DescriptorImageInfo.sampler, nullptr);
	vkFreeMemory(device, imageInfo.Memory, nullptr);
	vkDestroyImage(device, imageInfo.Image, nullptr);
}

VkRenderPass VulkanFactory::RenderPass::Create(const char* name, VkDevice device, VkFormat colorFormat, VkFormat depthFormat)
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

	VkRenderPass renderPass;
	VkResult result = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
	assert(result == VK_SUCCESS);
	Debug::Utils::SetRenderPassName(device, renderPass, name);

	return renderPass;
}

void VulkanFactory::RenderPass::Destroy(VkDevice device, VkRenderPass renderPass)
{
	vkDestroyRenderPass(device, renderPass, nullptr);
}

VkFramebuffer VulkanFactory::Framebuffer::Create(const char* name, VkDevice device, VkRenderPass renderPass,
	uint32_t width, uint32_t height, VkImageView colorView, VkImageView depthView)
{
	const int attachmentCount = 2;
	VkImageView attachments[attachmentCount];
	attachments[0] = colorView;
	attachments[1] = depthView;

	auto framebufferInitializer = VulkanInitializers::Framebuffer();
	framebufferInitializer.renderPass = renderPass;
	framebufferInitializer.attachmentCount = attachmentCount;
	framebufferInitializer.pAttachments = attachments;
	framebufferInitializer.width = width;
	framebufferInitializer.height = height;
	framebufferInitializer.layers = 1;

	VkFramebuffer framebuffer;
	VkResult result = vkCreateFramebuffer(device, &framebufferInitializer, nullptr, &framebuffer);
	assert(result == VK_SUCCESS);
	Debug::Utils::SetFramebufferName(device, framebuffer, name);

	return framebuffer;
}

void VulkanFactory::Framebuffer::Destroy(VkDevice device, VkFramebuffer framebuffer)
{
	vkDestroyFramebuffer(device, framebuffer, nullptr);
}

VkDescriptorSetLayout VulkanFactory::Descriptor::CreateSetLayout(const char* name, VkDevice device,
	VkDescriptorSetLayoutBinding* layoutBindings, uint32_t bindingCount)
{
	VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = VulkanInitializers::DescriptorSetLayout(
		layoutBindings, bindingCount);

	VkDescriptorSetLayout descriptorSetLayout;
	VkResult result = vkCreateDescriptorSetLayout(device, &descriptorLayoutCreateInfo, nullptr, &descriptorSetLayout);
	assert(result == VK_SUCCESS);
	Debug::Utils::SetDescriptorSetLayoutName(device, descriptorSetLayout, name);

	return descriptorSetLayout;
}

void VulkanFactory::Descriptor::DestroySetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout)
{
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}

VkDescriptorPool VulkanFactory::Descriptor::CreatePool(const char* name, VkDevice device, VkDescriptorPoolSize* poolSizes,
	uint32_t sizesCount, uint32_t maxSets)
{
	VkDescriptorPoolCreateInfo descriptorPoolInfo = VulkanInitializers::DescriptorPool(poolSizes,
		sizesCount, maxSets);

	VkDescriptorPool descriptorPool;
	VkResult result = vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool);
	assert(result == VK_SUCCESS);
	Debug::Utils::SetDescriptorPoolName(device, descriptorPool, name);

	return descriptorPool;
}

void VulkanFactory::Descriptor::DestroyPool(VkDevice device, VkDescriptorPool descriptorPool)
{
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

VkDescriptorSet VulkanFactory::Descriptor::AllocateSet(VkDevice device, VkDescriptorPool descriptorPool,
	VkDescriptorSetLayout descriptorSetLayout)
{
	auto descriptorSetAllocationInitializer = VulkanInitializers::DescriptorSetAllocation(descriptorPool,
		&descriptorSetLayout, 1);

	VkDescriptorSet descriptorSet;
	VkResult result = vkAllocateDescriptorSets(device, &descriptorSetAllocationInitializer, &descriptorSet);
	assert(result == VK_SUCCESS);

	return descriptorSet;
}

void VulkanFactory::Descriptor::FreeSet(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSet descriptorSet)
{
	vkFreeDescriptorSets(device, descriptorPool, 1, &descriptorSet);
}

VkPipeline VulkanFactory::Pipeline::CreateGraphics(VkDevice device, VkRenderPass renderPass,
	VkShaderModule vertexShader, VkShaderModule fragmentShader, 
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

	auto viewportStateInitializer = VulkanInitializers::PipelineViewportState(1, 1);

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

	VkPipelineVertexInputStateCreateInfo emptyInputState = VulkanInitializers::PipelineVertexInputState(
		nullptr, 0, nullptr, 0);

	auto pipelineInitializer = VulkanInitializers::GraphicsPipeline();
	pipelineInitializer.renderPass = renderPass;
	pipelineInitializer.layout = pipelineLayout;
	pipelineInitializer.pVertexInputState = &emptyInputState;
	pipelineInitializer.pInputAssemblyState = &inputAssemblyStateInitializer;
	pipelineInitializer.pRasterizationState = &rasterizationStateInitializer;
	pipelineInitializer.pColorBlendState = &colorBlendStateInitializer;
	pipelineInitializer.pMultisampleState = &multisampleStateInitializer;
	pipelineInitializer.pViewportState = &viewportStateInitializer;
	pipelineInitializer.pDepthStencilState = &depthStencilStateInitializer;
	pipelineInitializer.pDynamicState = &dynamicStateInitializer;
	pipelineInitializer.stageCount = shaderCount;
	pipelineInitializer.pStages = shaderStages;
	pipelineInitializer.renderPass = renderPass;
	
	VkPipeline pipeline;
	VkResult result = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInitializer, nullptr, &pipeline);
	assert(result == VK_SUCCESS);
	Debug::Utils::SetPipelineName(device, pipeline, "Display Pipeline");

	return pipeline;
}

VkPipeline VulkanFactory::Pipeline::CreateGuiGraphics(VkDevice device, VkRenderPass renderPass, VkShaderModule vertexShader, VkShaderModule fragmentShader, VkPipelineLayout pipelineLayout, VkPipelineCache pipelineCache)
{
	auto inputAssemblyStateInitializer = VulkanInitializers::PipelineInputAssemblyState(
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

	auto rasterizationStateInitializer = VulkanInitializers::PipelineRasterizationState(
		VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

	auto blendAttachmentStateInitializer = VulkanInitializers::PipelineColorBlendAttachment(
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		VK_TRUE);
	blendAttachmentStateInitializer.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachmentStateInitializer.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachmentStateInitializer.colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentStateInitializer.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachmentStateInitializer.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendAttachmentStateInitializer.alphaBlendOp = VK_BLEND_OP_ADD;

	auto colorBlendStateInitializer = VulkanInitializers::PipelineColorBlendState(
		&blendAttachmentStateInitializer, 1);

	auto depthStencilStateInitializer = VulkanInitializers::PipelineDepthStencilState(
		VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);

	auto viewportStateInitializer = VulkanInitializers::PipelineViewportState(1, 1);

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

	auto vertexInputBindingDescription = VulkanInitializers::VertexInputBindingDescription(
		0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX);

	std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions =
	{
		VulkanInitializers::VertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)),
		VulkanInitializers::VertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)),
		VulkanInitializers::VertexInputAttributeDescription(0, 2, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)),
	};

	auto pipelineVertexInputStateInitializer = VulkanInitializers::PipelineVertexInputState(
		&vertexInputBindingDescription, 1, 
		vertexInputAttributeDescriptions.data(), (uint32_t)vertexInputAttributeDescriptions.size());

	auto pipelineInitializer = VulkanInitializers::GraphicsPipeline();
	pipelineInitializer.renderPass = renderPass;
	pipelineInitializer.layout = pipelineLayout;
	pipelineInitializer.pVertexInputState = &pipelineVertexInputStateInitializer;
	pipelineInitializer.pInputAssemblyState = &inputAssemblyStateInitializer;
	pipelineInitializer.pRasterizationState = &rasterizationStateInitializer;
	pipelineInitializer.pColorBlendState = &colorBlendStateInitializer;
	pipelineInitializer.pMultisampleState = &multisampleStateInitializer;
	pipelineInitializer.pViewportState = &viewportStateInitializer;
	pipelineInitializer.pDepthStencilState = &depthStencilStateInitializer;
	pipelineInitializer.pDynamicState = &dynamicStateInitializer;
	pipelineInitializer.stageCount = shaderCount;
	pipelineInitializer.pStages = shaderStages;
	pipelineInitializer.renderPass = renderPass;

	VkPipeline pipeline;
	VkResult result = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInitializer, nullptr, &pipeline);
	assert(result == VK_SUCCESS);
	Debug::Utils::SetPipelineName(device, pipeline, "GUI Pipeline");

	return pipeline;
}

VkPipeline VulkanFactory::Pipeline::CreateCompute(VkDevice device, VkPipelineLayout pipelineLayout, VkShaderModule computeShader,
	VkPipelineCache pipelineCache)
{
	auto shaderInitializer = VulkanInitializers::PipelineShaderStage(computeShader, VK_SHADER_STAGE_COMPUTE_BIT);

	auto pipelineInitializer = VulkanInitializers::ComputePipeline();
	pipelineInitializer.layout = pipelineLayout;
	pipelineInitializer.stage = shaderInitializer;

	VkPipeline pipeline;
	VkResult result = vkCreateComputePipelines(device, pipelineCache, 1, &pipelineInitializer, nullptr, &pipeline);
	assert(result == VK_SUCCESS);
	Debug::Utils::SetPipelineName(device, pipeline, "Trace Pipeline");

	return pipeline;
}

void VulkanFactory::Pipeline::Destroy(VkDevice device, VkPipeline pipeline)
{
	vkDestroyPipeline(device, pipeline, nullptr);
}

VkPipelineLayout VulkanFactory::Pipeline::CreateLayout(const char* name, VkDevice device, VkDescriptorSetLayout descriptorSetLayout,
	VkPushConstantRange* pushConstantRanges, uint32_t rangesCount)
{
	auto pipelineLayoutInitializer = VulkanInitializers::PipelineLayout(&descriptorSetLayout, 1,
		pushConstantRanges, rangesCount);

	VkPipelineLayout pipelineLayout;
	VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInitializer, nullptr, &pipelineLayout);
	assert(result == VK_SUCCESS);
	Debug::Utils::SetPipelineLayoutName(device, pipelineLayout, name);

	return pipelineLayout;
}

void VulkanFactory::Pipeline::DestroyLayout(VkDevice device, VkPipelineLayout pipelineLayout)
{
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

VkPipelineCache VulkanFactory::Pipeline::CreateCache(const char* name, VkDevice device)
{
	auto pipelineCacheInitializer = VulkanInitializers::PipelineCache();
	VkPipelineCache pipelineCache;
	VkResult result = vkCreatePipelineCache(device, &pipelineCacheInitializer, nullptr, &pipelineCache);
	assert(result == VK_SUCCESS);
	Debug::Utils::SetPipelineCacheName(device, pipelineCache, name);

	return pipelineCache;
}

void VulkanFactory::Pipeline::DestroyCache(VkDevice device, VkPipelineCache pipelineCache)
{
	vkDestroyPipelineCache(device, pipelineCache, nullptr);
}

VkShaderModule VulkanFactory::Shader::Create(const char* name, VkDevice device, const std::string& path)
{
	std::vector<uint8_t> byteCode;

	{
		size_t size = CoreFilesystem.GetFileSize(path);
		byteCode.resize(size);
		CoreFilesystem.ReadFile(path, byteCode.data(), size);
	}

	return Create(name, device, (uint32_t* const)byteCode.data(), (uint32_t)byteCode.size());
}

VkShaderModule VulkanFactory::Shader::Create(const char* name, VkDevice device, uint32_t* const data, uint32_t size)
{
	auto shaderInitializer = VulkanInitializers::Shader();
	shaderInitializer.codeSize = size;
	shaderInitializer.pCode = data;

	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(device, &shaderInitializer, nullptr, &shaderModule);
	assert(result == VK_SUCCESS);
	Debug::Utils::SetShaderModuleName(device, shaderModule, name);

	return shaderModule;
}

void VulkanFactory::Shader::Destroy(VkDevice device, VkShaderModule shader)
{
	vkDestroyShaderModule(device, shader, nullptr);
}
