#pragma once

#include "common.hpp"
#include <set>

class CoverageCollector
{
public:
    CoverageCollector();
    ~CoverageCollector(); 

    void UpdateCodeCoverage(uint32_t address, uint32_t size);

    bool WriteDRCOV(const std::string& path);
    bool WriteEZCOV(const std::string& path);

private:
    // Use set for deduplicating data
    std::set<std::pair<uint32_t, uint32_t>> mCoverage;
};
