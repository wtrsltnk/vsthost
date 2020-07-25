#include "imguiutils.h"

namespace ImGui
{
    void MoveCursorPos(
        ImVec2 delta)
    {
        auto pos = GetCursorPos();
        SetCursorPos(ImVec2(pos.x + delta.x, pos.y + delta.y));
    }

    bool ActiveButton(char const *label, bool active)
    {
        if (active)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
        }

        auto result = ImGui::Button(label);

        if (active)
        {
            ImGui::PopStyleColor();
        }

        return result;
    }

} // namespace ImGui
