#ifndef WIN32VSTPLUGINSERVICE_H
#define WIN32VSTPLUGINSERVICE_H

#include "ivstpluginservice.h"
#include <map>
#include <sqlitelib.h>
#include <string>
#include <windows.h>

struct VstPluginDescription
{
    std::string path;
    std::string md5;
    std::string vendorName;
    int vendorVersion;
    std::string effectName;
    bool isSynth;
    bool hasEditor;
};

class Win32VstPluginService :
    public IVstPluginService
{
public:
    Win32VstPluginService(
        HWND owner);

    virtual std::shared_ptr<class VstPlugin> LoadPlugin(
        const std::wstring &filename);

    virtual std::shared_ptr<class VstPlugin> LoadPlugin(
        const std::string &filename);

    virtual std::shared_ptr<class VstPlugin> LoadFromFileDialog();

private:
    std::unique_ptr<sqlitelib::Sqlite> _db;
    HWND _owner;
    std::map<std::wstring, struct VstPluginDescription> _loadedPlugins;

    void EnsurePluginDescription(
        struct VstPluginDescription desc);
};

#endif // WIN32VSTPLUGINSERVICE_H
