#pragma once
#include "Core/Common.hpp"
#include "Vulkan/VulkanFactory.hpp"
#include "Vulkan/Utils.hpp"
#include <imgui.h>

namespace GUI
{
	class Renderer
	{
	public:
		static bool Update(const VulkanFactory::Device::DeviceInfo& deviceInfo,
			VulkanFactory::Buffer::BufferInfo& guiVertexBuffer, VulkanFactory::Buffer::BufferInfo& guiIndexBuffer,
			float width, float height, float renderTimeDelta, float fps);
		static void Draw(VkCommandBuffer commandBuffer, VkPipeline pipeline, VkPipelineLayout pipelineLayout,
			VkDescriptorSet descriptorSet, VulkanUtils::PushConstantBlock* pushConstantBlock,
			const VulkanFactory::Buffer::BufferInfo& vertexBuffer, const VulkanFactory::Buffer::BufferInfo& indexBuffer);
	};
}
