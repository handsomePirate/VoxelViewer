#include "ShaderManager.hpp"
#include "Core/Platform/Platform.hpp"
#include "Core/Logger/Logger.hpp"
#include <imgui.h>

void Shader::Manager::AddShader(const std::string& path)
{
	std::string name = CoreFilesystem.Filename(path);
	shaders_.push_back({ name, path, false });
}

void Shader::Manager::Draw(const char* title, bool* open)
{
    if (!ImGui::Begin(title, open))
    {
        ImGui::End();
        return;
    }

    int id = 10000;
    for (auto& shader : shaders_)
    {
        ImGui::PushID(shader.Name.c_str());
        ImGui::TextUnformatted(shader.Name.c_str());
        if (!shader.shouldReload)
        {
            ImGui::SameLine();
        }
        shader.shouldReload = shader.shouldReload || ImGui::Button("Reload");
        ImGui::Separator();
        ImGui::PopID();
    }

    ImGui::End();
}

const std::vector<Shader::ShaderEntry>& Shader::Manager::GetShaderList() const
{
    return shaders_;
}

void Shader::Manager::SignalShaderReloaded(uint32_t shaderNumber)
{
    shaders_[shaderNumber].shouldReload = false;
}
