#ifndef IVSTPLUGINSERVICE_H
#define IVSTPLUGINSERVICE_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

class VstPlugin;

struct PluginDescription
{
    int id;
    std::string type;
    std::string path;
    std::string md5;
    std::string vendorName;
    int vendorVersion;
    std::string effectName;
    int programCount;
    int paramCount;
    int inputCount;
    int outputCount;
    bool isSynth;
    bool hasEditor;
};

class IPluginService
{
public:
    virtual ~IPluginService() = default;

    virtual std::shared_ptr<class VstPlugin> LoadPlugin(
        const std::wstring &filename) = 0;

    virtual std::shared_ptr<class VstPlugin> LoadPlugin(
        const std::string &filename) = 0;

    virtual std::shared_ptr<VstPlugin> LoadFromFileDialog() = 0;

    virtual std::vector<struct PluginDescription> ListPlugins(
        std::function<bool(const struct PluginDescription &)> filter = [](const struct PluginDescription &) { return true; }) = 0;
};

#endif // IVSTPLUGINSERVICE_H
