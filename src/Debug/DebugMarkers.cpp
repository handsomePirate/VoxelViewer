#include "Debug.hpp"

static PFN_vkDebugMarkerSetObjectTagEXT DebugMarkerSetObjectTag;
static PFN_vkDebugMarkerSetObjectNameEXT DebugMarkerSetObjectName;
static PFN_vkCmdDebugMarkerBeginEXT CmdDebugMarkerBegin;
static PFN_vkCmdDebugMarkerEndEXT CmdDebugMarkerEnd;
static PFN_vkCmdDebugMarkerInsertEXT CmdDebugMarkerInsert;

void Debug::Markers::Setup(VkDevice device)
{
	DebugMarkerSetObjectTag = (PFN_vkDebugMarkerSetObjectTagEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectTagEXT");
	DebugMarkerSetObjectName = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT");
	CmdDebugMarkerBegin = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT");
	CmdDebugMarkerEnd = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT");
	CmdDebugMarkerInsert = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT");

	// Set flag if at least one function pointer is present
	assert(DebugMarkerSetObjectName != VK_NULL_HANDLE);
}

void Debug::Markers::SetObjectName(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, const char* name)
{
	if (DebugMarkerSetObjectName)
	{
		VkDebugMarkerObjectNameInfoEXT nameInfo = {};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = objectType;
		nameInfo.object = object;
		nameInfo.pObjectName = name;
		DebugMarkerSetObjectName(device, &nameInfo);
	}
}

void Debug::Markers::SetObjectTag(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, uint64_t name,
	size_t tagSize, const void* tag)
{
	// Check for valid function pointer (may not be present if not running in a debugging application)
	if (DebugMarkerSetObjectTag)
	{
		VkDebugMarkerObjectTagInfoEXT tagInfo = {};
		tagInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT;
		tagInfo.objectType = objectType;
		tagInfo.object = object;
		tagInfo.tagName = name;
		tagInfo.tagSize = tagSize;
		tagInfo.pTag = tag;
		DebugMarkerSetObjectTag(device, &tagInfo);
	}
}

void Debug::Markers::BeginMarkedRegion(VkCommandBuffer cmdbuffer, const char* pMarkerName, float color[4])
{
	// Check for valid function pointer (may not be present if not running in a debugging application)
	if (CmdDebugMarkerBegin)
	{
		VkDebugMarkerMarkerInfoEXT markerInfo = {};
		markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
		memcpy(markerInfo.color, color, sizeof(float) * 4);
		markerInfo.pMarkerName = pMarkerName;
		CmdDebugMarkerBegin(cmdbuffer, &markerInfo);
	}
}

void Debug::Markers::InsertMarker(VkCommandBuffer cmdbuffer, std::string markerName, float color[4])
{
	// Check for valid function pointer (may not be present if not running in a debugging application)
	if (CmdDebugMarkerInsert)
	{
		VkDebugMarkerMarkerInfoEXT markerInfo = {};
		markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
		memcpy(markerInfo.color, color, sizeof(float) * 4);
		markerInfo.pMarkerName = markerName.c_str();
		CmdDebugMarkerInsert(cmdbuffer, &markerInfo);
	}
}

void Debug::Markers::EndMarkedRegion(VkCommandBuffer cmdBuffer)
{
	// Check for valid function (may not be present if not runnin in a debugging application)
	if (CmdDebugMarkerEnd)
	{
		CmdDebugMarkerEnd(cmdBuffer);
	}
}

void Debug::Markers::SetCommandBufferName(VkDevice device, VkCommandBuffer cmdBuffer, const char* name)
{
	SetObjectName(device, (uint64_t)cmdBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, name);
}

void Debug::Markers::SetQueueName(VkDevice device, VkQueue queue, const char* name)
{
	SetObjectName(device, (uint64_t)queue, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT, name);
}

void Debug::Markers::SetImageName(VkDevice device, VkImage image, const char* name)
{
	SetObjectName(device, (uint64_t)image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, name);
}

void Debug::Markers::SetSamplerName(VkDevice device, VkSampler sampler, const char* name)
{
	SetObjectName(device, (uint64_t)sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, name);
}

void Debug::Markers::SetBufferName(VkDevice device, VkBuffer buffer, const char* name)
{
	SetObjectName(device, (uint64_t)buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, name);
}

void Debug::Markers::SetDeviceMemoryName(VkDevice device, VkDeviceMemory memory, const char* name)
{
	SetObjectName(device, (uint64_t)memory, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, name);
}

void Debug::Markers::SetShaderModuleName(VkDevice device, VkShaderModule shaderModule, const char* name)
{
	SetObjectName(device, (uint64_t)shaderModule, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, name);
}

void Debug::Markers::SetPipelineName(VkDevice device, VkPipeline pipeline, const char* name)
{
	SetObjectName(device, (uint64_t)pipeline, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, name);
}

void Debug::Markers::SetPipelineLayoutName(VkDevice device, VkPipelineLayout pipelineLayout, const char* name)
{
	SetObjectName(device, (uint64_t)pipelineLayout, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, name);
}

void Debug::Markers::SetRenderPassName(VkDevice device, VkRenderPass renderPass, const char* name)
{
	SetObjectName(device, (uint64_t)renderPass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, name);
}

void Debug::Markers::SetFramebufferName(VkDevice device, VkFramebuffer framebuffer, const char* name)
{
	SetObjectName(device, (uint64_t)framebuffer, VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT, name);
}

void Debug::Markers::SetDescriptorSetLayoutName(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const char* name)
{
	SetObjectName(device, (uint64_t)descriptorSetLayout, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, name);
}

void Debug::Markers::SetDescriptorSetName(VkDevice device, VkDescriptorSet descriptorSet, const char* name)
{
	SetObjectName(device, (uint64_t)descriptorSet, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, name);
}

void Debug::Markers::SetSemaphoreName(VkDevice device, VkSemaphore semaphore, const char* name)
{
	SetObjectName(device, (uint64_t)semaphore, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT, name);
}

void Debug::Markers::SetFenceName(VkDevice device, VkFence fence, const char* name)
{
	SetObjectName(device, (uint64_t)fence, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, name);
}

void Debug::Markers::SetEventName(VkDevice device, VkEvent _event, const char* name)
{
	SetObjectName(device, (uint64_t)_event, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT, name);
}
