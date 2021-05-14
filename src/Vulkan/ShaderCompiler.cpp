#include "ShaderCompiler.hpp"
#include "Core/Platform/Platform.hpp"
#include "Core/Logger/Logger.hpp"
#include "Vulkan/VulkanFactory.hpp"
#include <shaderc/shaderc.hpp>
#include <filesystem>

VkShaderModule Shader::Compiler::LoadShader(VkDevice device, const std::string& path)
{
	std::filesystem::path fsPath(path);
	auto extension = fsPath.extension();
	
	if (extension == ".spv")
	{
		fsPath.replace_extension();
		fsPath.replace_extension();
		auto filename = fsPath.filename();
		return VulkanFactory::Shader::Create(filename.string().c_str(), device, path);
	}
	else if (extension == ".glsl")
	{
		fsPath.replace_extension();
		auto filename = fsPath.filename();
		std::vector<char> shaderData;
		size_t size = CoreFilesystem.GetFileSize(path);
		shaderData.resize(size / sizeof(char));
		CoreFilesystem.ReadFile(path, shaderData.data(), size);

		auto shaderKindStr = fsPath.extension().string();
		shaderc_shader_kind shaderKind;
		if (shaderKindStr == ".vert")
		{
			shaderKind = shaderc_vertex_shader;
		}
		else if (shaderKindStr == ".frag")
		{
			shaderKind = shaderc_fragment_shader;
		}
		else if (shaderKindStr == ".comp")
		{
			shaderKind = shaderc_compute_shader;
		}
		else
		{
			CoreLogError("Internal shader compiler cannot figure out a supported shader stage of this file.");
			return VK_NULL_HANDLE;
		}

		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
		const bool optimize = true;
		if (optimize)
		{
			options.SetOptimizationLevel(shaderc_optimization_level_performance);
		}
		shaderc::Compiler compiler;
		auto compilationResult = compiler.CompileGlslToSpv(shaderData.data(), shaderData.size(),
			shaderKind, path.c_str(), "main", options);
		auto compilationStatus = compilationResult.GetCompilationStatus();

		if (compilationStatus != shaderc_compilation_status_success)
		{
			size_t errorCount = compilationResult.GetNumErrors();
			CoreLogError("Internal shader compiler encountered errors (%zu).", errorCount);

			std::string errorMessage = compilationResult.GetErrorMessage();
			CoreLogError("%s", errorMessage.c_str());

			return VK_NULL_HANDLE;
		}

		std::vector<uint32_t> byteCode(compilationResult.begin(), compilationResult.end());

		return VulkanFactory::Shader::Create(filename.string().c_str(), device,
			byteCode.data(), (uint32_t)(byteCode.size() * 4));
	}

	CoreLogError("Internal shader compiler cannot load files with the provided extension (%s).", extension.string().c_str());
	return VK_NULL_HANDLE;
}
