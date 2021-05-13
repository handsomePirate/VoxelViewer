#pragma once
#include "Core/Common.hpp"
#include <imgui.h>

class ImGuiLogger
{
public:
    ImGuiTextBuffer Buffer;
    ImGuiTextFilter Filter;
    ImVector<int> LineOffsets;
    bool AutoScroll;

    ImGuiLogger();

    void Clear();
    void Log(const char* message, ...);
    void Draw(const char* title, bool* open = nullptr);
};