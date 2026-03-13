#pragma once

#include "Peripheral.hpp"
#include "Device.hpp"
#include "Window.hpp"

#include <yaml-cpp/yaml.h>

namespace emu
{

namespace Registry
{

namespace impl
{

template<typename T, typename... Args>
concept ConstructibleWithConfig = std::is_constructible_v<T, const YAML::Node&, Args...>;

using ConfigType = std::reference_wrapper<const YAML::Node>;

using PeripheralFactoryFn = std::function<std::shared_ptr<Peripheral>()>;
using DeviceFactoryFn = std::function<std::shared_ptr<Device>(std::optional<ConfigType>)>;

void RegisterPeripheralFactory(const std::string& id, PeripheralFactoryFn factoryFn);
void RegisterDeviceFactory(const std::string& id, DeviceFactoryFn factoryFn);
void RegisterWindow(std::shared_ptr<Window>&& window);

} // namespace impl

template<std::derived_from<Peripheral> T, typename... Args>
void RegisterPeripheral(const std::string& id, Args&&... args)
{
    impl::RegisterPeripheralFactory(id,
        [... args = std::forward<Args>(args)]() -> std::shared_ptr<Peripheral> {
            return std::make_shared<T>(std::forward<Args>(args)...);
        }
    );
}

template<std::derived_from<Device> T, typename... Args>
void RegisterDevice(const std::string& id, Args&&... args)
{
    impl::RegisterDeviceFactory(id,
        [... args = std::forward<Args>(args)](std::optional<impl::ConfigType> config) -> std::shared_ptr<Device> {
            if constexpr (impl::ConstructibleWithConfig<T, Args...>) {
                if (config) {
                    return std::make_shared<T>(config->get(), std::forward<Args>(args)...);
                }
            }

            return std::make_shared<T>(std::forward<Args>(args)...);
        }
    );
}

template<std::derived_from<Window> T, typename... Args>
void RegisterWindow(Args&&... args)
{
    impl::RegisterWindow(std::make_shared<T>(std::forward<Args>(args)...));
}

using LoaderFn = std::function<bool(Emulator*, const YAML::Node&)>;
void RegisterLoader(const std::string& id, LoaderFn loader);

std::shared_ptr<Peripheral> CreatePeripheral(const std::string& id);
std::shared_ptr<Device> CreateDevice(const std::string& id);
std::shared_ptr<Device> CreateDevice(const std::string& id, const YAML::Node& config);
const std::list<std::shared_ptr<Window>>& GetWindows();
std::optional<LoaderFn> GetLoader(const std::string& id);

} // namespace Registry

} // namespace emu
