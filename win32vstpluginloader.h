#ifndef WIN32VSTPLUGINLOADER_H
#define WIN32VSTPLUGINLOADER_H

#include "ivstpluginloader.h"
#include <windows.h>

class Win32VstPluginLoader :
    public IVstPluginLoader
{
public:
    Win32VstPluginLoader(
        HWND owner);

    virtual class VstPlugin *LoadFromFileDialog();
private:
    HWND _owner;
};

#endif // WIN32VSTPLUGINLOADER_H
