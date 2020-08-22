#include "midinote.h"

MidiNote::MidiNote()
{
}

MidiNote::MidiNote(
    std::chrono::milliseconds::rep t,
    unsigned int v)
    : time(t),
      velocity(v)
{
}

MidiNote::CollectionInTimeByNote MidiNote::ConvertMidiEventsToMidiNotes(
    const MidiEvent::CollectionInTime &events)
{
    MidiNote::CollectionInTimeByNote result;

    struct ActiveNote
    {
        std::chrono::milliseconds::rep time;
        unsigned int velocity;
    };

    std::map<unsigned int, ActiveNote> activeNotes;

    for (auto eventsAtTime : events)
    {
        for (auto event : eventsAtTime.second)
        {
            auto activeNoteByEvent = activeNotes.find(event.num);

            if (event.type == MidiEventTypes::M_NOTE)
            {
                if (result.find(event.num) == result.end())
                {
                    result.insert(std::make_pair(event.num, CollectionInTime()));
                }

                if (event.value != 0 && activeNoteByEvent == activeNotes.end())
                {
                    activeNotes.insert(std::make_pair(event.num, ActiveNote{eventsAtTime.first, event.value}));
                    continue;
                }

                if (event.value == 0 && activeNoteByEvent != activeNotes.end())
                {
                    auto start = activeNoteByEvent->second.time;
                    auto end = eventsAtTime.first;

                    if (result[event.num].find(start) == result[event.num].end())
                    {
                        result[event.num].insert(std::make_pair(start, MidiNote::Collection()));
                    }

                    result[event.num][start].push_back(MidiNote(end - start, activeNoteByEvent->second.velocity));

                    activeNotes.erase(activeNoteByEvent);
                }
            }
        }
    }

    return result;
}
