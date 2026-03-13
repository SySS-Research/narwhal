#include "Configuration.hpp"

namespace emu
{

Configuration::Configuration()
{
}

Configuration::~Configuration()
{
}

std::shared_ptr<Configuration> Configuration::Load(const std::string& path)
{
    auto conf = std::make_shared<Configuration>();

    try {
        if (conf->LoadFromFile(path)) {
            return conf;
        }
    } catch (const YAML::Exception& e) {
        EMU_LOG_ERROR("Failed to parse config: {}", e.what());
    }
    
    return nullptr;
}

bool Configuration::LoadFromFile(const std::string& path)
{
    YAML::Node config = YAML::LoadFile(path);

    return LoadCpuConfig(config["cpu"])
        && LoadMemoryMapConfig(config["memorymap"])
        && LoadMemoryLoaders(config["loaders"])
        && LoadPeripherals(config["peripherals"])
        && LoadDevices(config["devices"])
        && LoadConnectors(config["connectors"]);
}

bool Configuration::LoadCpuConfig(YAML::Node cpuConfig)
{
    if (!cpuConfig) {
        EMU_LOG_ERROR("Missing cpu configuration");
        return false;
    }

    mCpuType = cpuConfig["type"].as<std::string>();
    mCpuId = cpuConfig["cpuid"].as<uint32_t>();
    mCpuVtor = cpuConfig["vtor"].as<uint32_t>();
    return true;
}

bool Configuration::LoadMemoryMapConfig(YAML::Node memorymapConfig)
{
    if (!memorymapConfig) {
        EMU_LOG_ERROR("Missing memorymap configuration");
        return false;
    }

    for (auto it = memorymapConfig.begin(); it != memorymapConfig.end(); it++) {
        auto& entry = mMemoryMap.emplace_back();

        std::string type = (*it)["type"].as<std::string>();
        emu::MemoryFlags flags{};
        if (type == "ram") {
            flags = emu::MemoryFlags::RWX;
        } else if (type == "rom") {
            flags = emu::MemoryFlags::Read | emu::MemoryFlags::Exec;
        } else if (type == "mmio") {
            flags = emu::MemoryFlags::MMIO;
        } else {
            EMU_LOG_ERROR("Unknown memory type '{}'", type.c_str());
            return false;
        }

        entry.flags = flags;
        entry.start = (*it)["start"].as<uint32_t>();
        entry.end = (*it)["end"].as<uint32_t>();
    }

    return true;
}

bool Configuration::LoadMemoryLoaders(YAML::Node loadersConfig)
{
    if (!loadersConfig) {
        EMU_LOG_ERROR("Missing loaders configuration");
        return false;
    }

    for (auto it = loadersConfig.begin(); it != loadersConfig.end(); it++) {
        auto& entry = mMemoryLoaders.emplace_back();

        std::string id = (*it)["id"].as<std::string>();
        auto loaderFn = emu::Registry::GetLoader(id);
        if (!loaderFn) {
            EMU_LOG_ERROR("Failed to find loader '{}'", id);
            return false;
        }

        entry.function = *loaderFn;
        entry.config = YAML::Node(*it);
    }

    return true;
}

bool Configuration::LoadPeripherals(YAML::Node peripheralsConfig)
{
    if (!peripheralsConfig) {
        EMU_LOG_ERROR("Missing peripherals configuration");
        return false;
    }

    for (auto it = peripheralsConfig.begin(); it != peripheralsConfig.end(); it++) {
        std::string id = (*it)["id"].as<std::string>();
        auto peripheral = emu::Registry::CreatePeripheral(id);
        if (!peripheral) {
            EMU_LOG_ERROR("Failed to create peripheral: '{}'", id);
            return false;
        }

        peripheral->SetBounds((*it)["start"].as<uint32_t>(), (*it)["end"].as<uint32_t>());

        if ((*it)["identifier"]) {
            peripheral->SetIdentifier((*it)["identifier"].as<std::string>());
            mIdentifiedConnectables[(*it)["identifier"].as<std::string>()] = peripheral;
        }

        peripheral->SetName(peripheral->GetIdentifier() ? *peripheral->GetIdentifier() : id);

        mPeripherals.push_back(peripheral);
    }

    return true;
}

bool Configuration::LoadDevices(YAML::Node devicesConfig)
{
    if (!devicesConfig) {
        EMU_LOG_ERROR("Missing devices configuration");
        return false;
    }

    for (auto it = devicesConfig.begin(); it != devicesConfig.end(); it++) {
        std::string id = (*it)["id"].as<std::string>();
        auto device = emu::Registry::CreateDevice(id, *it);
        if (!device) {
            EMU_LOG_ERROR("Failed to create Device: '{}'", id);
            continue;
        }

        if ((*it)["identifier"]) {
            device->SetIdentifier((*it)["identifier"].as<std::string>());
            mIdentifiedConnectables[(*it)["identifier"].as<std::string>()] = device;
        }

        device->SetName(device->GetIdentifier() ? *device->GetIdentifier() : id);

        mDevices.push_back(device);
    }

    return true;
}

bool Configuration::LoadConnectors(YAML::Node connectorsConfig)
{
    if (!connectorsConfig) {
        EMU_LOG_ERROR("Missing connectors configuration");
        return false;
    }

    for (auto it = connectorsConfig.begin(); it != connectorsConfig.end(); it++) {
        YAML::Node from = (*it)["from"];
        YAML::Node to = (*it)["to"];
        auto fromIdentifier = from["identifier"].as<std::string>();
        auto toIdentifier = to["identifier"].as<std::string>();
        auto fromPort = from["port"].as<std::string>();
        auto toPort = to["port"].as<std::string>();

        auto fromConnectable = mIdentifiedConnectables.find(fromIdentifier);
        auto toConnectable = mIdentifiedConnectables.find(toIdentifier);
        if (fromConnectable == mIdentifiedConnectables.end() || toConnectable == mIdentifiedConnectables.end()) {
            EMU_LOG_ERROR("Failed to find connector");
            return false;
        }

        auto fromConnector = fromConnectable->second->GetOutputConnectors().Get(fromPort);
        auto toConnector = toConnectable->second->GetInputConnectors().Get(toPort);
        if (!fromConnector || !toConnector) {
            EMU_LOG_ERROR("Failed to find connector");
            return false;
        }

        // Connect the two
        fromConnector->Connect(toConnector);

        // Insert into connections for node editor
        mConnections.emplace_back(std::make_tuple(fromIdentifier, fromPort, toIdentifier, toPort));
    }

    return true;
}

} // namespace emu    
