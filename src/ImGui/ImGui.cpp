#include "ImGui.hpp"
#include "Vulkan/Utils.hpp"
#include "Core/Logger/Logger.hpp"

bool UpdateInternal(const VulkanFactory::Device::DeviceInfo& deviceInfo,
	VulkanFactory::Buffer::BufferInfo& vertexBuffer, VulkanFactory::Buffer::BufferInfo& indexBuffer);

bool GUI::Renderer::Update(const VulkanFactory::Device::DeviceInfo& deviceInfo,
	VulkanFactory::Buffer::BufferInfo& guiVertexBuffer, VulkanFactory::Buffer::BufferInfo& guiIndexBuffer,
	float width, float height, float renderTimeDelta, float fps)
{
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(width, height);
	io.DeltaTime = renderTimeDelta;

	// TODO: Mouse position and buttons.

	ImGui::NewFrame();

	ImGui::Begin("Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("Device: %s", deviceInfo.Properties.deviceName);
	ImGui::Text("%.2f ms per frame", (1000.0f / fps), fps);
	ImGui::Text("(%.1f fps)", fps);
	ImGui::End();

	ImGui::Render();

	return UpdateInternal(deviceInfo, guiVertexBuffer, guiIndexBuffer);
}

bool UpdateInternal(const VulkanFactory::Device::DeviceInfo& deviceInfo,
	VulkanFactory::Buffer::BufferInfo& vertexBuffer, VulkanFactory::Buffer::BufferInfo& indexBuffer)
{
	static int vertexCount = 0;
	static int indexCount = 0;

	ImDrawData* imDrawData = ImGui::GetDrawData();
	bool updateCmdBuffers = false;

	if (!imDrawData)
	{
		return false;
	}

	// Note: Alignment is done inside buffer creation
	VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
	VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

	// Update buffers only if vertex or index count has been changed compared to current buffer size
	if (vertexBufferSize == 0 || indexBufferSize == 0)
	{
		return false;
	}
	/*
	std::vector<ImDrawVert> test;
	test.resize(vertexCount);
	void* mappedHandle = VulkanUtils::Memory::Map(deviceInfo.Handle, vertexBuffer.Memory,
		vertexBuffer.DescriptorBufferInfo.offset, vertexBuffer.Size);
	memcpy(test.data(), mappedHandle, vertexCount * sizeof(ImDrawVert));
	VulkanUtils::Memory::Unmap(deviceInfo.Handle, vertexBuffer.Memory);
	*/
	// Vertex buffer
	if (vertexBuffer.DescriptorBufferInfo.buffer == VK_NULL_HANDLE || vertexCount < imDrawData->TotalVtxCount)
	{
		if (vertexBuffer.DescriptorBufferInfo.buffer != VK_NULL_HANDLE)
		{
			VulkanFactory::Buffer::Destroy(deviceInfo, vertexBuffer);
		}
		VulkanFactory::Buffer::Create(deviceInfo, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBufferSize,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, vertexBuffer);
		vertexCount = imDrawData->TotalVtxCount;
		updateCmdBuffers = true;
	}

	// Index buffer
	VkDeviceSize indexSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);
	if (indexBuffer.DescriptorBufferInfo.buffer == VK_NULL_HANDLE || indexCount < imDrawData->TotalIdxCount)
	{
		if (indexBuffer.DescriptorBufferInfo.buffer != VK_NULL_HANDLE)
		{
			VulkanFactory::Buffer::Destroy(deviceInfo, indexBuffer);
		}
		VulkanFactory::Buffer::Create(deviceInfo, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBufferSize,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, indexBuffer);
		indexCount = imDrawData->TotalIdxCount;
		updateCmdBuffers = true;
	}

	// Upload data
	void* vertexMapping = VulkanUtils::Memory::Map(deviceInfo.Handle, vertexBuffer.Memory,
		vertexBuffer.DescriptorBufferInfo.offset, vertexBuffer.Size);
	void* indexMapping = VulkanUtils::Memory::Map(deviceInfo.Handle, indexBuffer.Memory,
		indexBuffer.DescriptorBufferInfo.offset, indexBuffer.Size);
	ImDrawVert* vtxDst = (ImDrawVert*)vertexMapping;
	ImDrawIdx* idxDst = (ImDrawIdx*)indexMapping;

	for (int n = 0; n < imDrawData->CmdListsCount; ++n)
	{
		const ImDrawList* cmd_list = imDrawData->CmdLists[n];
		memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtxDst += cmd_list->VtxBuffer.Size;
		idxDst += cmd_list->IdxBuffer.Size;
	}

	// Flush to make writes visible to GPU
	VulkanUtils::Memory::Flush(deviceInfo.Handle, vertexBuffer.Memory);
	VulkanUtils::Memory::Flush(deviceInfo.Handle, indexBuffer.Memory);

	VulkanUtils::Memory::Unmap(deviceInfo.Handle, vertexBuffer.Memory);
	VulkanUtils::Memory::Unmap(deviceInfo.Handle, indexBuffer.Memory);
	
	return updateCmdBuffers;
}

void GUI::Renderer::Draw(VkCommandBuffer commandBuffer, VkPipeline pipeline, VkPipelineLayout pipelineLayout,
	VkDescriptorSet descriptorSet, VulkanUtils::PushConstantBlock* pushConstantBlock,
	const VulkanFactory::Buffer::BufferInfo& vertexBuffer, const VulkanFactory::Buffer::BufferInfo& indexBuffer)
{
	ImDrawData* imDrawData = ImGui::GetDrawData();
	int32_t vertexOffset = 0;
	int32_t indexOffset = 0;

	if (!imDrawData || imDrawData->CmdListsCount == 0)
	{
		return;
	}

	ImGuiIO& io = ImGui::GetIO();

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

	pushConstantBlock->ScaleX = 2.f / io.DisplaySize.x;
	pushConstantBlock->ScaleY = 2.f / io.DisplaySize.y;
	pushConstantBlock->TranslationX = -1.f;
	pushConstantBlock->TranslationY = -1.f;

	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
		sizeof(VulkanUtils::PushConstantBlock), pushConstantBlock);

	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.DescriptorBufferInfo.buffer, &offset);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer.DescriptorBufferInfo.buffer, 0, VK_INDEX_TYPE_UINT16);

	for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
	{
		const ImDrawList* cmd_list = imDrawData->CmdLists[i];
		for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];

			VkRect2D scissor;
			scissor.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
			scissor.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
			scissor.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
			scissor.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);

			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
			vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);

			indexOffset += pcmd->ElemCount;
		}
		vertexOffset += cmd_list->VtxBuffer.Size;
	}
}