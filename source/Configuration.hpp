#pragma once

#include <memory>
#include <string>
#include <yaml-cpp/yaml.h>

#include "common.hpp"
#include "Registry.hpp"

namespace emu
{

class Configuration
{
public:
    Configuration();
    ~Configuration();

    static std::shared_ptr<Configuration> Load(const std::string& path);
    // TODO Save

    const std::string& GetCpuType() const { return mCpuType; }
    uint32_t GetCpuId() const { return mCpuId; }
    uint32_t GetCpuVtor() const { return mCpuVtor; }

    struct MemoryMapEntry
    {
        emu::MemoryFlags flags;
        uint32_t start;
        uint32_t end;
    };
    const std::list<MemoryMapEntry>& GetMemoryMap() const { return mMemoryMap; }

    struct LoaderEntry
    {
        Registry::LoaderFn function;
        YAML::Node config;
    };
    const std::list<LoaderEntry>& GetMemoryLoaders() const { return mMemoryLoaders; }

    const std::list<std::shared_ptr<Peripheral>>& GetPeripherals() const { return mPeripherals; }

    const std::list<std::shared_ptr<Device>>& GetDevices() const { return mDevices; }

    const std::list<std::tuple<std::string, std::string, std::string, std::string>>& GetConnections() const { return mConnections; }

private:
    bool LoadFromFile(const std::string& path);

    bool LoadCpuConfig(YAML::Node cpuConfig);
    bool LoadMemoryMapConfig(YAML::Node memorymapConfig);
    bool LoadMemoryLoaders(YAML::Node loadersConfig);
    bool LoadPeripherals(YAML::Node peripheralsConfig);
    bool LoadDevices(YAML::Node devicesConfig);
    bool LoadConnectors(YAML::Node connectorsConfig);

private:
    std::string mCpuType;
    uint32_t mCpuId;
    uint32_t mCpuVtor;

    std::list<MemoryMapEntry> mMemoryMap;
    std::list<LoaderEntry> mMemoryLoaders;
    std::list<std::shared_ptr<Peripheral>> mPeripherals;
    std::list<std::shared_ptr<Device>> mDevices;

    // Peripherals and Devices which have an identifier in the config
    std::unordered_map<std::string, std::shared_ptr<emu::IConnectable>> mIdentifiedConnectables;

    // TODO not great
    std::list<std::tuple<std::string, std::string, std::string, std::string>> mConnections;
};

} // namespace emu
