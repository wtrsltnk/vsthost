#include "historymanager.h"

HistoryManager::HistoryManager() = default;

HistoryManager::~HistoryManager()
{
    Cleanup(_firstEntryInHistoryTrack);

    _currentEntryInHistoryTrack = nullptr;

    _firstEntryInHistoryTrack = nullptr;
}

void HistoryManager::Cleanup(
    HistoryEntry *from)
{
    while (from != nullptr)
    {
        auto tmp = from;
        from = tmp->_nextEntry;

        delete tmp;
    }
}

bool HistoryManager::HasUndo()
{
    return _currentEntryInHistoryTrack != nullptr;
}

void HistoryManager::Undo()
{
    if (!HasUndo())
    {
        return;
    }

    _currentEntryInHistoryTrack->_track->SetRegions(_currentEntryInHistoryTrack->_regions);
    if (_currentEntryInHistoryTrack->_prevEntry != nullptr)
    {
        _currentEntryInHistoryTrack = _currentEntryInHistoryTrack->_prevEntry;
    }
    else
    {
        Cleanup(_currentEntryInHistoryTrack);
        _currentEntryInHistoryTrack = nullptr;
        _firstEntryInHistoryTrack = nullptr;
    }
}

bool HistoryManager::HasRedo()
{
    return _currentEntryInHistoryTrack != nullptr && _currentEntryInHistoryTrack->_nextEntry != nullptr;
}

void HistoryManager::Redo()
{}

void HistoryManager::AddEntry(
    const char *title,
    ITrack *track,
    ITrack::RegionCollection regions)
{
    if (_currentEntryInHistoryTrack != nullptr)
    {
        Cleanup(_currentEntryInHistoryTrack->_nextEntry);
    }

    auto entry = new HistoryEntry();

    entry->_title = title;
    entry->_track = track;
    entry->_regions = regions;
    entry->_prevEntry = _currentEntryInHistoryTrack;

    if (_currentEntryInHistoryTrack != nullptr)
    {
        _currentEntryInHistoryTrack->_nextEntry = entry;
    }

    _currentEntryInHistoryTrack = entry;

    if (_firstEntryInHistoryTrack == nullptr)
    {
        _firstEntryInHistoryTrack = _currentEntryInHistoryTrack;
    }
}
