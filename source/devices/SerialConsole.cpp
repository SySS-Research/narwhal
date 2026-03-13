#include "SerialConsole.hpp"
#include "common.hpp"

#include "Registry.hpp"
#include "windows/SerialConsoleWindow.hpp"

SerialConsole::SerialConsole() : Device(),
 mRxConnectorView("rx"),
 mBuffer(),
 mBufferMutex()
{
    GetInputConnectors().Add(mRxConnectorView);

    mRxConnectorView.OnCall([this](uint32_t ch) {
        OnData(ch);
    });
}

SerialConsole::~SerialConsole()
{
}

void SerialConsole::Init()
{
    // TODO this is ... not great
    emu::Registry::RegisterWindow<SerialConsoleWindow>(this);
}

void SerialConsole::OnData(uint32_t ch)
{
    std::lock_guard lk(mBufferMutex);

    mBuffer += static_cast<char>(ch);

    // TODO don't let the buffer grow indefinitely

    // Flush buffer on newline
    // if (ch == '\n') {
    //     EMU_LOG_INFO("{}", mBuffer);
    //     mBuffer.clear();
    // }
}

std::string SerialConsole::GetBuffer()
{
    std::lock_guard lk(mBufferMutex);

    // Create a copy of the buffer
    // TODO not great, especially for large buffers
    return mBuffer;
}
