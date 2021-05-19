#include "ImGuiLogger.hpp"

ImGuiLogger::ImGuiLogger()
{
    AutoScroll = true;
    Clear();
}

void ImGuiLogger::Clear()
{
    Buffer.clear();
    LineOffsets.clear();
    LineOffsets.push_back(0);
#ifdef IMGUI_LOGGER_USE_COLORS
    LineColors.clear();
    LineColors.push_back({ 0.f, 0.f, 0.f, 0.f });
#endif
}

#ifdef IMGUI_LOGGER_USE_COLORS
void ImGuiLogger::Log(const ImVec4& color, const char* message, ...)
#else
void ImGuiLogger::Log(const char* message, ...)
#endif
{
    int oldSize = Buffer.size();
    va_list args;
    va_start(args, message);
    Buffer.appendfv(message, args);
    va_end(args);
    for (int new_size = Buffer.size(); oldSize < new_size; ++oldSize)
    {
        if (Buffer[oldSize] == '\n')
        {
            LineOffsets.push_back(oldSize + 1);
#ifdef IMGUI_LOGGER_USE_COLORS
            LineColors.push_back(color);
#endif
        }
    }
}

void ImGuiLogger::Draw(const char* title, bool* open)
{
    if (!ImGui::Begin(title, open))
    {
        ImGui::End();
        return;
    }

    // Options menu
    if (ImGui::BeginPopup("Options"))
    {
        ImGui::Checkbox("Auto-scroll", &AutoScroll);
        ImGui::EndPopup();
    }

    // Main window
    if (ImGui::Button("Options"))
    {
        ImGui::OpenPopup("Options");
    }

    ImGui::SameLine();
    bool clear = ImGui::Button("Clear");
    ImGui::SameLine();
    bool copy = ImGui::Button("Copy");
    ImGui::SameLine();
    Filter.Draw("Filter", -100.0f);

    ImGui::Separator();
    ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    if (clear)
    {
        Clear();
    }
    if (copy)
    {
        ImGui::LogToClipboard();
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    const char* buffer = Buffer.begin();
    const char* bufferEnd = Buffer.end();
    
#ifdef IMGUI_LOGGER_USE_COLORS
    const int maxMessageSize = 2048;
    char message[maxMessageSize];
#endif

    if (Filter.IsActive())
    {
        for (int lineNumber = 0; lineNumber < LineOffsets.Size; ++lineNumber)
        {
            const char* lineStart = buffer + LineOffsets[lineNumber];
            const char* lineEnd = (lineNumber + 1 < LineOffsets.Size) ? (buffer + LineOffsets[lineNumber + 1] - 1) : bufferEnd;
#ifdef IMGUI_LOGGER_USE_COLORS
            const int lineSize = (lineNumber + 1 < LineOffsets.Size) ?
                LineOffsets[lineNumber + 1] - LineOffsets[lineNumber] : Buffer.size() - LineOffsets[lineNumber];
#endif
            if (Filter.PassFilter(lineStart, lineEnd))
            {
#ifdef IMGUI_LOGGER_USE_COLORS
                memcpy(message, lineStart, lineSize);
                message[lineSize] = '\0';
                ImGui::TextColored(LineColors[(lineNumber + 1) % LineOffsets.Size], message);
#else
                ImGui::TextUnformatted(lineStart, lineEnd);
#endif
            }
        }
    }
    else
    {
        ImGuiListClipper clipper;
        clipper.Begin(LineOffsets.Size);
        while (clipper.Step())
        {
            for (int lineNumber = clipper.DisplayStart; lineNumber < clipper.DisplayEnd; ++lineNumber)
            {
                const char* lineStart = buffer + LineOffsets[lineNumber];
                const char* lineEnd = (lineNumber + 1 < LineOffsets.Size) ? (buffer + LineOffsets[lineNumber + 1] - 1) : bufferEnd;
#ifdef IMGUI_LOGGER_USE_COLORS
                const int lineSize = (lineNumber + 1 < LineOffsets.Size) ?
                    LineOffsets[lineNumber + 1] - LineOffsets[lineNumber] : Buffer.size() - LineOffsets[lineNumber];
                memcpy(message, lineStart, lineSize);
                message[lineSize] = '\0';
                ImGui::TextColored(LineColors[(lineNumber + 1) % LineOffsets.Size], message);
#else
                ImGui::TextUnformatted(lineStart, lineEnd);
#endif
            }
        }
        clipper.End();
    }
    ImGui::PopStyleVar();

    if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::End();
}
