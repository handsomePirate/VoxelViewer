#include "Debug.hpp"

static PFN_vkSetDebugUtilsObjectNameEXT VkSetDebugUtilsObjectNameEXT = nullptr;

void Debug::Utils::Start(VkInstance instance)
{
    VkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
        vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT"));
}

void Debug::Utils::SetObjectName(VkDevice device, uint64_t object, VkObjectType objectType, const char* name)
{
    if (VkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT objectNameInfo{};
        objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        objectNameInfo.objectHandle = object;
        objectNameInfo.objectType = objectType;
        objectNameInfo.pObjectName = name;

        VkSetDebugUtilsObjectNameEXT(device, &objectNameInfo);
    }
}

void Debug::Utils::SetInstanceName(VkDevice device, VkInstance instance, const char* name)
{
    SetObjectName(device, (uint64_t)instance, VK_OBJECT_TYPE_INSTANCE, name);
}

void Debug::Utils::SetPhysicalDeviceName(VkDevice device, VkPhysicalDevice physicalDevice, const char* name)
{
    SetObjectName(device, (uint64_t)physicalDevice, VK_OBJECT_TYPE_PHYSICAL_DEVICE, name);
}

void Debug::Utils::SetDeviceName(VkDevice device, const char* name)
{
    SetObjectName(device, (uint64_t)device, VK_OBJECT_TYPE_DEVICE, name);
}

void Debug::Utils::SetQueueName(VkDevice device, VkQueue queue, const char* name)
{
    SetObjectName(device, (uint64_t)queue, VK_OBJECT_TYPE_QUEUE, name);
}

void Debug::Utils::SetSemaphoreName(VkDevice device, VkSemaphore semaphore, const char* name)
{
    SetObjectName(device, (uint64_t)semaphore, VK_OBJECT_TYPE_SEMAPHORE, name);
}

void Debug::Utils::SetCommandBufferName(VkDevice device, VkCommandBuffer commandBuffer, const char* name)
{
    SetObjectName(device, (uint64_t)commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, name);
}

void Debug::Utils::SetFenceName(VkDevice device, VkFence fence, const char* name)
{
    SetObjectName(device, (uint64_t)fence, VK_OBJECT_TYPE_FENCE, name);
}

void Debug::Utils::SetDeviceMemoryName(VkDevice device, VkDeviceMemory deviceMemory, const char* name)
{
    SetObjectName(device, (uint64_t)deviceMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, name);
}

void Debug::Utils::SetBufferName(VkDevice device, VkBuffer buffer, const char* name)
{
    SetObjectName(device, (uint64_t)buffer, VK_OBJECT_TYPE_BUFFER, name);
}

void Debug::Utils::SetImageName(VkDevice device, VkImage image, const char* name)
{
    SetObjectName(device, (uint64_t)image, VK_OBJECT_TYPE_IMAGE, name);
}

void Debug::Utils::SetEventName(VkDevice device, VkEvent event, const char* name)
{
    SetObjectName(device, (uint64_t)event, VK_OBJECT_TYPE_EVENT, name);
}

void Debug::Utils::SetQueryPoolName(VkDevice device, VkQueryPool queryPool, const char* name)
{
    SetObjectName(device, (uint64_t)queryPool, VK_OBJECT_TYPE_QUERY_POOL, name);
}

void Debug::Utils::SetBufferViewName(VkDevice device, VkBufferView bufferView, const char* name)
{
    SetObjectName(device, (uint64_t)bufferView, VK_OBJECT_TYPE_BUFFER_VIEW, name);
}

void Debug::Utils::SetImageViewName(VkDevice device, VkImageView imageView, const char* name)
{
    SetObjectName(device, (uint64_t)imageView, VK_OBJECT_TYPE_IMAGE_VIEW, name);
}

void Debug::Utils::SetShaderModuleName(VkDevice device, VkShaderModule shaderModule, const char* name)
{
    SetObjectName(device, (uint64_t)shaderModule, VK_OBJECT_TYPE_SHADER_MODULE, name);
}

void Debug::Utils::SetPipelineCacheName(VkDevice device, VkPipelineCache pipelineCache, const char* name)
{
    SetObjectName(device, (uint64_t)pipelineCache, VK_OBJECT_TYPE_PIPELINE_CACHE, name);
}

void Debug::Utils::SetPipelineLayoutName(VkDevice device, VkPipelineLayout pipelineLayout, const char* name)
{
    SetObjectName(device, (uint64_t)pipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, name);
}

void Debug::Utils::SetRenderPassName(VkDevice device, VkRenderPass renderPass, const char* name)
{
    SetObjectName(device, (uint64_t)renderPass, VK_OBJECT_TYPE_RENDER_PASS, name);
}

void Debug::Utils::SetPipelineName(VkDevice device, VkPipeline pipeline, const char* name)
{
    SetObjectName(device, (uint64_t)pipeline, VK_OBJECT_TYPE_PIPELINE, name);
}

void Debug::Utils::SetDescriptorSetLayoutName(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const char* name)
{
    SetObjectName(device, (uint64_t)descriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, name);
}

void Debug::Utils::SetSamplerName(VkDevice device, VkSampler sampler, const char* name)
{
    SetObjectName(device, (uint64_t)sampler, VK_OBJECT_TYPE_SAMPLER, name);
}

void Debug::Utils::SetDescriptorPoolName(VkDevice device, VkDescriptorPool descriptorPool, const char* name)
{
    SetObjectName(device, (uint64_t)descriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, name);
}

void Debug::Utils::SetDescriptorSetName(VkDevice device, VkDescriptorSet descriptorSet, const char* name)
{
    SetObjectName(device, (uint64_t)descriptorSet, VK_OBJECT_TYPE_DESCRIPTOR_SET, name);
}

void Debug::Utils::SetFramebufferName(VkDevice device, VkFramebuffer framebuffer, const char* name)
{
    SetObjectName(device, (uint64_t)framebuffer, VK_OBJECT_TYPE_FRAMEBUFFER, name);
}

void Debug::Utils::SetCommandPoolName(VkDevice device, VkCommandPool commandPool, const char* name)
{
    SetObjectName(device, (uint64_t)commandPool, VK_OBJECT_TYPE_COMMAND_POOL, name);
}

void Debug::Utils::SetSurfaceName(VkDevice device, VkSurfaceKHR surface, const char* name)
{
    SetObjectName(device, (uint64_t)surface, VK_OBJECT_TYPE_SURFACE_KHR, name);
}

void Debug::Utils::SetSwapchainName(VkDevice device, VkSwapchainKHR swapchain, const char* name)
{
    SetObjectName(device, (uint64_t)swapchain, VK_OBJECT_TYPE_SWAPCHAIN_KHR, name);
}

void Debug::Utils::SetDisplayName(VkDevice device, VkDisplayKHR display, const char* name)
{
    SetObjectName(device, (uint64_t)display, VK_OBJECT_TYPE_DISPLAY_KHR, name);
}

void Debug::Utils::SetDisplayModeName(VkDevice device, VkDisplayModeKHR displayMode, const char* name)
{
    SetObjectName(device, (uint64_t)displayMode, VK_OBJECT_TYPE_DISPLAY_MODE_KHR, name);
}

void Debug::Utils::SetDebugReportCallbackName(VkDevice device, VkDebugReportCallbackEXT debugReportCallback, const char* name)
{
    SetObjectName(device, (uint64_t)debugReportCallback, VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT, name);
}

void Debug::Utils::SetDebugUtilsMessengerName(VkDevice device, VkDebugUtilsMessengerEXT debugUtilsMessenger, const char* name)
{
    SetObjectName(device, (uint64_t)debugUtilsMessenger, VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT, name);
}

void Debug::Utils::SetAccelerationStructureName(VkDevice device, VkAccelerationStructureKHR accelerationStructure, const char* name)
{
    SetObjectName(device, (uint64_t)accelerationStructure, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, name);
}

void Debug::Utils::SetAccelerationStructureName(VkDevice device, VkAccelerationStructureNV accelerationStructure, const char* name)
{
    SetObjectName(device, (uint64_t)accelerationStructure, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV, name);
}

void Debug::Utils::SetValidationCacheName(VkDevice device, VkValidationCacheEXT validationCache, const char* name)
{
    SetObjectName(device, (uint64_t)validationCache, VK_OBJECT_TYPE_VALIDATION_CACHE_EXT, name);
}
