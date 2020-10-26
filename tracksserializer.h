#ifndef TRACKSSERIALIZER_H
#define TRACKSSERIALIZER_H

#include "tracksmanager.h"

class TracksSerializer
{
public:
    TracksSerializer(
        TracksManager &tracks);

    void Serialize(
        const std::string &filepath);

    bool Deserialize(
        const std::string &filepath);

private:
    TracksManager &_tracks;
};

#endif // TRACKSSERIALIZER_H
