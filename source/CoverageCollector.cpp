#include "CoverageCollector.hpp"

#include <cstdio>
#include <string_view>
#include <print>

namespace
{

// TODO which versions to support?
constexpr uint32_t sDrcovVersion = 3;
// TODO what flavor makes sense here?
constexpr std::string_view sDrcovFlavor = "drcov"; // "emuframework"

} // namespace

CoverageCollector::CoverageCollector()
{
}

CoverageCollector::~CoverageCollector()
{
}

void CoverageCollector::UpdateCodeCoverage(uint32_t address, uint32_t size)
{
    mCoverage.emplace(address, size);
}

bool CoverageCollector::WriteDRCOV(const std::string& path)
{
    auto f = std::unique_ptr<std::FILE, int(*)(std::FILE*)>(std::fopen(path.c_str(), "wb"), &std::fclose);
    if (!f) {
        EMU_LOG_ERROR("Failed to create file");
        return false;
    }

    // Write version and flavor
    std::println(f.get(), "DRCOV VERSION: {}", sDrcovVersion);
    std::println(f.get(), "DRCOV FLAVOR: {}", sDrcovFlavor);

    std::println(f.get(), "Module Table: version 2, count 1");
    std::println(f.get(), "Columns: id, base, end, entry, checksum, timestamp, path");
    std::println(f.get(), "0, 0x00000000, 0xffffffff, 0x0000000000000000, 0x00000000, 0x00000000, ram");

    std::println(f.get(), "BB Table: {} bbs", mCoverage.size());

    // Great, time to switch to binary
    auto write32 = [&f](uint32_t val) { std::fwrite(&val, 1, sizeof(val), f.get()); };
    auto write16 = [&f](uint16_t val) { std::fwrite(&val, 1, sizeof(val), f.get()); };
    for (const auto& entry : mCoverage) {
        write32(entry.first);
        write16(static_cast<uint16_t>(entry.second));
        write16(0);
    }

    return true;
}

bool CoverageCollector::WriteEZCOV(const std::string& path)
{
    auto f = std::unique_ptr<std::FILE, int(*)(std::FILE*)>(std::fopen(path.c_str(), "w"), &std::fclose);
    if (!f) {
        EMU_LOG_ERROR("Failed to create file");
        return false;
    }

    std::println(f.get(), "EZCOV VERSION: 1");
    for (const auto& entry : mCoverage) {
        std::println(f.get(), "0x{:08X},{:>8}, [  ]", entry.first, entry.second);
    }

    return true;
}
