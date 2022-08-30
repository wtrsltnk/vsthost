#ifndef TRACKSSERIALIZER_H
#define TRACKSSERIALIZER_H

#include "track.h"
#include <ITracksManager.h>

class TracksSerializer
{
public:
    TracksSerializer(
        ITracksManager *tracks);

    void Serialize(
        const std::string &filepath);

    bool Deserialize(
        const std::string &filepath);

private:
    ITracksManager *_tracks;
};

#endif // TRACKSSERIALIZER_H
