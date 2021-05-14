#pragma once
#include "Core/Common.hpp"
#include <imgui.h>

class ImGuiLogger
{
public:
    ImGuiTextBuffer Buffer;
    ImGuiTextFilter Filter;
    ImVector<int> LineOffsets;
#ifdef IMGUI_LOGGER_USE_COLORS
    ImVector<ImVec4> LineColors;
#endif
    bool AutoScroll;

    ImGuiLogger();

    void Clear();
#ifdef IMGUI_LOGGER_USE_COLORS
    void Log(const ImVec4& color, const char* message, ...);
#else
    void Log(const char* message, ...);
#endif
    void Draw(const char* title, bool* open = nullptr);
};