#include "pianowindow.h"

#include "../imguiutils.h"
#include <midinote.h>

const ImColor PianoWindow::blackKeyButton = ImColor(50, 50, 60);
const ImColor PianoWindow::blackKeyText = ImColor(250, 250, 250);
const ImColor PianoWindow::whiteKeyButton = ImColor(155, 175, 200);
const ImColor PianoWindow::whiteKeyText = ImColor(20, 20, 20);
const ImColor PianoWindow::downKeyButton = ImColor(50, 150, 50);
const ImColor PianoWindow::downKeyText = ImColor(250, 250, 250);

std::set<uint32_t> PianoWindow::downKeys;

PianoWindow::PianoWindow() = default;

void PianoWindow::SetState(
    State *state)
{
    _state = state;
}

void NoteButton(
    int note,
    const ImVec2 &size)
{
    auto markAsDown = PianoWindow::downKeys.contains(note);

    if (markAsDown)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)PianoWindow::downKeyButton);
        ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)PianoWindow::downKeyText);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(note / float(12 * 3), 0.6f, 0.6f));
    }

    ImGui::Button(NoteToString(note), size);
    HandleKeyUpDown(note);

    if (markAsDown)
    {
        ImGui::PopStyleColor();
    }
    ImGui::PopStyleColor();
}

void NoteButtonWithoutColor(
    int note,
    const ImVec2 &size)
{
    auto markAsDown = PianoWindow::downKeys.contains(note);

    if (markAsDown)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)PianoWindow::downKeyButton);
        ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)PianoWindow::downKeyText);
    }

    ImGui::Button(NoteToString(note), size);
    HandleKeyUpDown(note);

    if (markAsDown)
    {
        ImGui::PopStyleColor(2);
    }
}

void PianoWindow::Render(
    ImVec2 const &pos,
    ImVec2 const &size)
{
    ImGui::Begin("Piano", nullptr, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    {
        ImGui::SetWindowPos(pos);
        ImGui::SetWindowSize(size);

        const int keyWidth = 32;
        const int keyHeight = 64;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 38));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)blackKeyButton);
        ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)blackKeyText);

        auto drawPos = ImGui::GetCursorScreenPos();
        auto drawHeight = ImGui::GetContentRegionAvail().y;
        int octaveWidth = int(float(keyWidth + ImGui::GetStyle().ItemSpacing.x) * 7);
        int n;
        for (int i = 0; i < _octaves; i++)
        {
            auto a = ImVec2(drawPos.x - (ImGui::GetStyle().ItemSpacing.x / 2) + octaveWidth * i, drawPos.y);
            auto b = ImVec2(a.x + octaveWidth, a.y + drawHeight);
            ImGui::GetWindowDrawList()->AddRectFilled(a, b, IM_COL32(66, 150, 249, (i + 1) * 20));

            ImGui::PushID(i);
            if (i == 0)
            {
                ImGui::InvisibleButton("HalfSpace", ImVec2((keyWidth / 2) - (ImGui::GetStyle().ItemSpacing.x / 2), keyHeight * 1.1f));
            }

            ImGui::SameLine();

            ImGui::MoveCursorPos(ImVec2(keyWidth * 0.1f, 0.0f));

            n = firstKeyNoteNumber + ((i + _state->_octaveShift) * 12) + Note_CSharp_OffsetFromC;
            NoteButtonWithoutColor(n, ImVec2(keyWidth * 0.8f, keyHeight * 1.1f));
            ImGui::SameLine();

            ImGui::MoveCursorPos(ImVec2(keyWidth * 0.2f, 0.0f));

            n = firstKeyNoteNumber + ((i + _state->_octaveShift) * 12) + Note_DSharp_OffsetFromC;
            NoteButtonWithoutColor(n, ImVec2(keyWidth * 0.8f, keyHeight * 1.1f));
            ImGui::SameLine();

            ImGui::MoveCursorPos(ImVec2(keyWidth * 0.2f, 0.0f));

            ImGui::InvisibleButton("SpaceE", ImVec2(keyWidth, keyHeight * 1.1f));
            ImGui::SameLine();

            n = firstKeyNoteNumber + ((i + _state->_octaveShift) * 12) + Note_FSharp_OffsetFromC;
            NoteButtonWithoutColor(n, ImVec2(keyWidth * 0.8f, keyHeight * 1.1f));
            ImGui::SameLine();

            ImGui::MoveCursorPos(ImVec2(keyWidth * 0.2f, 0.0f));

            n = firstKeyNoteNumber + ((i + _state->_octaveShift) * 12) + Note_GSharp_OffsetFromC;
            NoteButtonWithoutColor(n, ImVec2(keyWidth * 0.8f, keyHeight * 1.1f));
            ImGui::SameLine();

            ImGui::MoveCursorPos(ImVec2(keyWidth * 0.2f, 0.0f));

            n = firstKeyNoteNumber + ((i + _state->_octaveShift) * 12) + Note_ASharp_OffsetFromC;
            NoteButtonWithoutColor(n, ImVec2(keyWidth * 0.8f, keyHeight * 1.1f));
            ImGui::SameLine();

            ImGui::MoveCursorPos(ImVec2(keyWidth * 0.1f, 0.0f));

            ImGui::InvisibleButton("HalfSpace", ImVec2(keyWidth, keyHeight * 1.1f));

            ImGui::PopID();
        }

        ImGui::PopStyleColor(2);

        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)whiteKeyButton);
        ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)whiteKeyText);

        for (int i = 0; i < _octaves; i++)
        {
            ImGui::PushID(i);

            if (i > 0)
            {
                ImGui::SameLine();
            }

            n = firstKeyNoteNumber + ((i + _state->_octaveShift) * 12) + Note_C_OffsetFromC;
            NoteButtonWithoutColor(n, ImVec2(keyWidth, keyHeight * 0.9f));
            ImGui::SameLine();

            n = firstKeyNoteNumber + ((i + _state->_octaveShift) * 12) + Note_D_OffsetFromC;
            NoteButtonWithoutColor(n, ImVec2(keyWidth, keyHeight * 0.9f));
            ImGui::SameLine();

            n = firstKeyNoteNumber + ((i + _state->_octaveShift) * 12) + Note_E_OffsetFromC;
            NoteButtonWithoutColor(n, ImVec2(keyWidth, keyHeight * 0.9f));
            ImGui::SameLine();

            n = firstKeyNoteNumber + ((i + _state->_octaveShift) * 12) + Note_F_OffsetFromC;
            NoteButtonWithoutColor(n, ImVec2(keyWidth, keyHeight * 0.9f));
            ImGui::SameLine();

            n = firstKeyNoteNumber + ((i + _state->_octaveShift) * 12) + Note_G_OffsetFromC;
            NoteButtonWithoutColor(n, ImVec2(keyWidth, keyHeight * 0.9f));
            ImGui::SameLine();

            n = firstKeyNoteNumber + ((i + _state->_octaveShift) * 12) + Note_A_OffsetFromC;
            NoteButtonWithoutColor(n, ImVec2(keyWidth, keyHeight * 0.9f));
            ImGui::SameLine();

            n = firstKeyNoteNumber + ((i + _state->_octaveShift) * 12) + Note_B_OffsetFromC;
            NoteButtonWithoutColor(n, ImVec2(keyWidth, keyHeight * 0.9f));
            HandleKeyUpDown(n);

            ImGui::PopID();
        }

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
    }
    ImGui::End();
}

char const *NoteToString(
    uint32_t note)
{
    switch (note)
    {
        case 127:
            return "G-9";
        case 126:
            return "F#9";
        case 125:
            return "F-9";
        case 124:
            return "E-9";
        case 123:
            return "D#9";
        case 122:
            return "D-9";
        case 121:
            return "C#9";
        case 120:
            return "C-9";
        case 119:
            return "B-8";
        case 118:
            return "A#8";
        case 117:
            return "A-8";
        case 116:
            return "G#8";
        case 115:
            return "G-8";
        case 114:
            return "F#8";
        case 113:
            return "F-8";
        case 112:
            return "E-8";
        case 111:
            return "D#8";
        case 110:
            return "D-8";
        case 109:
            return "C#8";
        case 108:
            return "C-8";
        case 107:
            return "B-7";
        case 106:
            return "A#7";
        case 105:
            return "A-7";
        case 104:
            return "G#7";
        case 103:
            return "G-7";
        case 102:
            return "F#7";
        case 101:
            return "F-7";
        case 100:
            return "E-7";
        case 99:
            return "D#7";
        case 98:
            return "D-7";
        case 97:
            return "C#7";
        case 96:
            return "C-7";
        case 95:
            return "B-6";
        case 94:
            return "A#6";
        case 93:
            return "A-6";
        case 92:
            return "G#6";
        case 91:
            return "G-6";
        case 90:
            return "F#6";
        case 89:
            return "F-6";
        case 88:
            return "E-6";
        case 87:
            return "D#6";
        case 86:
            return "D-6";
        case 85:
            return "C#6";
        case 84:
            return "C-6";
        case 83:
            return "B-5";
        case 82:
            return "A#5";
        case 81:
            return "A-5";
        case 80:
            return "G#5";
        case 79:
            return "G-5";
        case 78:
            return "F#5";
        case 77:
            return "F-5";
        case 76:
            return "E-5";
        case 75:
            return "D#5";
        case 74:
            return "D-5";
        case 73:
            return "C#5";
        case 72:
            return "C-5";
        case 71:
            return "B-4";
        case 70:
            return "A#4";
        case 69:
            return "A-4";
        case 68:
            return "G#4";
        case 67:
            return "G-4";
        case 66:
            return "F#4";
        case 65:
            return "F-4";
        case 64:
            return "E-4";
        case 63:
            return "D#4";
        case 62:
            return "D-4";
        case 61:
            return "C#4";
        case 60:
            return "C-4";
        case 59:
            return "B-3";
        case 58:
            return "A#3";
        case 57:
            return "A-3";
        case 56:
            return "G#3";
        case 55:
            return "G-3";
        case 54:
            return "F#3";
        case 53:
            return "F-3";
        case 52:
            return "E-3";
        case 51:
            return "D#3";
        case 50:
            return "D-3";
        case 49:
            return "C#3";
        case 48:
            return "C-3";
        case 47:
            return "B-2";
        case 46:
            return "A#2";
        case 45:
            return "A-2";
        case 44:
            return "G#2";
        case 43:
            return "G-2";
        case 42:
            return "F#2";
        case 41:
            return "F-2";
        case 40:
            return "E-2";
        case 39:
            return "D#2";
        case 38:
            return "D-2";
        case 37:
            return "C#2";
        case 36:
            return "C-2";
        case 35:
            return "B-1";
        case 34:
            return "A#1";
        case 33:
            return "A-1";
        case 32:
            return "G#1";
        case 31:
            return "G-1";
        case 30:
            return "F#1";
        case 29:
            return "F-1";
        case 28:
            return "E-1";
        case 27:
            return "D#1";
        case 26:
            return "D-1";
        case 25:
            return "C#1";
        case 24:
            return "C-1";
        case 23:
            return "B-0";
        case 22:
            return "A#0";
        case 21:
            return "A-0";
    }
    return "---";
}
