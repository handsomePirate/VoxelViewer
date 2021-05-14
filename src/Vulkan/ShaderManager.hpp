#pragma once
#include "Core/Common.hpp"
#include "Core/Singleton.hpp"
#include <tuple>

namespace Shader
{
	struct ShaderEntry
	{
		std::string Name;
		std::string Path;
		bool shouldReload;
	};

	class Manager
	{
	public:
		Manager() = default;
		~Manager() = default;

		void AddShader(const std::string& path);

		void Draw(const char* title, bool* open = nullptr);

		const std::vector<ShaderEntry>& GetShaderList() const;
		void SignalShaderReloaded(uint32_t shaderNumber);

	private:
		std::vector<ShaderEntry> shaders_;
	};
}

#define ShaderManager (::Core::Singleton<::Shader::Manager>::GetInstance())
