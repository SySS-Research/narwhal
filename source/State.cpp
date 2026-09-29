#include "State.hpp"

namespace emu
{

State::State()
 : mStateBuffer(),
   mPosition(0)
{
}

State::~State()
{
}

void State::Store(std::string path)
{
    auto f = std::unique_ptr<std::FILE, int(*)(std::FILE*)>(std::fopen(path.c_str(), "wb"), &std::fclose);
    if (f) {
        std::fwrite(mStateBuffer.data(), 1, mStateBuffer.size(), f.get());
    }
}

bool State::Load(std::string path)
{
    auto f = std::unique_ptr<std::FILE, int(*)(std::FILE*)>(std::fopen(path.c_str(), "rb"), &std::fclose);
    if (f) {
        std::fseek(f.get(), 0, SEEK_END);
        size_t size = std::ftell(f.get());
        std::fseek(f.get(), 0, SEEK_SET);

        mStateBuffer.resize(size);
        std::fread(mStateBuffer.data(), 1, size, f.get());
        return true;
    }

    return false;
}

std::size_t State::Read(const std::span<std::byte>& data)
{
    if (data.size() > GetRemaining()) {
        return 0;
    }

    std::copy_n(mStateBuffer.begin() + mPosition, data.size(), data.begin());
    mPosition += data.size();
    return data.size();
}

std::size_t State::Write(const std::span<const std::byte>& data)
{
    // Resize vector if not enough bytes remain
    if (mPosition + data.size() > mStateBuffer.size()) {
        mStateBuffer.resize(mPosition + data.size());
    }

    std::copy(data.begin(), data.end(), mStateBuffer.begin() + mPosition);
    mPosition += data.size();
    return data.size();
}

bool State::SetPosition(std::size_t position)
{
    if (position >= mStateBuffer.size()) {
        return false;
    }

    mPosition = position;
    return true;
}

std::size_t State::GetPosition() const
{
    return mPosition;
}

std::size_t State::GetRemaining() const
{
    return mStateBuffer.size() - mPosition;
}

State& State::operator>>(bool& val)
{
    std::uint8_t i;
    *this >> i;
    val = !!i;

    return *this;
}

State& State::operator>>(float& val)
{
    std::uint32_t i;
    *this >> i;
    val = std::bit_cast<float>(i);

    return *this;
}

State& State::operator>>(double& val)
{
    std::uint64_t i;
    *this >> i;
    val = std::bit_cast<double>(i);

    return *this;
}

State& State::operator<<(bool val)
{
    std::uint8_t i = val;
    *this << i;

    return *this;
}

State& State::operator<<(float val)
{
    std::uint32_t i = std::bit_cast<std::uint32_t>(val);
    *this << i;

    return *this;
}

State& State::operator<<(double val)
{
    std::uint64_t i = std::bit_cast<std::uint64_t>(val);
    *this << i;

    return *this;
}

} // namespace emu
