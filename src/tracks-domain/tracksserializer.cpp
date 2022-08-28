#include "tracksserializer.h"

#include "base64.h"
#include <fstream>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

TracksSerializer::TracksSerializer(
    TracksManager &tracks)
    : _tracks(tracks)
{}

//namespace YAML
//{

//    template <>
//    struct convert<glm::vec4>
//    {
//        static Node encode(const glm::vec4 &rhs)
//        {
//            Node node;
//            node.push_back(rhs.x);
//            node.push_back(rhs.y);
//            node.push_back(rhs.z);
//            node.push_back(rhs.w);
//            return node;
//        }

//        static bool decode(const Node &node, glm::vec4 &rhs)
//        {
//            if (!node.IsSequence() || node.size() != 4)
//                return false;

//            rhs.x = node[0].as<float>();
//            rhs.y = node[1].as<float>();
//            rhs.z = node[2].as<float>();
//            rhs.w = node[3].as<float>();
//            return true;
//        }
//    };

//} // namespace YAML

//YAML::Emitter &operator<<(YAML::Emitter &out, const glm::vec4 &v)
//{
//    out << YAML::Flow;
//    out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
//    return out;
//}

void SerializePlugin(
    YAML::Emitter &out,
    const std::unique_ptr<VstPlugin> &plugin)
{
    if (plugin == nullptr)
    {
        return;
    }

    out << YAML::Key << "Plugin" << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "ModulePath" << YAML::Value << plugin->ModulePath();

    /* Save plugin data*/
    void *getLen;
    int length = plugin->dispatcher(effGetChunk, 0, 0, &getLen, 0.0f);
    auto data = reinterpret_cast<BYTE *>(getLen);
    out << YAML::Key << "PluginData" << YAML::Value << base64_encode(&data[0], length);

    out << YAML::EndMap; // Plugin
}

void SerializeInstrument(
    YAML::Emitter &out,
    std::shared_ptr<Instrument> instrument)
{
    if (instrument == nullptr)
    {
        return;
    }

    out << YAML::Key << "Instrument" << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "Name" << YAML::Value << instrument->Name();
    out << YAML::Key << "MidiChannel" << YAML::Value << instrument->MidiChannel();

    instrument->Lock();
    SerializePlugin(out, instrument->Plugin());
    instrument->Unlock();

    out << YAML::EndMap; // Instrument
}

void SerializeTrack(
    YAML::Emitter &out,
    Track *track)
{
    out << YAML::BeginMap; // Track
    out << YAML::Key << "Name" << YAML::Value << track->GetName();
    //out << YAML::Key << "Color" << YAML::Value << glm::vec4(track->GetColor()[0], track->GetColor()[1], track->GetColor()[2], track->GetColor()[3]);
    out << YAML::Key << "IsMuted" << YAML::Value << track->IsMuted();
    out << YAML::Key << "IsReadyForRecoding" << YAML::Value << track->IsReadyForRecoding();

    SerializeInstrument(out, track->GetInstrument());

    out << YAML::EndMap; // Track
}

void TracksSerializer::Serialize(
    const std::string &filepath)
{
    YAML::Emitter out;

    out << YAML::BeginMap;
    out << YAML::Key << "Song" << YAML::Value << "Untitled";
    out << YAML::Key << "Tracks" << YAML::Value << YAML::BeginSeq;

    for (auto track : _tracks.GetTracks())
    {
        SerializeTrack(out, &track);
    }

    out << YAML::EndSeq;
    out << YAML::EndMap;

    std::ofstream fout(filepath);
    fout << out.c_str();
}

std::unique_ptr<VstPlugin> DeserializePlugin(
    const YAML::Node &instrumentData)
{
    auto pluginData = instrumentData["Plugin"];
    if (!pluginData)
    {
        spdlog::error("no plugin data found");

        return nullptr;
    }

    auto pluginModulePath = pluginData["ModulePath"].as<std::string>();
    auto pluginPluginData = pluginData["PluginData"].as<std::string>();

    auto plugin = std::make_unique<VstPlugin>();
    if (!plugin->init(pluginModulePath.c_str()))
    {
        spdlog::error("failed to load module");

        return nullptr;
    }

    auto data = base64_decode(pluginPluginData);

    /* Load plugin data*/
    plugin->dispatcher(effSetChunk, 0, (VstInt32)data.size(), data.data(), 0);

    return plugin;
}

std::shared_ptr<Instrument> DeserializeInstrument(
    const YAML::Node &trackData)
{
    auto instrumentData = trackData["Instrument"];
    if (!instrumentData)
    {
        spdlog::error("no instrument data found");

        return nullptr;
    }

    auto instrumentName = instrumentData["Name"].as<std::string>();
    auto instrumentMidiChannel = instrumentData["MidiChannel"].as<int>();

    auto instrument = std::make_shared<Instrument>();
    instrument->SetName(instrumentName);
    instrument->SetMidiChannel(instrumentMidiChannel);

    auto plugin = DeserializePlugin(instrumentData);
    if (plugin != nullptr)
    {
        instrument->SetPlugin(std::move(plugin));
    }

    return instrument;
}

bool TracksSerializer::Deserialize(
    const std::string &filepath)
{
    std::ifstream stream(filepath);
    std::stringstream strStream;
    strStream << stream.rdbuf();

    YAML::Node data = YAML::Load(strStream.str());
    if (!data["Song"])
    {
        spdlog::error("no song data found in {0}", filepath);

        return false;
    }

    std::string songName = data["Song"].as<std::string>();

    auto allTracksData = data["Tracks"];
    if (!allTracksData)
    {
        spdlog::error("no tracks data found in {0}", filepath);

        return false;
    }

    for (auto trackData : allTracksData)
    {
        auto trackName = trackData["Name"].as<std::string>();
        auto trackColor = trackData["Color"];
        auto trackIsMuted = trackData["IsMuted"];
        auto trackIsReadyForRecoding = trackData["IsReadyForRecoding"];

        auto trackId = _tracks.AddTrack(trackName, DeserializeInstrument(trackData));
        if (trackId == Track::Null)
        {
            continue;
        }

        auto &track = _tracks.GetTrack(trackId);
        //        if (trackColor)
        //        {
        //            track.SetColor(trackColor.as<glm::vec4>());
        //        }
        if (trackIsMuted)
        {
            trackIsMuted.as<bool>() ? track.Mute() : track.Unmute();
        }
        if (trackIsReadyForRecoding)
        {
            track.SetReadyForRecording(trackIsReadyForRecoding.as<bool>());
        }

    }

    return true;
}
