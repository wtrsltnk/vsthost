#ifndef WIN32VSTPLUGINSERVICE_H
#define WIN32VSTPLUGINSERVICE_H

#include "ivstpluginservice.h"
#include <windows.h>

class Win32VstPluginService :
    public IVstPluginService
{
public:
    Win32VstPluginService(
        HWND owner);

    virtual class VstPlugin *LoadFromFileDialog();

    virtual const std::vector<VstPlugin *> &LoadedPlugins() const;

private:
    HWND _owner;
    std::vector<VstPlugin *> _loadedPlugins;
};

#endif // WIN32VSTPLUGINSERVICE_H
