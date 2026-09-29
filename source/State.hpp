#pragma once

#include "common.hpp"
#include "arm.hpp"
#include <bitset>

namespace emu
{

class State
{
public:
    State();
    ~State();

    void Store(std::string path);
    bool Load(std::string path);

    std::size_t Read(const std::span<std::byte>& data);
    std::size_t Write(const std::span<const std::byte>& data);

    bool SetPosition(std::size_t position);
    std::size_t GetPosition() const;
    std::size_t GetRemaining() const;

    // State read operators
    template<std::integral T>
    State& operator>>(T& val)
    {
        val = 0;
        Read(std::as_writable_bytes(std::span(std::addressof(val), 1)));
        return *this;
    }

    template<typename T>
    State& operator>>(arm::RegisterBaseT<T>& reg)
    {
        T i;
        *this >> i;
        reg = i;
        return *this;
    }

    template<typename T, std::size_t N>
    State& operator>>(std::array<T, N>& array)
    {
        for (size_t i = 0; i < N; i++) {
            *this >> array[i];
        }
        return *this;
    }

    template<std::size_t N>
    State& operator>>(std::bitset<N>& bitset)
    {
        // TODO do not store individual bits as bytes
        for (size_t i = 0; i < N; i++) {
            bool val;
            *this >> val;
            bitset[i] = val;
        }
        return *this;
    }

    State& operator>>(bool& val);
    State& operator>>(float& val);
    State& operator>>(double& val);

    // State write operators
    template<std::integral T>
    State& operator<<(T val)
    {
        Write(std::as_bytes(std::span(std::addressof(val), 1)));
        return *this;
    }

    template<typename T>
    State& operator<<(arm::RegisterBaseT<T>& reg)
    {
        T i = reg;
        *this << i;
        return *this;
    }

    template<typename T, std::size_t N>
    State& operator<<(std::array<T, N>& array)
    {
        for (size_t i = 0; i < N; i++) {
            *this << array[i];
        }
        return *this;
    }

    template<std::size_t N>
    State& operator<<(std::bitset<N>& bitset)
    {
        // TODO do not store individual bits as bytes
        for (size_t i = 0; i < N; i++) {
            *this << bitset[i];
        }
        return *this;
    }

    State& operator<<(bool val);
    State& operator<<(float val);
    State& operator<<(double val);

private:
    std::vector<std::byte> mStateBuffer;
    std::size_t mPosition;
};

} // namespace emu
