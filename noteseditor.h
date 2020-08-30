#ifndef NOTESEDITOR_H
#define NOTESEDITOR_H

#include "abstracttimelineeditor.h"
#include "itracksmanager.h"
#include "midinote.h"
#include "state.h"

#include <imgui.h>

class NotesEditor :
    public AbstractTimelineEditor
{
    ImVec2 _noteDrawingAndEditingStart;
    bool _drawingNotes = false;
    bool _editingNotes = false;

    void HandleNotesEditorShortCuts();

    void RenderNotesCanvas(
        Region &region,
        const ImVec2 &origin);

    void RenderEditableNote(
        Region &region,
        int noteNumber,
        std::chrono::milliseconds::rep start,
        std::chrono::milliseconds::rep length,
        unsigned int velocity,
        const ImVec2 &noteSize,
        const ImVec2 &origin);

public:
    NotesEditor();

    void Render(
        const ImVec2 &pos,
        const ImVec2 &size);
};

#endif // NOTESEDITOR_H
