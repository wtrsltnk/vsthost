#ifndef IVSTPLUGINSERVICE_H
#define IVSTPLUGINSERVICE_H

#include <memory>
#include <string>
#include <vector>

class VstPlugin;

class IVstPluginService
{
public:
    virtual ~IVstPluginService() = default;

    virtual std::shared_ptr<class VstPlugin> LoadPlugin(
        const std::wstring &filename) = 0;

    virtual std::shared_ptr<class VstPlugin> LoadPlugin(
        const std::string &filename) = 0;

    virtual std::shared_ptr<VstPlugin> LoadFromFileDialog() = 0;
};

#endif // IVSTPLUGINSERVICE_H
