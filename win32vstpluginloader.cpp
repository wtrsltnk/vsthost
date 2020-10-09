#include "win32vstpluginloader.h"

#include "vstplugin.h"

Win32VstPluginLoader::Win32VstPluginLoader(
    HWND owner)
    : _owner(owner)
{
}

VstPlugin *Win32VstPluginLoader::LoadFromFileDialog()
{
    wchar_t fn[MAX_PATH + 1] = {'\0'};
    OPENFILENAME ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = L"VSTi DLL(*.dll)\0*.dll\0All Files(*.*)\0*.*\0\0";
    ofn.lpstrFile = fn;
    ofn.nMaxFile = _countof(fn);
    ofn.lpstrTitle = L"Select VST DLL";
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
