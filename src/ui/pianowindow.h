#ifndef PIANOWINDOW_H
#define PIANOWINDOW_H

#include "../state.h"
#include <imgui.h>
#include <set>

extern void HandleKeyUpDown(
    int noteNumber);

extern void NoteButton(
    int note,
    const ImVec2 &size);

class PianoWindow
{
public:
    PianoWindow();

    void SetState(
        State *state);

    void Render(
        ImVec2 const &pos,
        ImVec2 const &size);

    static std::set<uint32_t> downKeys;

private:
    State *_state = nullptr;
    int _octaves = 8;

public:
    static const ImColor blackKeyButton;
    static const ImColor blackKeyText;
    static const ImColor whiteKeyButton;
    static const ImColor whiteKeyText;
    static const ImColor downKeyButton;
    static const ImColor downKeyText;
};

#endif // PIANOWINDOW_H
