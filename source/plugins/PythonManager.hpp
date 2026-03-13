#pragma once

#include "common.hpp"
#include <list>
#include <functional>

#include <pybind11/pybind11.h>

namespace emu
{

class Emulator;

class PythonManager
{
public:
    PythonManager() = delete;

    static void Init();
    static void Deinit();

    static bool LoadPlugins(const std::string& path);
    static void StartPlugins(Emulator* emulator);
    static void StartFuzzing(Emulator* emulator, std::span<const uint8_t> input);

    static bool GenerateStub(const std::string& module, const std::string& outputPath);

    using StartCallbackFn = std::function<void(Emulator*)>;
    using FuzzingStartCallbackFn = std::function<void(Emulator*, std::span<const uint8_t>)>;

    static void RegisterStartCallback(StartCallbackFn fn);
    static void SetFuzzingStartCallback(FuzzingStartCallbackFn fn);

private:
    static inline PyThreadState* sThreadState = nullptr;
    static inline std::list<StartCallbackFn> sStartCallbacks;
    static inline FuzzingStartCallbackFn sFuzzingStartCallback;
};

} // namespace emu
