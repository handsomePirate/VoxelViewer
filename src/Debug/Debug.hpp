#pragma once
#include "Core/Common.hpp"

namespace Debug
{
	namespace ValidationLayers
	{
		bool CheckLayerPresent(const char* name);
		void Start(VkInstance instance);
		void Shutdown(VkInstance instance);
	}

	namespace Utils
	{
		void Start(VkInstance instance);

		void SetObjectName(VkDevice device, uint64_t object, VkObjectType objectType, const char* name);

		void SetInstanceName(VkDevice device, VkInstance instance, const char* name);
		void SetPhysicalDeviceName(VkDevice device, VkPhysicalDevice physicalDevice, const char* name);
		void SetDeviceName(VkDevice device, const char* name);
		void SetQueueName(VkDevice device, VkQueue queue, const char* name);
		void SetSemaphoreName(VkDevice device, VkSemaphore semaphore, const char* name);
		void SetCommandBufferName(VkDevice device, VkCommandBuffer commandBuffer, const char* name);
		void SetFenceName(VkDevice device, VkFence fence, const char* name);
		void SetDeviceMemoryName(VkDevice device, VkDeviceMemory deviceMemory, const char* name);
		void SetBufferName(VkDevice device, VkBuffer buffer, const char* name);
		void SetImageName(VkDevice device, VkImage image, const char* name);
		void SetEventName(VkDevice device, VkEvent event, const char* name);
		void SetQueryPoolName(VkDevice device, VkQueryPool queryPool, const char* name);
		void SetBufferViewName(VkDevice device, VkBufferView bufferView, const char* name);
		void SetImageViewName(VkDevice device, VkImageView imageView, const char* name);
		void SetShaderModuleName(VkDevice device, VkShaderModule shaderModule, const char* name);
		void SetPipelineCacheName(VkDevice device, VkPipelineCache pipelineCache, const char* name);
		void SetPipelineLayoutName(VkDevice device, VkPipelineLayout pipelineLayout, const char* name);
		void SetRenderPassName(VkDevice device, VkRenderPass renderPass, const char* name);
		void SetPipelineName(VkDevice device, VkPipeline pipeline, const char* name);
		void SetDescriptorSetLayoutName(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const char* name);
		void SetSamplerName(VkDevice device, VkSampler sampler, const char* name);
		void SetDescriptorPoolName(VkDevice device, VkDescriptorPool descriptorPool, const char* name);
		void SetDescriptorSetName(VkDevice device, VkDescriptorSet descriptorSet, const char* name);
		void SetFramebufferName(VkDevice device, VkFramebuffer framebuffer, const char* name);
		void SetCommandPoolName(VkDevice device, VkCommandPool commandPool, const char* name);
		void SetSurfaceName(VkDevice device, VkSurfaceKHR surface, const char* name);
		void SetSwapchainName(VkDevice device, VkSwapchainKHR swapchain, const char* name);
		void SetDisplayName(VkDevice device, VkDisplayKHR display, const char* name);
		void SetDisplayModeName(VkDevice device, VkDisplayModeKHR displayMode, const char* name);
		void SetDebugReportCallbackName(VkDevice device, VkDebugReportCallbackEXT debugReportCallback, const char* name);
		void SetDebugUtilsMessengerName(VkDevice device, VkDebugUtilsMessengerEXT debugUtilsMessenger, const char* name);
		void SetAccelerationStructureName(VkDevice device, VkAccelerationStructureKHR accelerationStructure, const char* name);
		void SetAccelerationStructureName(VkDevice device, VkAccelerationStructureNV accelerationStructure, const char* name);
		void SetValidationCacheName(VkDevice device, VkValidationCacheEXT validationCache, const char* name);
	}
}
