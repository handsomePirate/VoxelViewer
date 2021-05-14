#pragma once
#include "Core/Common.hpp"
#include "Vulkan/VulkanFactory.hpp"

namespace Shader
{
	class Compiler
	{
	public:
		static VkShaderModule LoadShader(VkDevice device, const std::string& path);
	};
}
