#ifndef TRACKSSERIALIZER_H
#define TRACKSSERIALIZER_H

#include <ITracksManager.h>
#include <ipluginservice.h>

class TracksSerializer
{
public:
    TracksSerializer(
        ITracksManager *tracks,
        IPluginService *vstPluginService);

    void Serialize(
        const std::string &filepath);

    bool Deserialize(
        const std::string &filepath);

private:
    ITracksManager *_tracks = nullptr;
    IPluginService *_vstPluginService = nullptr;
};

#endif // TRACKSSERIALIZER_H
