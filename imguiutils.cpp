#include "imguiutils.h"

#include <cmath>

namespace ImGui
{
    void MoveCursorPos(
        ImVec2 delta)
    {
        auto pos = GetCursorPos();
        SetCursorPos(ImVec2(pos.x + delta.x, pos.y + delta.y));
    }

    bool ActiveButton(
        char const *label,
        bool active)
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

    void RadioIconButton(
        const char *label,
        const char *tooltip,
        int optionValue,
        int *currentValue)
    {
        auto selected = optionValue == *currentValue;
        if (selected)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
        }

        if (ImGui::Button(label, ImVec2(50, 0)))
        {
            *currentValue = optionValue;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", tooltip);
        }

        if (selected)
        {
            ImGui::PopStyleColor();
        }
    }

    bool ImGui::Knob(
        char const *label,
        float *p_value,
        float v_min,
        float v_max,
        ImVec2 const &size,
        char const *tooltip)
    {
        bool showLabel = label[0] != '#' && label[1] != '#' && label[0] != '\0';

        ImGuiIO &io = ImGui::GetIO();
        ImGuiStyle &style = ImGui::GetStyle();
        ImVec2 s(size.x - 4, size.y - 4);

        float radius_outer = std::fmin(s.x, s.y) / 2.0f;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        pos = ImVec2(pos.x + 2, pos.y + 2);
        ImVec2 center = ImVec2(pos.x + radius_outer, pos.y + radius_outer);

        float line_height = ImGui::GetTextLineHeight();
        ImDrawList *draw_list = ImGui::GetWindowDrawList();

        float ANGLE_MIN = 3.141592f * 0.70f;
        float ANGLE_MAX = 3.141592f * 2.30f;

        if (s.x != 0.0f && s.y != 0.0f)
        {
            center.x = pos.x + (s.x / 2.0f);
            center.y = pos.y + (s.y / 2.0f);
            ImGui::InvisibleButton(label, ImVec2(s.x, s.y + (showLabel ? line_height + style.ItemInnerSpacing.y : 0)));
        }
        else
        {
            ImGui::InvisibleButton(label, ImVec2(radius_outer * 2, radius_outer * 2 + (showLabel ? line_height + style.ItemInnerSpacing.y : 0)));
        }
        bool value_changed = false;
        bool is_active = ImGui::IsItemActive();
        bool is_hovered = ImGui::IsItemActive();
        if (is_active && io.MouseDelta.y != 0.0f)
        {
            float step = (v_max - v_min) / 200.0f;
            *p_value -= io.MouseDelta.y * step;
            if (*p_value < v_min)
                *p_value = v_min;
            if (*p_value > v_max)
                *p_value = v_max;
            value_changed = true;
        }

        float angle = ANGLE_MIN + (ANGLE_MAX - ANGLE_MIN) * (*p_value - v_min) / (v_max - v_min);
        float angle_cos = cosf(angle);
        float angle_sin = sinf(angle);

        draw_list->AddCircleFilled(center, radius_outer * 0.7f, ImGui::GetColorU32(ImGuiCol_Button), 16);
        draw_list->PathArcTo(center, radius_outer, ANGLE_MIN, ANGLE_MAX, 16);
        draw_list->PathStroke(ImGui::GetColorU32(ImGuiCol_FrameBg), false, 3.0f);
        draw_list->AddLine(
            ImVec2(center.x + angle_cos * (radius_outer * 0.35f), center.y + angle_sin * (radius_outer * 0.35f)),
            ImVec2(center.x + angle_cos * (radius_outer * 0.7f), center.y + angle_sin * (radius_outer * 0.7f)),
            ImGui::GetColorU32(ImGuiCol_SliderGrabActive), 2.0f);
        draw_list->PathArcTo(center, radius_outer, ANGLE_MIN, angle + 0.02f, 16);
        draw_list->PathStroke(ImGui::GetColorU32(ImGuiCol_SliderGrabActive), false, 3.0f);

        if (showLabel)
        {
            auto textSize = CalcTextSize(label);
            draw_list->AddText(ImVec2(pos.x + ((size.x / 2) - (textSize.x / 2)), pos.y + radius_outer * 2 + style.ItemInnerSpacing.y), ImGui::GetColorU32(ImGuiCol_Text), label);
        }

        if (is_active || is_hovered)
        {
            ImGui::SetNextWindowPos(ImVec2(pos.x - style.WindowPadding.x, pos.y - (line_height * 2) - style.ItemInnerSpacing.y - style.WindowPadding.y));
            ImGui::BeginTooltip();
            if (tooltip != nullptr)
            {
                ImGui::Text("%s\nValue : %.3f", tooltip, static_cast<double>(*p_value));
            }
            else if (showLabel)
            {
                ImGui::Text("%s %.3f", label, static_cast<double>(*p_value));
            }
            else
            {
                ImGui::Text("%.3f", static_cast<double>(*p_value));
            }
            ImGui::EndTooltip();
        }

        return value_changed;
    }

    bool ImGui::KnobUchar(
        char const *label,
        unsigned char *p_value,
        unsigned char v_min,
        unsigned char v_max,
        ImVec2 const &size,
        char const *tooltip)
    {
        bool showLabel = label[0] != '#' && label[1] != '#' && label[0] != '\0';

        ImGuiIO &io = ImGui::GetIO();
        ImGuiStyle &style = ImGui::GetStyle();
        ImVec2 s(size.x - 4, size.y - 4);

        float radius_outer = std::fmin(s.x, s.y) / 2.0f;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        pos = ImVec2(pos.x, pos.y + 2);
        ImVec2 center = ImVec2(pos.x + radius_outer, pos.y + radius_outer);

        float line_height = ImGui::GetFrameHeight();
        ImDrawList *draw_list = ImGui::GetWindowDrawList();

        float ANGLE_MIN = 3.141592f * 0.70f;
        float ANGLE_MAX = 3.141592f * 2.30f;

        if (s.x != 0.0f && s.y != 0.0f)
        {
            center.x = pos.x + (s.x / 2.0f);
            center.y = pos.y + (s.y / 2.0f);
            ImGui::InvisibleButton(label, ImVec2(s.x, s.y + (showLabel ? line_height + style.ItemInnerSpacing.y : 0)));
        }
        else
        {
            ImGui::InvisibleButton(label, ImVec2(radius_outer * 2, radius_outer * 2 + (showLabel ? line_height + style.ItemInnerSpacing.y : 0)));
        }
        bool value_changed = false;
        bool is_active = ImGui::IsItemActive();
        bool is_hovered = ImGui::IsItemActive();
        if (is_active && io.MouseDelta.y != 0.0f)
        {
            int step = (v_max - v_min) / 127;
            int newVal = static_cast<int>(*p_value - io.MouseDelta.y * step);
            if (newVal < v_min)
                newVal = v_min;
            if (newVal > v_max)
                newVal = v_max;
            *p_value = static_cast<unsigned char>(newVal);
            value_changed = true;
        }

        float angle = ANGLE_MIN + (ANGLE_MAX - ANGLE_MIN) * (*p_value - v_min) / (v_max - v_min);
        float angle_cos = cosf(angle);
        float angle_sin = sinf(angle);

        draw_list->AddCircleFilled(center, radius_outer * 0.7f, ImGui::GetColorU32(ImGuiCol_Button), 16);
        draw_list->PathArcTo(center, radius_outer, ANGLE_MIN, ANGLE_MAX, 16);
        draw_list->PathStroke(ImGui::GetColorU32(ImGuiCol_FrameBg), false, 3.0f);
        draw_list->AddLine(
            ImVec2(center.x + angle_cos * (radius_outer * 0.35f), center.y + angle_sin * (radius_outer * 0.35f)),
            ImVec2(center.x + angle_cos * (radius_outer * 0.7f), center.y + angle_sin * (radius_outer * 0.7f)),
            ImGui::GetColorU32(ImGuiCol_Text), 2.0f);
        draw_list->PathArcTo(center, radius_outer, ANGLE_MIN, angle + 0.02f, 16);
        draw_list->PathStroke(ImGui::GetColorU32(ImGuiCol_SliderGrabActive), false, 3.0f);

        if (showLabel)
        {
            auto textSize = CalcTextSize(label);
            draw_list->AddText(ImVec2(pos.x + ((size.x / 2) - (textSize.x / 2)), pos.y + radius_outer * 2 + style.ItemInnerSpacing.y), ImGui::GetColorU32(ImGuiCol_Text), label);
        }

        if (is_active || is_hovered)
        {
            ImGui::SetNextWindowPos(ImVec2(pos.x - style.WindowPadding.x, pos.y - (line_height * 2) - style.ItemInnerSpacing.y - style.WindowPadding.y));
            ImGui::BeginTooltip();
            if (tooltip != nullptr)
            {
                ImGui::Text("%s\nValue : %d", tooltip, static_cast<unsigned int>(*p_value));
            }
            else if (showLabel)
            {
                ImGui::Text("%s %d", label, static_cast<unsigned int>(*p_value));
            }
            else
            {
                ImGui::Text("%d", static_cast<unsigned int>(*p_value));
            }
            ImGui::EndTooltip();
        }
        else if (ImGui::IsItemHovered() && tooltip != nullptr)
        {
            ImGui::SetTooltip("%s", tooltip);
        }

        return value_changed;
    }

} // namespace ImGui
