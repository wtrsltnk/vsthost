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

    virtual std::unique_ptr<class VstPlugin> LoadFromFileDialog();

private:
    HWND _owner;
};

#endif // WIN32VSTPLUGINSERVICE_H
