#include "common.hpp"

#include <cstdio>
#include <sstream>

namespace
{

#define COLOR_RED     "\x1B[31m"
#define COLOR_YELLOW  "\x1B[33m"
#define COLOR_WHITE   "\x1B[37m"
#define COLOR_RESET   "\x1B[0m"

emu::LogType sLogLevel = emu::LogType::Info;

} // namespace

namespace emu
{

std::vector<std::string> SplitString(const std::string& string, char delim)
{
    std::vector<std::string> seglist;

    std::stringstream stream(string);
    std::string segment;
    while (std::getline(stream, segment, delim)) {
        seglist.push_back(segment);
    }

    return seglist;
}

void SetLogLevel(LogType type)
{
    sLogLevel = type;
}

bool ShouldLog(LogType type)
{
    return sLogLevel <= type;
}

const char* GetLogPrefix(LogType type)
{
    switch (type) {
        case LogType::Debug:
            return "[DEBUG]\t";
            break;
        case LogType::Info:
            return COLOR_WHITE "[INFO]\t";
            break;
        case LogType::Warn:
            return COLOR_YELLOW "[WARN]\t";
            break;
        case LogType::Error:
            return COLOR_RED "[ERROR]\t";
            break;
    }

    return "";
}

const char* GetLogSuffix(LogType type)
{
    switch (type) {
        case LogType::Info:
        case LogType::Warn:
        case LogType::Error:
            return COLOR_RESET;
        default:
            return "";
    }
}

// https://stackoverflow.com/a/30606613/11511475
std::vector<uint8_t> HexToBytes(const std::string& hex)
{
    std::vector<uint8_t> bytes;

    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        char byte = (char) strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }

    return bytes;
}

std::string BytesToHex(const void* data, size_t size)
{
    std::string hex;

    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; i++) {
        hex += std::format("{:02x}", ptr[i]);
    }

    return hex;
}

}; // namespace saveling
