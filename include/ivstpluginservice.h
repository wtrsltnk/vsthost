#ifndef IVSTPLUGINSERVICE_H
#define IVSTPLUGINSERVICE_H

#include <memory>
#include <vector>

class VstPlugin;

class IVstPluginService
{
public:
    virtual ~IVstPluginService() = default;

    virtual std::unique_ptr<VstPlugin> LoadFromFileDialog() = 0;
};

#endif // IVSTPLUGINSERVICE_H
