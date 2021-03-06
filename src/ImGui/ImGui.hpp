#pragma once
#include "Core/Common.hpp"
#include "Vulkan/VulkanFactory.hpp"
#include "Vulkan/Utils.hpp"
#include "Vulkan/Camera.hpp"
#include "Vulkan/CuttingPlanes.hpp"
#include "Core/Events/EventSystem.hpp"
#include "Core/Platform/Platform.hpp"
#include <imgui.h>

namespace GUI
{
	enum class EditingTool
	{
		Brush,
		Copy,
		Fill,
		Pick,
		ToolCount
	};

	class Renderer
	{
	public:
		static void Init(uint64_t windowHandle);
		static void Shutdown();
		static bool Update(const VulkanFactory::Device::DeviceInfo& deviceInfo,
			VulkanFactory::Buffer::BufferInfo& guiVertexBuffer, VulkanFactory::Buffer::BufferInfo& guiIndexBuffer,
			Core::Window* const window, float renderTimeDelta, float fps, Camera& camera,
			TracingParameters& tracingParameters, CuttingPlanes& cuttingPlanes, float& mouseSensitivity,
			Eigen::Vector3f& editColor, Eigen::Vector3i& selectedVoxel, EditingTool& tool);
		static void Draw(VkCommandBuffer commandBuffer, VkPipeline pipeline, VkPipelineLayout pipelineLayout,
			VkDescriptorSet descriptorSet, VulkanUtils::PushConstantBlock* pushConstantBlock,
			const VulkanFactory::Buffer::BufferInfo& vertexBuffer, const VulkanFactory::Buffer::BufferInfo& indexBuffer);
		static bool WantMouseCapture();
		static bool WantKeyboardCapture();
	};
}
