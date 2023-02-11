#ifndef IMGUIUTILS_H
#define IMGUIUTILS_H

#include <imgui.h>
#include <vector>

char const *NoteToString(
    uint32_t note);

namespace ImGui
{
    void MoveCursorPos(
        ImVec2 delta);

    bool ActiveButton(
        char const *label,
        bool active);

    void RadioIconButton(
        const char *label,
        const char *tooltip,
        int optionValue,
        int *currentValue);

    bool Knob(
        char const *label,
        float *p_value,
        float v_min,
        float v_max,
        ImVec2 const &size,
        char const *tooltip = nullptr);

    bool KnobUchar(
        char const *label,
        unsigned char *p_value,
        unsigned char v_min,
        unsigned char v_max,
        ImVec2 const &size,
        char const *tooltip = nullptr);

    void PushDisabled(
        bool disabled = true);

    void PopDisabled(
        int num = 1);

} // namespace ImGui

#endif // IMGUIUTILS_H
