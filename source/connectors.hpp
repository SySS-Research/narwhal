#pragma once

#include "Connector.hpp"

struct USARTDataViewImpl
{
    static constexpr std::string_view sTypeId = "emu.usart.data:v1";

    using CallSignature = void(uint32_t);

    static inline void Serialize(emu::Connector& connector, uint32_t data)
    {
        connector(uint64_t(data), 0, {});
    }

    static inline uint64_t Deserialize(uint64_t arg0, uint64_t arg1, std::span<const uint8_t> data,
                                        std::function<CallSignature>& fn)
    {
        fn(static_cast<uint32_t>(arg0));
        return 0;
    }
};

struct MMCCommandViewImpl
{
    static constexpr std::string_view sTypeId = "emu.mmc.command:v1";

    using CallSignature = void(uint32_t, uint32_t, uint32_t);

    static inline void Serialize(emu::Connector& connector, uint32_t cmd, uint32_t arg, uint32_t dataSize)
    {
        uint8_t extraArg[4];
        extraArg[0] = (dataSize >> 24) & 0xFF;
        extraArg[1] = (dataSize >> 16) & 0xFF;
        extraArg[2] = (dataSize >> 8) & 0xFF;
        extraArg[3] = dataSize & 0xFF;
        connector(uint64_t(cmd), uint64_t(arg), extraArg);
    }

    static inline uint64_t Deserialize(uint64_t arg0, uint64_t arg1, std::span<const uint8_t> data,
                                        std::function<CallSignature>& fn)
    {
        uint32_t dataSize = (static_cast<uint32_t>(data[0]) << 24) |
                            (static_cast<uint32_t>(data[1]) << 16) |
                            (static_cast<uint32_t>(data[2]) << 8) |
                            static_cast<uint32_t>(data[3]);
        fn(static_cast<uint32_t>(arg0), static_cast<uint32_t>(arg1), dataSize);
        return 0;
    }
};

struct MMCDataViewImpl
{
    static constexpr std::string_view sTypeId = "emu.mmc.data:v1";

    using CallSignature = void(uint32_t, std::span<const uint8_t>);

    static inline void Serialize(emu::Connector& connector, uint32_t cmd, std::span<const uint8_t> data)
    {
        connector(uint64_t(cmd), 0, data);
    }

    static inline uint64_t Deserialize(uint64_t arg0, uint64_t arg1, std::span<const uint8_t> data,
                                        std::function<CallSignature>& fn)
    {
        fn(static_cast<uint32_t>(arg0), data);
        return 0;
    }
};

// TODO rename to bus interface
struct ST7789CommandViewImpl
{
    static constexpr std::string_view sTypeId = "emu.st7789.command:v1";

    using CallSignature = void(uint8_t);

    static inline void Serialize(emu::Connector& connector, uint8_t cmd)
    {
        connector(uint64_t(cmd), 0, {});
    }

    static inline uint64_t Deserialize(uint64_t arg0, uint64_t arg1, std::span<const uint8_t> data,
                                        std::function<CallSignature>& fn)
    {
        fn(static_cast<uint8_t>(arg0));
        return 0;
    }
};

struct ST7789DataViewImpl
{
    static constexpr std::string_view sTypeId = "emu.st7789.data:v1";

    using CallSignature = void(uint32_t, size_t);

    static inline void Serialize(emu::Connector& connector, uint32_t data, size_t size)
    {
        connector(uint64_t(data), uint64_t(size), {});
    }

    static inline uint64_t Deserialize(uint64_t arg0, uint64_t arg1, std::span<const uint8_t> data,
                                        std::function<CallSignature>& fn)
    {
        fn(static_cast<uint32_t>(arg0), static_cast<size_t>(arg1));
        return 0;
    }
};

struct GPIOViewImpl
{
    static constexpr std::string_view sTypeId = "emu.gpio:v1";

    using CallSignature = void(bool);

    static inline void Serialize(emu::Connector& connector, bool state)
    {
        connector(uint64_t(state), 0, {});
    }

    static inline uint64_t Deserialize(uint64_t arg0, uint64_t arg1, std::span<const uint8_t> data,
                                        std::function<CallSignature>& fn)
    {
        fn(!!arg0);
        return 0;
    }
};

using USARTDataConnectorView = emu::ConnectorView<USARTDataViewImpl>;
using MMCCommandConnectorView = emu::ConnectorView<MMCCommandViewImpl>;
using MMCDataConnectorView = emu::ConnectorView<MMCDataViewImpl>;
using ST7789CommandConnectorView = emu::ConnectorView<ST7789CommandViewImpl>;
using ST7789DataConnectorView = emu::ConnectorView<ST7789DataViewImpl>;
using GPIOConnectorView = emu::ConnectorView<GPIOViewImpl>;
