#ifndef PLUGINSERVICE_H
#define PLUGINSERVICE_H

#include "ipluginservice.h"
#include <map>
#include <sqlitelib.h>
#include <string>

class PluginService :
    public IPluginService
{
public:
    PluginService(
        void *owner);

    virtual std::shared_ptr<class VstPlugin> LoadPlugin(
        const std::wstring &filename);

    virtual std::shared_ptr<class VstPlugin> LoadPlugin(
        const std::string &filename);

    virtual std::shared_ptr<class VstPlugin> LoadFromFileDialog();

    virtual std::vector<struct PluginDescription> ListPlugins(
        std::function<bool(const struct PluginDescription &)> filter = [](const struct PluginDescription &) { return true; });

private:
    std::unique_ptr<sqlitelib::Sqlite> _db;
    void *_owner;
    std::map<std::wstring, struct PluginDescription> _loadedPlugins;

    void EnsurePluginDescription(
        struct PluginDescription desc);
};

#endif // PLUGINSERVICE_H
