#ifndef IVSTPLUGINSERVICE_H
#define IVSTPLUGINSERVICE_H

#include <vector>

class VstPlugin;

class IVstPluginService
{
public:
    virtual ~IVstPluginService() = default;

    virtual VstPlugin *LoadFromFileDialog() = 0;

    virtual const std::vector<VstPlugin *> &LoadedPlugins() const = 0;
};

#endif // IVSTPLUGINSERVICE_H
