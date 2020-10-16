#ifndef PIANOWINDOW_H
#define PIANOWINDOW_H

#include <imgui.h>
#include <set>

extern void HandleKeyUpDown(
    int noteNumber);

extern void NoteButton(
    int note,
    const ImVec2 &size);

class PianoWindow
{
    int _octaves = 8;

public:
    PianoWindow();

    void Render(
        ImVec2 const &pos,
        ImVec2 const &size);

    static std::set<uint32_t> downKeys;
};

#endif // PIANOWINDOW_H
