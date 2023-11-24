#ifndef TRACKSSERIALIZER_H
#define TRACKSSERIALIZER_H

#include <ITracksManager.h>
#include <ivstpluginservice.h>

class TracksSerializer
{
public:
    TracksSerializer(
        ITracksManager *tracks,
        IVstPluginService *vstPluginService);

    void Serialize(
        const std::string &filepath);

    bool Deserialize(
        const std::string &filepath);

private:
    ITracksManager *_tracks = nullptr;
    IVstPluginService *_vstPluginService = nullptr;
};

#endif // TRACKSSERIALIZER_H
