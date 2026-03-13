#include "FileLoader.hpp"
#include "common.hpp"
#include "Registry.hpp"

#include <cstdio>
#include <fstream>
#include <elfio/elfio.hpp>

namespace
{

std::vector<uint8_t> HexStringToBytes(const std::string& hexstring)
{
    std::vector<uint8_t> bytes;

    for (size_t i = 0; i < hexstring.size(); i += 2) {
        bytes.push_back(std::stoul(hexstring.substr(i, 2), nullptr, 16));
    }

    return bytes;
}

uint8_t CalculateSRECChecksum(const std::string& hexstring)
{
    uint8_t checksum = 0;
    auto bytes = HexStringToBytes(hexstring);

    for (auto b : bytes) {
        checksum += b;
    }

    return 0xff - (checksum & 0xff);
}

} // namespace

bool FileLoader::LoadBin(emu::Emulator* emulator, uint32_t address, const std::string& path, size_t offset)
{
    EMU_LOG_DEBUG("Loading {}", path);

    auto f = std::unique_ptr<std::FILE, int(*)(std::FILE*)>(std::fopen(path.c_str(), "rb"), &std::fclose);
    if (!f) {
        EMU_LOG_ERROR("Failed to open file");
        return false;
    }

    if (std::fseek(f.get(), offset, SEEK_SET) == -1) {
        EMU_LOG_ERROR("Failed to seek file");
        return false;
    }

    uint8_t buffer[256];
    while (!std::feof(f.get())) {
        size_t size = std::fread(buffer, 1, sizeof(buffer), f.get());
        emulator->WriteMemory(address, buffer, size);
        address += size;
    }

    if (std::ferror(f.get())) {
        return false;
    }

    return true;
}

bool FileLoader::LoadElf(emu::Emulator* emulator, const std::string& path)
{
    EMU_LOG_DEBUG("Loading {}", path);

    ELFIO::elfio reader;
    if (!reader.load(path)) {
        return false;
    }

    // TODO verify elf


    // Print ELF file segments info
    ELFIO::Elf_Half seg_num = reader.segments.size();
    EMU_LOG_DEBUG("Number of segments: {}", seg_num);

    for (int i = 0; i < seg_num; ++i) {
        const ELFIO::segment* pseg = reader.segments[i];
        EMU_LOG_DEBUG("[ {} ] 0x{:x}\t0x{:x}\t0x{:x}\t0x{:x}", i, pseg->get_flags(), pseg->get_virtual_address(), pseg->get_file_size(), pseg->get_memory_size());

        // Access segments's data
        const char* p = reader.segments[i]->get_data();
        if (p) {
            emulator->WriteMemory(pseg->get_physical_address(), p, pseg->get_file_size());
        }
    }

    return true;
}

bool FileLoader::LoadSREC(emu::Emulator* emulator, const std::string& path, uint32_t& outStartAddress)
{
    EMU_LOG_DEBUG("Loading {}", path);

    // out address might be missing from file
    outStartAddress = 0;

    std::ifstream f(path);
    if (!f.is_open()) {
        EMU_LOG_ERROR("Failed to open file");
        return false;
    }

    std::string line;
    while (std::getline(f, line)) {
        if (line.length() < 6) {
            EMU_LOG_ERROR("Record too short");
            return false;
        }

        if (line[0] != 'S') {
            EMU_LOG_ERROR("Invalid record start");
            return false;
        }

        char type = line[1];

        // Figure out how many "address" characters follow
        size_t addressChars = 0;
        switch (type) {
            case '0': // Header
            case '1': // 16-bit data
            case '5': // 16-bit count
            case '9': // 16-bit start address
                addressChars = 4;
                break;
            case '2': // 24-bit data
            case '6': // 24-bit count
            case '8': // 24-bit start address
                addressChars = 6;
                break;
            case '3': // 32-bit data
            case '7': // 32-bit start address
                addressChars = 8;
                break;
            default:
                EMU_LOG_ERROR("Invalid record type");
                return false;
        }

        // Verify checksum
        auto checksum = std::stoul(line.substr(line.length() - 2, 2), nullptr, 16);
        if (CalculateSRECChecksum(line.substr(2, line.length() - 4)) != checksum) {
            EMU_LOG_ERROR("Invalid checksum");
            return false;
        }

        // Read and verify byte count
        auto byteCount = std::stoul(line.substr(2, 2), nullptr, 16);
        if (byteCount + 3 >= line.length()) {
            EMU_LOG_ERROR("Not enough data in record");
            return false;
        }

        // Read address
        auto address = std::stoul(line.substr(4, addressChars), nullptr, 16);

        // Read data
        auto dataChars = byteCount * 2;
        auto data = HexStringToBytes(line.substr(4 + addressChars, dataChars));

        // Handle record
        switch (type) {
            case '0': // Header
                break;
            case '1': // 16-bit data
            case '2': // 24-bit data
            case '3': // 32-bit data
                emulator->WriteMemory(address, data);
                break;
            case '5': // 16-bit count
            case '6': // 24-bit count
                break;
            case '7': // 32-bit start address
            case '8': // 24-bit start address
            case '9': // 16-bit start address
                outStartAddress = address;
                break;
            default:
                EMU_LOG_ERROR("Invalid record type");
                return false;
        }
    }

    if (f.bad()) {
        return false;
    }

    return true;
}

void FileLoader::RegisterLoaders()
{
    emu::Registry::RegisterLoader("emu.loader.bin", [](emu::Emulator* emulator, const YAML::Node& config) -> bool {
        size_t offset = config["offset"] ? config["offset"].as<size_t>() : 0;
        return FileLoader::LoadBin(emulator, config["address"].as<uint32_t>(), config["file"].as<std::string>(), offset);
    });
    emu::Registry::RegisterLoader("emu.loader.elf", [](emu::Emulator* emulator, const YAML::Node& config) -> bool {
        return FileLoader::LoadElf(emulator, config["file"].as<std::string>());
    });
    emu::Registry::RegisterLoader("emu.loader.srec", [](emu::Emulator* emulator, const YAML::Node& config) -> bool {
        uint32_t startAddress;
        return FileLoader::LoadSREC(emulator, config["file"].as<std::string>(), startAddress);
    });
    emu::Registry::RegisterLoader("emu.loader.imm32", [](emu::Emulator* emulator, const YAML::Node& config) -> bool {
        uint32_t address = config["address"].as<uint32_t>();
        uint32_t value = config["value"].as<uint32_t>();
        return emulator->WriteMemory(address, &value, sizeof(value));
    });
    emu::Registry::RegisterLoader("emu.loader.imm16", [](emu::Emulator* emulator, const YAML::Node& config) -> bool {
        uint32_t address = config["address"].as<uint32_t>();
        uint16_t value = config["value"].as<uint16_t>();
        return emulator->WriteMemory(address, &value, sizeof(value));
    });
    emu::Registry::RegisterLoader("emu.loader.imm8", [](emu::Emulator* emulator, const YAML::Node& config) -> bool {
        uint32_t address = config["address"].as<uint32_t>();
        uint8_t value = config["value"].as<uint8_t>();
        return emulator->WriteMemory(address, &value, sizeof(value));
    });
}
