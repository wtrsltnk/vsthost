#ifndef IMGUIUTILS_H
#define IMGUIUTILS_H

#include <imgui.h>

namespace ImGui
{
    void MoveCursorPos(
        ImVec2 delta);

    bool ActiveButton(char const *label, bool active);
} // namespace ImGui

#endif // IMGUIUTILS_H
