#ifndef PIANOWINDOW_H
#define PIANOWINDOW_H

#include <imgui.h>

extern void HandleKeyUpDown(
    int noteNumber);

class PianoWindow
{
    int _octaves = 8;

public:
    PianoWindow();

    void Render(
        ImVec2 const &pos,
        ImVec2 const &size);
};

#endif // PIANOWINDOW_H
