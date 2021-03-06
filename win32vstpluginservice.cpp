#include "win32vstpluginservice.h"

#include "vstplugin.h"

Win32VstPluginService::Win32VstPluginService(
    HWND owner)
    : _owner(owner)
{
}

VstPlugin *Win32VstPluginService::LoadFromFileDialog()
{
    char fn[MAX_PATH + 1] = {'\0'};
    OPENFILENAME ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "VSTi DLL(*.dll)\0*.dll\0All Files(*.*)\0*.*\0\0";
    ofn.lpstrFile = fn;
    ofn.nMaxFile = _countof(fn);
    ofn.lpstrTitle = "Select VST DLL";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_ENABLESIZING;
    ofn.hwndOwner = _owner;

    if (GetOpenFileName(&ofn) == TRUE)
    {
        auto result = new VstPlugin();
        result->init(fn);
        return result;
    }

    return nullptr;
}

const std::vector<VstPlugin *> &Win32VstPluginService::LoadedPlugins() const
{
    return _loadedPlugins;
}
