#pragma once

#include <format>
#include <print>
#include <memory>
#include <vector>
#include <string>
#include <span>
#include <optional>
#include <array>
#include <cstdint>
#include <type_traits>
#include <bitset>

#define __FILENAME__ (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)

#define EMU_LOG(type, format, ...) emu::Log(type, "[{:>23}]{:>30}@L{:04} " format, __FILENAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define EMU_LOG_DEBUG(format, ...) EMU_LOG(emu::LogType::Debug, format, ##__VA_ARGS__)
#define EMU_LOG_INFO(format, ...)  EMU_LOG(emu::LogType::Info,  format, ##__VA_ARGS__)
#define EMU_LOG_WARN(format, ...)  EMU_LOG(emu::LogType::Warn,  format, ##__VA_ARGS__)
#define EMU_LOG_ERROR(format, ...) EMU_LOG(emu::LogType::Error, format, ##__VA_ARGS__)

#define EMU_FATAL(format, ...) {{ EMU_LOG_ERROR("FATAL: " format, ##__VA_ARGS__); exit(1); }}

#define EMU_ASSERT(expr) {{ if (!(expr)) EMU_FATAL("Assertion `" #expr "` failed."); }}

#define ENUM_BITMASK_TYPE(_type)                                                                                                                     \
    extern "C++" {                                                                                                                                   \
    namespace                                                                                                                                        \
    {                                                                                                                                                \
    constexpr inline _type                                                                                                                           \
    operator~(_type lhs)                                                                                                                             \
    {                                                                                                                                                \
        return static_cast<_type>(~static_cast<std::underlying_type_t<_type>>(lhs));                                                                 \
    }                                                                                                                                                \
    inline bool                                                                                                                                      \
    operator!(_type lhs)                                                                                                                             \
    {                                                                                                                                                \
        return !static_cast<std::underlying_type_t<_type>>(lhs);                                                                                     \
    }                                                                                                                                                \
    constexpr inline _type                                                                                                                           \
    operator&(_type lhs, _type rhs)                                                                                                                  \
    {                                                                                                                                                \
        return static_cast<_type>(static_cast<std::underlying_type_t<_type>>(lhs) & static_cast<std::underlying_type_t<_type>>(rhs));                \
    }                                                                                                                                                \
    constexpr inline _type                                                                                                                           \
    operator|(_type lhs, _type rhs)                                                                                                                  \
    {                                                                                                                                                \
        return static_cast<_type>(static_cast<std::underlying_type_t<_type>>(lhs) | static_cast<std::underlying_type_t<_type>>(rhs));                \
    }                                                                                                                                                \
    constexpr inline _type                                                                                                                           \
    operator^(_type lhs, _type rhs)                                                                                                                  \
    {                                                                                                                                                \
        return static_cast<_type>(static_cast<std::underlying_type_t<_type>>(lhs) ^ static_cast<std::underlying_type_t<_type>>(rhs));                \
    }                                                                                                                                                \
    inline _type &                                                                                                                                   \
    operator&=(_type &lhs, _type rhs)                                                                                                                \
    {                                                                                                                                                \
        return reinterpret_cast<_type &>(reinterpret_cast<std::underlying_type_t<_type> &>(lhs) &= static_cast<std::underlying_type_t<_type>>(rhs)); \
    }                                                                                                                                                \
    inline _type &                                                                                                                                   \
    operator|=(_type &lhs, _type rhs)                                                                                                                \
    {                                                                                                                                                \
        return reinterpret_cast<_type &>(reinterpret_cast<std::underlying_type_t<_type> &>(lhs) |= static_cast<std::underlying_type_t<_type>>(rhs)); \
    }                                                                                                                                                \
    inline _type &                                                                                                                                   \
    operator^=(_type &lhs, _type rhs)                                                                                                                \
    {                                                                                                                                                \
        return reinterpret_cast<_type &>(reinterpret_cast<std::underlying_type_t<_type> &>(lhs) ^= static_cast<std::underlying_type_t<_type>>(rhs)); \
    }                                                                                                                                                \
    }                                                                                                                                                \
    }

namespace emu
{

std::vector<std::string> SplitString(const std::string& string, char delim);

enum class LogType
{
    Debug,
    Info,
    Warn,
    Error,
};

void SetLogLevel(LogType type);
bool ShouldLog(LogType type);
const char* GetLogPrefix(LogType type);
const char* GetLogSuffix(LogType type);

template<typename ...Args>
void Log(LogType type, std::format_string<Args...> format, Args ...args)
{
    if (!ShouldLog(type)) {
        return;
    }

    std::string line = std::format(format, std::forward<Args>(args)...);
    std::println(stderr, "{}{}{}", GetLogPrefix(type), line, GetLogSuffix(type));
}

enum class WatchpointType
{
    Read,
    Write,
    Access,
};

enum class MemoryFlags {
    Read    = (1u << 0),
    Write   = (1u << 1),
    Exec    = (1u << 2),
    RWX     = (Read | Write | Exec),

    // Special flag for Emulator class to allow registering memory-mapped peripherals in a region
    MMIO    = (1u << 3),
};

template <class E>
constexpr E from_underlying(std::underlying_type_t<E> v)
{
    return static_cast<E>(v);
}

template <typename T, size_t N>
T slice_bitset(const std::bitset<N>& bitset, size_t offset)
{
    static_assert(std::is_unsigned_v<T>);
    constexpr size_t width = sizeof(T) * 8;

    T val = 0;
    for (size_t i = 0; i < width; i++) {
        val |= T(bitset[offset + i]) << i;
    }

    return val;
}

std::vector<uint8_t> HexToBytes(const std::string& hex);
std::string BytesToHex(const void* data, size_t size);

} // namespace emu

ENUM_BITMASK_TYPE(emu::MemoryFlags);
