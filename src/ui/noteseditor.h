#ifndef NOTESEDITOR_H
#define NOTESEDITOR_H

#include "../state.h"
#include "abstracttimelineeditor.h"
#include <imgui.h>
#include <itracksmanager.h>
#include <midinote.h>

class NotesEditor :
    public AbstractTimelineEditor
{
public:
    NotesEditor();

    void Init();

    void Render(
        const ImVec2 &pos,
        const ImVec2 &size);

private:
    ImFont *_monofont = nullptr;
    ImVec2 _noteDrawingAndEditingStart;
    bool _drawingNotes = false;
    bool _editingNotes = false;
    float _lastNotePreviewY = 0;

    void HandleNotesEditorShortCuts();

    void RenderTrackNotes(
        const ImVec2 &size);

    void RenderEventButtonsInRegion(
        Region &region,
        const ImVec2 &originContainerScreenPos,
        const ImVec2 &origin);
    
    void RenderNoteHelpersInRegion(
        Region &region,
        const ImVec2 &originContainerScreenPos,
        const ImVec2 &origin);

    void RenderNotesCanvas(
        Region &region);

    void RenderEditableNote(
        Region &region,
        int noteNumber,
        std::chrono::milliseconds::rep start,
        std::chrono::milliseconds::rep length,
        uint32_t velocity,
        const ImVec2 &noteSize,
        const ImVec2 &origin);
};

#endif // NOTESEDITOR_H
