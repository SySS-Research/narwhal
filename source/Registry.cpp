#include "Registry.hpp"

#include <unordered_map>

namespace emu::Registry
{

namespace impl
{

std::unordered_map<std::string, PeripheralFactoryFn> gPeripheralFactories;
std::unordered_map<std::string, DeviceFactoryFn> gDeviceFactories;
std::list<std::shared_ptr<Window>> gWindows;
std::unordered_map<std::string, LoaderFn> gLoaders;

void RegisterPeripheralFactory(const std::string& id, PeripheralFactoryFn factoryFn)
{
    if (gPeripheralFactories.contains(id)) {
        EMU_FATAL("Trying to register peripherals with the same Id");
    }

    gPeripheralFactories[id] = factoryFn;
}

void RegisterDeviceFactory(const std::string& id, DeviceFactoryFn factoryFn)
{
    if (gDeviceFactories.contains(id)) {
        EMU_FATAL("Trying to register Devices with the same Id");
    }

    gDeviceFactories[id] = factoryFn;
}

void RegisterWindow(std::shared_ptr<Window>&& window)
{
    gWindows.push_back(window);
}

} // namespace 

void RegisterLoader(const std::string& id, LoaderFn loader)
{
    if (impl::gLoaders.contains(id)) {
        EMU_FATAL("Trying to register Loaders with the same Id");
    }

    impl::gLoaders[id] = loader;
}

std::shared_ptr<Peripheral> CreatePeripheral(const std::string& id)
{
    auto factory = impl::gPeripheralFactories.find(id);
    if (factory == impl::gPeripheralFactories.end()) {
        return nullptr;
    }

    return factory->second();
}

std::shared_ptr<Device> CreateDevice(const std::string& id)
{
    auto factory = impl::gDeviceFactories.find(id);
    if (factory == impl::gDeviceFactories.end()) {
        return nullptr;
    }

    return factory->second(std::nullopt);
}

std::shared_ptr<Device> CreateDevice(const std::string& id, const YAML::Node& config)
{
    auto factory = impl::gDeviceFactories.find(id);
    if (factory == impl::gDeviceFactories.end()) {
        return nullptr;
    }

    return factory->second(config);
}

const std::list<std::shared_ptr<Window>>& GetWindows()
{
    return impl::gWindows;
}

std::optional<LoaderFn> GetLoader(const std::string& id)
{
    auto factory = impl::gLoaders.find(id);
    if (factory == impl::gLoaders.end()) {
        return std::nullopt;
    }

    return factory->second;
}

} // namespace emu::Registry
