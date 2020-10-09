#ifndef IVSTPLUGINLOADER_H
#define IVSTPLUGINLOADER_H

class IVstPluginLoader
{
public:
    virtual ~IVstPluginLoader() = default;

    virtual class VstPlugin *LoadFromFileDialog() = 0;
};

#endif // IVSTPLUGINLOADER_H
