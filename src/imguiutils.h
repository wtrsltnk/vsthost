#ifndef IMGUIUTILS_H
#define IMGUIUTILS_H

#include <functional>
#include <imgui.h>
#include <map>
#include <vector>

char const *NoteToString(
    uint32_t note);

void PushPopStyleVar(
    ImGuiStyleVar idx,
    const ImVec2 &val,
    std::function<void()> scope);

void PushPopStyleVar(
    ImGuiStyleVar idx,
    float val,
    std::function<void()> scope);

void PushPopStyleColor(
    ImGuiCol idx,
    const ImVec4 &col,
    std::function<void()> scope);

void PushPopStyleColor(
    std::map<ImGuiCol, const ImVec4 &> colors,
    std::function<void()> scope);

void PushPopID(
    int int_id,
    std::function<void()> scope);

void PushPopFont(
    ImFont *font,
    std::function<void()> scope);

void PushPopItemWidth(
    float item_width,
    std::function<void()> scope);

void PushPopDisabled(
    bool disabled,
    std::function<void()> scope);

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
