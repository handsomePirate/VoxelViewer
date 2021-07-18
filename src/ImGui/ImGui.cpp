#include "ImGui.hpp"
#include "Vulkan/Utils.hpp"
#include "Core/Logger/Logger.hpp"
#include "Core/Input/Input.hpp"
#include "Core/Platform/Platform.hpp"
#include "Vulkan/ShaderManager.hpp"
#include "HashDAG/HashDAG.hpp"

bool UpdateInternal(const VulkanFactory::Device::DeviceInfo& deviceInfo,
	VulkanFactory::Buffer::BufferInfo& vertexBuffer, VulkanFactory::Buffer::BufferInfo& indexBuffer);

static int GuiRendererListener = 0;

bool CharacterInput(Core::EventCode code, Core::EventData context)
{
	ImGuiIO& io = ImGui::GetIO();

	if (io.WantTextInput)
	{
		io.AddInputCharacterUTF16(context.data.u16[0]);
	}

	return false;
}

bool MouseWheel(Core::EventCode code, Core::EventData context)
{
	ImGuiIO& io = ImGui::GetIO();

	const float sensitivity = 0.5f;
	if (context.data.u8[1])
	{
		io.MouseWheelH += context.data.i8[0] * sensitivity;
	}
	else
	{
		io.MouseWheel += context.data.i8[0] * sensitivity;
	}

	return false;
}

static char inputText[256];

void GUI::Renderer::Init(uint64_t windowHandle)
{
	ImGui::CreateContext();

	ImGui::StyleColorsDark();

	ImGuiIO& io = ImGui::GetIO();

	io.KeyMap[ImGuiKey_Tab] = (int)Core::Input::Keys::Tab;
	io.KeyMap[ImGuiKey_LeftArrow] = (int)Core::Input::Keys::Left;
	io.KeyMap[ImGuiKey_RightArrow] = (int)Core::Input::Keys::Right;
	io.KeyMap[ImGuiKey_UpArrow] = (int)Core::Input::Keys::Up;
	io.KeyMap[ImGuiKey_DownArrow] = (int)Core::Input::Keys::Down;
	io.KeyMap[ImGuiKey_PageUp] = (int)Core::Input::Keys::Prior;
	io.KeyMap[ImGuiKey_PageDown] = (int)Core::Input::Keys::Next;
	io.KeyMap[ImGuiKey_Home] = (int)Core::Input::Keys::Home;
	io.KeyMap[ImGuiKey_End] = (int)Core::Input::Keys::End;
	io.KeyMap[ImGuiKey_Insert] = (int)Core::Input::Keys::Insert;
	io.KeyMap[ImGuiKey_Delete] = (int)Core::Input::Keys::Delete;
	io.KeyMap[ImGuiKey_Backspace] = (int)Core::Input::Keys::Backspace;
	io.KeyMap[ImGuiKey_Space] = (int)Core::Input::Keys::Space;
	io.KeyMap[ImGuiKey_Enter] = (int)Core::Input::Keys::Enter;
	io.KeyMap[ImGuiKey_Escape] = (int)Core::Input::Keys::Escape;
	io.KeyMap[ImGuiKey_KeyPadEnter] = (int)Core::Input::Keys::Enter;
	io.KeyMap[ImGuiKey_A] = (int)Core::Input::Keys::A;
	io.KeyMap[ImGuiKey_C] = (int)Core::Input::Keys::C;
	io.KeyMap[ImGuiKey_V] = (int)Core::Input::Keys::V;
	io.KeyMap[ImGuiKey_X] = (int)Core::Input::Keys::X;
	io.KeyMap[ImGuiKey_Y] = (int)Core::Input::Keys::Y;
	io.KeyMap[ImGuiKey_Z] = (int)Core::Input::Keys::Z;
	io.ImeWindowHandle = (void*)windowHandle;

	memset(inputText, 0, 256 * sizeof(char));

	CoreEventSystem.SubscribeToEvent(Core::EventCode::CharacterInput, &CharacterInput, &GuiRendererListener);
	CoreEventSystem.SubscribeToEvent(Core::EventCode::MouseWheel, &MouseWheel, &GuiRendererListener);
}

void GUI::Renderer::Shutdown()
{
	CoreEventSystem.UnsubscribeFromEvent(Core::EventCode::CharacterInput, &GuiRendererListener);
	CoreEventSystem.UnsubscribeFromEvent(Core::EventCode::MouseWheel, &GuiRendererListener);
}

#define UpdateImguiKey(key) io.KeysDown[(int)Core::Input::Keys::key] = CoreInput.IsKeyPressed(Core::Input::Keys::key)

bool GUI::Renderer::Update(const VulkanFactory::Device::DeviceInfo& deviceInfo,
	VulkanFactory::Buffer::BufferInfo& guiVertexBuffer, VulkanFactory::Buffer::BufferInfo& guiIndexBuffer,
	Core::Window* const window, float renderTimeDelta, float fps, Camera& camera,
	TracingParameters& tracingParameters, CuttingPlanes& cuttingPlanes, float& mouseSensitivity,
	Eigen::Vector3f& editColor, EditingTool& tool)
{
	static CuttingPlanes cuttingPlanesMinMax = cuttingPlanes;

	float deltaTimeSeconds = renderTimeDelta * .001f;
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)window->GetWidth(), (float)window->GetHeight());
	io.DeltaTime = deltaTimeSeconds;

	auto mouseX = CoreInput.GetMouseX();
	auto mouseY = CoreInput.GetMouseY();
	window->ClipMousePosition(mouseX, mouseY);
	io.MousePos = ImVec2((float)mouseX, (float)mouseY);
	io.MouseDown[0] = CoreInput.IsMouseButtonPressed(Core::Input::MouseButtons::Left);
	io.MouseDown[1] = CoreInput.IsMouseButtonPressed(Core::Input::MouseButtons::Right);
	io.MouseDown[2] = CoreInput.IsMouseButtonPressed(Core::Input::MouseButtons::Middle);
	UpdateImguiKey(Tab);
	UpdateImguiKey(Left);
	UpdateImguiKey(Right);
	UpdateImguiKey(Up);
	UpdateImguiKey(Down);
	UpdateImguiKey(Prior);
	UpdateImguiKey(Next);
	UpdateImguiKey(Home);
	UpdateImguiKey(End);
	UpdateImguiKey(Insert);
	UpdateImguiKey(Delete);
	UpdateImguiKey(Backspace);
	UpdateImguiKey(Space);
	UpdateImguiKey(Enter);
	UpdateImguiKey(Escape);
	UpdateImguiKey(A);
	UpdateImguiKey(C);
	UpdateImguiKey(V);
	UpdateImguiKey(X);
	UpdateImguiKey(Y);
	UpdateImguiKey(Z);
	io.KeysDown[ImGuiKey_KeyPadEnter] = CoreInput.IsKeyPressed(Core::Input::Keys::Enter);
	io.KeyShift = CoreInput.IsKeyPressed(Core::Input::Keys::Shift);
	io.KeyCtrl = CoreInput.IsKeyPressed(Core::Input::Keys::Control);
	io.KeyAlt = CoreInput.IsKeyPressed(Core::Input::Keys::Alt);

	static Core::CursorType lastCursor = Core::CursorType::CursorTypeCount;
	Core::CursorType currentCursor = (Core::CursorType)ImGui::GetMouseCursor();
	if (lastCursor != currentCursor)
	{
		CorePlatform.SetCursor(currentCursor);
		lastCursor = currentCursor;
	}

	ImGui::NewFrame();

	if (!ImGui::Begin("Info & Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::End();
	}
	else
	{
		ImGui::Text("Device: %s", deviceInfo.Properties.deviceName);
		ImGui::Text("%.2f ms per frame", (1000.0f / fps), fps);
		ImGui::Text("(%.1f fps)", fps);
		ImGui::TextUnformatted("Log: ");
		ImGui::SameLine();
		bool guiLogger = (int)CoreLogger.GetTypes() & (int)Core::LoggerType::ImGui;
		ImGui::Checkbox("GUI", &guiLogger);
		ImGui::SameLine();
		bool consoleLogger = (int)CoreLogger.GetTypes() & (int)Core::LoggerType::Console;
		ImGui::Checkbox("Console", &consoleLogger);

		float degFov = Camera::RadToDeg(camera.Fov());
		ImGui::SliderFloat("fov", &degFov, 60.f, 160.f);
		camera.Fov() = Camera::DegToRad(degFov);

		ImGui::SliderFloat("mouse sensitivity", &mouseSensitivity, 0.01f, 1.f);

		Core::LoggerType resultingTypes = (Core::LoggerType)
			((guiLogger ? (int)Core::LoggerType::ImGui : 0) |
			(consoleLogger ? (int)Core::LoggerType::Console : 0));
		CoreLogger.SetTypes(resultingTypes);

		ImGui::End();
	}

	if (!ImGui::Begin("Editing Tools", nullptr))
	{
		ImGui::End();
	}
	else
	{
		ImGui::Text("Color");
		ImGui::ColorPicker3("", &editColor[0]);

		ImGui::Separator();

		ImGui::Text("Cutting planes");
		ImGui::SliderFloat("min x", &cuttingPlanes.xMin, cuttingPlanesMinMax.xMin, cuttingPlanesMinMax.xMax);
		ImGui::SliderFloat("max x", &cuttingPlanes.xMax, cuttingPlanesMinMax.xMin, cuttingPlanesMinMax.xMax);
		ImGui::SliderFloat("min y", &cuttingPlanes.yMin, cuttingPlanesMinMax.yMin, cuttingPlanesMinMax.yMax);
		ImGui::SliderFloat("max y", &cuttingPlanes.yMax, cuttingPlanesMinMax.yMin, cuttingPlanesMinMax.yMax);
		ImGui::SliderFloat("min z", &cuttingPlanes.zMin, cuttingPlanesMinMax.zMin, cuttingPlanesMinMax.zMax);
		ImGui::SliderFloat("max z", &cuttingPlanes.zMax, cuttingPlanesMinMax.zMin, cuttingPlanesMinMax.zMax);

		ImGui::Separator();

		ImGui::Text("Tool type");
		if (ImGui::RadioButton("Brush", tool == EditingTool::Brush))
		{
			tool = EditingTool::Brush;
		}
		if (ImGui::RadioButton("Copy", tool == EditingTool::Copy))
		{
			tool = EditingTool::Copy;
		}
		if (ImGui::RadioButton("Fill", tool == EditingTool::Fill))
		{
			tool = EditingTool::Fill;
		}

		ImGui::End();
	}

	CoreLogger.DrawImGuiLogger("Log");

	if (ShaderManager.ShouldDraw())
	{
		ShaderManager.Draw("Shaders");
	}

	//ImGui::ShowDemoWindow();

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
		VulkanFactory::Buffer::Create("GUI Vertex Buffer", deviceInfo, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBufferSize,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexBuffer);
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
		VulkanFactory::Buffer::Create("GUI Index Buffer", deviceInfo, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBufferSize,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, indexBuffer);
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

bool GUI::Renderer::WantMouseCapture()
{
	return ImGui::GetIO().WantCaptureMouse;
}

bool GUI::Renderer::WantKeyboardCapture()
{
	return ImGui::GetIO().WantCaptureKeyboard;
}
