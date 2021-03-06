#pragma once
#include "Core/Common.hpp"
#include "Utils.hpp"

namespace VulkanInitializers
{
	inline VkApplicationInfo ApplicationInfo()
	{
		VkApplicationInfo applicationInfo{};
		applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		return applicationInfo;
	}

	inline VkInstanceCreateInfo Instance(const VkApplicationInfo* appInfo)
	{
		VkInstanceCreateInfo instanceCreateInfo{};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pApplicationInfo = appInfo;
		return instanceCreateInfo;
	}

	inline VkDeviceQueueCreateInfo Queue(uint32_t index, const float defaultPriority = 0.f)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.queueFamilyIndex = index;
		return queueCreateInfo;
	}

	inline VkDeviceQueueCreateInfo Queue(const std::vector<VkQueueFamilyProperties>& queueProperties, 
		VkQueueFlags flags)
	{
		uint32_t index = VulkanUtils::Queue::GetQueueFamilyIndex(queueProperties, flags);

		VkDeviceQueueCreateInfo queueCreateInfo = Queue(index);
		return queueCreateInfo;
	}

	inline VkDeviceCreateInfo Device()
	{
		VkDeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		return deviceCreateInfo;
	}

	inline VkCommandPoolCreateInfo CommandPool()
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo{};
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		return commandPoolCreateInfo;
	}

	inline VkSemaphoreCreateInfo Semaphore()
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo{};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		return semaphoreCreateInfo;
	}

	inline VkFenceCreateInfo Fence()
	{
		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		return fenceCreateInfo;
	}

	inline VkSubmitInfo SubmitInfo()
	{
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		return submitInfo;
	}

	inline VkSwapchainCreateInfoKHR Swapchain(VkExtent2D extent, VkSurfaceKHR surface)
	{
		VkSwapchainCreateInfoKHR swapchainInfo{};
		swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainInfo.clipped = VK_TRUE;
		swapchainInfo.imageArrayLayers = 1;
		swapchainInfo.imageExtent = extent;
		swapchainInfo.surface = surface;
		return swapchainInfo;
	}

	inline VkImageViewCreateInfo ImageView(VkImage image, VkFormat format)
	{
		VkImageViewCreateInfo imageView = {};
		imageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageView.image = image;
		imageView.format = format;
		return imageView;
	}

	inline VkSamplerCreateInfo Sampler()
	{
		VkSamplerCreateInfo samplerCreateInfo{};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.maxAnisotropy = 1.0f;
		return samplerCreateInfo;
	}

	inline VkImageViewCreateInfo ColorAttachmentView(VkImage image, VkFormat format)
	{
		VkImageViewCreateInfo colorAttachmentView = ImageView(image, format);
		
		colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		colorAttachmentView.components =
		{
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A
		};
		colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorAttachmentView.subresourceRange.levelCount = 1;
		colorAttachmentView.subresourceRange.layerCount = 1;
		colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;

		return colorAttachmentView;
	}

	inline VkImageViewCreateInfo DepthAttachmentView(VkImage image, VkFormat format)
	{
		VkImageViewCreateInfo depthAttachmentView = ImageView(image, format);

		depthAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		depthAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		depthAttachmentView.subresourceRange.levelCount = 1;
		depthAttachmentView.subresourceRange.layerCount = 1;
		depthAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;

		if (format >= VK_FORMAT_D16_UNORM_S8_UINT)
		{
			depthAttachmentView.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		return depthAttachmentView;
	}

	inline VkCommandBufferAllocateInfo CommandBufferAllocation(VkCommandPool commandPool,
		VkCommandBufferLevel level, uint32_t bufferCount)
	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandPool = commandPool;
		commandBufferAllocateInfo.level = level;
		commandBufferAllocateInfo.commandBufferCount = bufferCount;
		return commandBufferAllocateInfo;
	}

	inline VkCommandBufferBeginInfo CommandBufferBeginning()
	{
		VkCommandBufferBeginInfo commandBufferBeginInfo{};
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		return commandBufferBeginInfo;
	}

	inline VkImageCreateInfo Image(VkFormat format)
	{
		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		return imageCreateInfo;
	}

	inline VkMemoryAllocateInfo MemoryAllocation(VkDeviceSize size, uint32_t typeIndex)
	{
		VkMemoryAllocateInfo memoryAllocateInfo{};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.allocationSize = size;
		memoryAllocateInfo.memoryTypeIndex = typeIndex;
		return memoryAllocateInfo;
	}

	inline VkFramebufferCreateInfo Framebuffer()
	{
		VkFramebufferCreateInfo framebufferCreateInfo{};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		return framebufferCreateInfo;
	}

	inline VkPipelineCacheCreateInfo PipelineCache()
	{
		VkPipelineCacheCreateInfo pipelineCacheInfo{};
		pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		return pipelineCacheInfo;
	}

	inline VkShaderModuleCreateInfo Shader()
	{
		VkShaderModuleCreateInfo shaderModuleCreateInfo{};
		shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		return shaderModuleCreateInfo;
	}

	inline VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyState(VkPrimitiveTopology primitiveTopology,
		VkBool32 primitiveRestartEnable, VkPipelineInputAssemblyStateCreateFlags flags = 0)
	{
		VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyCreateInfo{};
		pipelineInputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		pipelineInputAssemblyCreateInfo.topology = primitiveTopology;
		pipelineInputAssemblyCreateInfo.primitiveRestartEnable = primitiveRestartEnable;
		pipelineInputAssemblyCreateInfo.flags = flags;
		return pipelineInputAssemblyCreateInfo;
	}

	inline VkPipelineRasterizationStateCreateInfo PipelineRasterizationState(VkPolygonMode polygonMode,
		VkCullModeFlags cullMode, VkFrontFace frontFace, VkPipelineRasterizationStateCreateFlags flags = 0)
	{
		VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{};
		pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		pipelineRasterizationStateCreateInfo.polygonMode = polygonMode;
		pipelineRasterizationStateCreateInfo.cullMode = cullMode;
		pipelineRasterizationStateCreateInfo.frontFace = frontFace;
		pipelineRasterizationStateCreateInfo.flags = flags;
		pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
		return pipelineRasterizationStateCreateInfo;
	}

	inline VkPipelineColorBlendAttachmentState PipelineColorBlendAttachment(VkColorComponentFlags colorWriteMask,
		VkBool32 blendEnable)
	{
		VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
		pipelineColorBlendAttachmentState.colorWriteMask = colorWriteMask;
		pipelineColorBlendAttachmentState.blendEnable = blendEnable;
		return pipelineColorBlendAttachmentState;
	}

	inline VkPipelineColorBlendStateCreateInfo PipelineColorBlendState(
		VkPipelineColorBlendAttachmentState* pipelineColorBlendAttachments, uint32_t blendAttachmentCount)
	{
		VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{};
		pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		pipelineColorBlendStateCreateInfo.attachmentCount = blendAttachmentCount;
		pipelineColorBlendStateCreateInfo.pAttachments = pipelineColorBlendAttachments;
		return pipelineColorBlendStateCreateInfo;
	}

	inline VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilState(VkBool32 depthTestEnable,
		VkBool32 depthWriteEnable, VkCompareOp depthCompareOperation)
	{
		VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{};
		pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		pipelineDepthStencilStateCreateInfo.depthTestEnable = depthTestEnable;
		pipelineDepthStencilStateCreateInfo.depthWriteEnable = depthWriteEnable;
		pipelineDepthStencilStateCreateInfo.depthCompareOp = depthCompareOperation;
		return pipelineDepthStencilStateCreateInfo;
	}

	inline VkPipelineViewportStateCreateInfo PipelineViewportState(uint32_t viewportCount, uint32_t scissorCount,
		VkPipelineViewportStateCreateFlags flags = 0)
	{
		VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo{};
		pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		pipelineViewportStateCreateInfo.viewportCount = viewportCount;
		pipelineViewportStateCreateInfo.scissorCount = scissorCount;
		pipelineViewportStateCreateInfo.flags = flags;
		return pipelineViewportStateCreateInfo;
	}

	inline VkPipelineMultisampleStateCreateInfo PipelineMultisampleState(VkSampleCountFlagBits sampleCount,
		VkPipelineMultisampleStateCreateFlags flags = 0)
	{
		VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{};
		pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		pipelineMultisampleStateCreateInfo.rasterizationSamples = sampleCount;
		pipelineMultisampleStateCreateInfo.flags = flags;
		pipelineMultisampleStateCreateInfo.minSampleShading = 0.3f;
		return pipelineMultisampleStateCreateInfo;
	}

	inline VkPipelineDynamicStateCreateInfo PipelineDynamicState(VkDynamicState* dynamicStates, uint32_t stateCount)
	{
		VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
		pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		pipelineDynamicStateCreateInfo.dynamicStateCount = stateCount;
		pipelineDynamicStateCreateInfo.pDynamicStates = dynamicStates;
		return pipelineDynamicStateCreateInfo;
	}

	inline VkPipelineShaderStageCreateInfo PipelineShaderStage(VkShaderModule shader, VkShaderStageFlagBits stage)
	{
		VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo{};
		pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pipelineShaderStageCreateInfo.module = shader;
		pipelineShaderStageCreateInfo.pName = "main";
		pipelineShaderStageCreateInfo.stage = stage;
		return pipelineShaderStageCreateInfo;
	}

	inline VkVertexInputBindingDescription VertexInputBindingDescription(uint32_t binding, uint32_t stride,
		VkVertexInputRate inputRate)
	{
		VkVertexInputBindingDescription vertexInputBindingDescription{};
		vertexInputBindingDescription.binding = binding;
		vertexInputBindingDescription.stride = stride;
		vertexInputBindingDescription.inputRate = inputRate;
		return vertexInputBindingDescription;
	}

	inline VkVertexInputAttributeDescription VertexInputAttributeDescription(uint32_t binding, uint32_t location,
		VkFormat format, uint32_t offset)
	{
		VkVertexInputAttributeDescription vertexInputAttributeDescription{};
		vertexInputAttributeDescription.location = location;
		vertexInputAttributeDescription.binding = binding;
		vertexInputAttributeDescription.format = format;
		vertexInputAttributeDescription.offset = offset;
		return vertexInputAttributeDescription;
	}

	inline VkPipelineVertexInputStateCreateInfo PipelineVertexInputState(
		VkVertexInputBindingDescription* vertexInputBindingDescriptions, uint32_t bindingDescriptionCount,
		VkVertexInputAttributeDescription* vertexInputAttributeDescriptions, uint32_t attributeDescriptionCount)
	{
		VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
		pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = bindingDescriptionCount;
		pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = vertexInputBindingDescriptions;
		pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = attributeDescriptionCount;
		pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptions;
		return pipelineVertexInputStateCreateInfo;
	}

	inline VkGraphicsPipelineCreateInfo GraphicsPipeline()
	{
		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
		graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		return graphicsPipelineCreateInfo;
	}

	inline VkComputePipelineCreateInfo ComputePipeline()
	{
		VkComputePipelineCreateInfo computePipelineCreateInfo{};
		computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		return computePipelineCreateInfo;
	}

	inline VkRenderPassBeginInfo RenderPassBeginning(VkRenderPass renderPass, uint32_t width, uint32_t height)
	{
		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		return renderPassBeginInfo;
	}

	inline VkPresentInfoKHR Present(uint32_t swapchainCount, VkSwapchainKHR* swapchains, uint32_t* imageIndices)
	{
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.swapchainCount = swapchainCount;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = imageIndices;
		return presentInfo;
	}

	inline VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(VkDescriptorType type,
		VkShaderStageFlags stageFlags, uint32_t binding, uint32_t descriptorCount = 1)
	{
		VkDescriptorSetLayoutBinding descriptorSetLayoutBinding{};
		descriptorSetLayoutBinding.descriptorType = type;
		descriptorSetLayoutBinding.stageFlags = stageFlags;
		descriptorSetLayoutBinding.binding = binding;
		descriptorSetLayoutBinding.descriptorCount = descriptorCount;
		return descriptorSetLayoutBinding;
	}

	inline VkDescriptorSetLayoutCreateInfo DescriptorSetLayout(VkDescriptorSetLayoutBinding* descriptorSetLayoutBindings,
		uint32_t layoutCount)
	{
		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.bindingCount = layoutCount;
		descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings;
		return descriptorSetLayoutCreateInfo;
	}

	inline VkPipelineLayoutCreateInfo PipelineLayout(VkDescriptorSetLayout* descriptorSetLayouts, uint32_t layoutCount,
		VkPushConstantRange* pushConstantRanges, uint32_t rangeCount)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = layoutCount;
		pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts;
		pipelineLayoutCreateInfo.pushConstantRangeCount = rangeCount;
		pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges;
		return pipelineLayoutCreateInfo;
	}

	inline VkDescriptorPoolSize DescriptorPoolSize(VkDescriptorType descriptorType, uint32_t descriptorCount)
	{
		VkDescriptorPoolSize descriptorPoolSize{};
		descriptorPoolSize.type = descriptorType;
		descriptorPoolSize.descriptorCount = descriptorCount;
		return descriptorPoolSize;
	}

	inline VkDescriptorPoolCreateInfo DescriptorPool(VkDescriptorPoolSize* descriptorPoolSizes, uint32_t sizeCount,
		uint32_t maxSets)
	{
		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
		descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCreateInfo.poolSizeCount = sizeCount;
		descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;
		descriptorPoolCreateInfo.maxSets = maxSets;
		return descriptorPoolCreateInfo;
	}

	inline VkDescriptorSetAllocateInfo DescriptorSetAllocation(VkDescriptorPool descriptorPool,
		VkDescriptorSetLayout* descriptorSetLayouts, uint32_t setCount)
	{
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.descriptorPool = descriptorPool;
		descriptorSetAllocateInfo.pSetLayouts = descriptorSetLayouts;
		descriptorSetAllocateInfo.descriptorSetCount = setCount;
		return descriptorSetAllocateInfo;
	}

	inline VkWriteDescriptorSet WriteDescriptorSet(VkDescriptorSet set,
		VkDescriptorType type, uint32_t binding, VkDescriptorImageInfo* imageInfo,
		uint32_t descriptorCount = 1)
	{
		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = set;
		writeDescriptorSet.descriptorType = type;
		writeDescriptorSet.dstBinding = binding;
		writeDescriptorSet.pImageInfo = imageInfo;
		writeDescriptorSet.descriptorCount = descriptorCount;
		return writeDescriptorSet;
	}

	inline VkWriteDescriptorSet WriteDescriptorSet(VkDescriptorSet set,
		VkDescriptorType type, uint32_t binding, VkDescriptorBufferInfo* bufferInfo,
		uint32_t descriptorCount = 1)
	{
		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = set;
		writeDescriptorSet.descriptorType = type;
		writeDescriptorSet.dstBinding = binding;
		writeDescriptorSet.pBufferInfo = bufferInfo;
		writeDescriptorSet.descriptorCount = descriptorCount;
		return writeDescriptorSet;
	}

	inline VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags imageAspectMask)
	{
		VkImageSubresourceRange imageSubresourceRange{};
		imageSubresourceRange.aspectMask = imageAspectMask;
		imageSubresourceRange.baseMipLevel = 0;
		imageSubresourceRange.levelCount = 1;
		imageSubresourceRange.layerCount = 1;
		return imageSubresourceRange;
	}

	inline VkImageMemoryBarrier ImageMemoryBarrier()
	{
		VkImageMemoryBarrier imageMemoryBarrier{};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		return imageMemoryBarrier;
	}

	inline VkBufferCreateInfo Buffer(VkBufferUsageFlags usage, VkDeviceSize size)
	{
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.usage = usage;
		bufferCreateInfo.size = size;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		return bufferCreateInfo;
	}

	inline VkBufferCopy BufferCopy(VkDeviceSize size, VkDeviceSize sourceOffset = 0, VkDeviceSize destinationOffset = 0)
	{
		VkBufferCopy bufferCopy{};
		bufferCopy.size = size;
		bufferCopy.srcOffset = sourceOffset;
		bufferCopy.dstOffset = destinationOffset;
		return bufferCopy;
	}

	inline VkBufferImageCopy BufferImageCopy(VkImageAspectFlags aspect, uint32_t width, uint32_t height)
	{
		VkBufferImageCopy bufferImageCopy{};
		bufferImageCopy.imageSubresource.aspectMask = aspect;
		bufferImageCopy.imageSubresource.layerCount = 1;
		bufferImageCopy.imageExtent.width = width;
		bufferImageCopy.imageExtent.height = height;
		bufferImageCopy.imageExtent.depth = 1;
		return bufferImageCopy;
	}

	inline VkPushConstantRange PushConstantRange(VkShaderStageFlags stageFlags, uint32_t size, uint32_t offset)
	{
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = stageFlags;
		pushConstantRange.offset = offset;
		pushConstantRange.size = size;
		return pushConstantRange;
	}
};
