#include "PythonManager.hpp"
#include "Application.hpp"
#include "Registry.hpp"
#include "Emulator.hpp"
#include "python/PyDevice.hpp"
#include "python/PyPeripheral.hpp"
#include "python/PyWindow.hpp"

#include <print>
#include <filesystem>

#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/functional.h>
#include <pybind11/native_enum.h>

void py_init_module_imgui_main(pybind11::module_& m);
void py_init_module_imgui_internal(pybind11::module_& m);

namespace
{

pybind11::object ConvertFromYAML(const YAML::Node& node)
{
    switch (node.Type()) {
        case YAML::NodeType::Scalar:
            return pybind11::cast(node.as<std::string>());
        case YAML::NodeType::Sequence: {
            pybind11::list list;
            for (auto it = node.begin(); it != node.end(); it++) {
                list.append(ConvertFromYAML(*it));
            }
            return list;
        }
        case YAML::NodeType::Map: {
            pybind11::dict dict;
            for (auto it = node.begin(); it != node.end(); it++) {
                dict[it->first.as<std::string>().c_str()] = ConvertFromYAML(it->second);
            }
            return dict;
        }
        default:
            EMU_FATAL("Cannot convert from YAML");
            break;
    }
}

} // namespace

PYBIND11_EMBEDDED_MODULE(imgui, m)
{
    py_init_module_imgui_main(m);

    auto imgui_internal = m.def_submodule("internal");
    py_init_module_imgui_internal(imgui_internal);
}

PYBIND11_EMBEDDED_MODULE(emu, m)
{
    m.def("print", [](std::string s) {
        EMU_LOG_INFO("{}", s);
    }, pybind11::arg("s"));

    pybind11::native_enum<arm::Register>(m, "Register", "enum.Enum")
        .value("R0", arm::Register::R0)
        .value("R1", arm::Register::R1)
        .value("R2", arm::Register::R2)
        .value("R3", arm::Register::R3)
        .value("R4", arm::Register::R4)
        .value("R5", arm::Register::R5)
        .value("R6", arm::Register::R6)
        .value("R7", arm::Register::R7)
        .value("R8", arm::Register::R8)
        .value("R9", arm::Register::R9)
        .value("R10", arm::Register::R10)
        .value("R11", arm::Register::R11)
        .value("R12", arm::Register::R12)
        .value("SP", arm::Register::SP)
        .value("LR", arm::Register::LR)
        .value("PC", arm::Register::PC)
        .finalize();

    pybind11::class_<emu::Emulator>(m, "Emulator")
        .def("stop", &emu::Emulator::Stop)
        .def("get_register", &emu::Emulator::GetRegister)
        .def("set_register", &emu::Emulator::SetRegister)
        .def("read_memory", [](emu::Emulator& self, uint32_t address, uint32_t size) -> pybind11::object {
            std::vector<uint8_t> bytes;
            bytes.resize(size);

            if (!self.ReadMemory(address, bytes)) {
                return pybind11::none();
            }

            return pybind11::bytes(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        }, pybind11::arg("address"), pybind11::arg("size"))
        .def("write_memory", [](emu::Emulator& self, uint32_t address, pybind11::bytes bytes) -> bool {
            std::string bytesStr = bytes;
            return self.WriteMemory(address, std::span(reinterpret_cast<uint8_t*>(bytesStr.data()), bytesStr.size()));
        }, pybind11::arg("address"), pybind11::arg("data"))
        .def("add_breakpoint", &emu::Emulator::AddBreakpoint, pybind11::arg("address"), pybind11::arg("size"), pybind11::arg("callback"))
        .def("remove_breakpoint", &emu::Emulator::RemoveBreakpoint, pybind11::arg("handle"));

    m.def("get_emulator", []() -> emu::Emulator* {
        return emu::Application::Get()->GetEmulator();
    }, pybind11::return_value_policy::reference);

    m.def("on_start", [](emu::PythonManager::StartCallbackFn fn) {
        emu::PythonManager::RegisterStartCallback(std::move(fn));
    });

    m.def("on_fuzzing_start", [](std::function<void(emu::Emulator*, pybind11::bytes)> fn) {
        emu::PythonManager::SetFuzzingStartCallback([fn](emu::Emulator* emulator, std::span<const uint8_t> input) {
            pybind11::gil_scoped_acquire g{};

            fn(emulator, pybind11::bytes(reinterpret_cast<const char*>(input.data()), input.size()));
        });
    });

    using ConnectorPythonCallbackFn = std::function<uint64_t(uint64_t, uint64_t, pybind11::bytes)>;
    pybind11::class_<emu::Connector, pybind11::smart_holder>(m, "Connector")
        .def(pybind11::init([](const std::string& id, const std::string& type, ConnectorPythonCallbackFn callback) {
            auto connector = std::make_shared<emu::Connector>(id, type);
            connector->SetCallback([callback](uint64_t arg0, uint64_t arg1, std::span<const uint8_t> dataArg) -> uint64_t {
                pybind11::gil_scoped_acquire g{};

                return callback(arg0, arg1, dataArg.data() ? pybind11::bytes(reinterpret_cast<const char*>(dataArg.data()), dataArg.size()) : pybind11::bytes());
            });
            return connector;
        }), pybind11::arg("id"), pybind11::arg("type"), pybind11::arg("callback") = ConnectorPythonCallbackFn())
        .def("set_callback", [](emu::Connector& self, std::function<uint64_t(uint64_t, uint64_t, pybind11::bytes)> cb) {
            self.SetCallback([cb](uint64_t arg0, uint64_t arg1, std::span<const uint8_t> dataArg) -> uint64_t {
                pybind11::gil_scoped_acquire g{};

                return cb(arg0, arg1, dataArg.data() ? pybind11::bytes(reinterpret_cast<const char*>(dataArg.data()), dataArg.size()) : pybind11::bytes());
            });
        })
        .def("call_callback", [](emu::Connector& self, uint64_t arg0, uint64_t arg1, pybind11::bytes bytes) {
            std::string bytesStr = bytes;
            return self(arg0, arg1, std::span{reinterpret_cast<const uint8_t*>(bytesStr.data()), bytesStr.size()});
        });

    pybind11::class_<emu::IConnectable, pybind11::smart_holder>(m, "IConnectable")
        .def("register_input_connector", [](emu::IConnectable& self, std::shared_ptr<emu::Connector> connector) {
            self.GetInputConnectors().Add(connector);
        })
        .def("register_output_connector", [](emu::IConnectable& self, std::shared_ptr<emu::Connector> connector) {
            self.GetOutputConnectors().Add(connector);
        });

    pybind11::class_<emu::Peripheral, emu::IConnectable, emu::PyPeripheral, pybind11::smart_holder>(m, "Peripheral")
        .def(pybind11::init<>())
        .def("init", &emu::Peripheral::Init)
        .def("update", &emu::Peripheral::Update)
        .def("read", &emu::Peripheral::Read, pybind11::arg("offset"), pybind11::arg("size"))
        .def("write", &emu::Peripheral::Write, pybind11::arg("offset"), pybind11::arg("size"), pybind11::arg("value"));

    pybind11::class_<emu::Device, emu::IConnectable, emu::PyDevice, pybind11::smart_holder>(m, "Device")
        .def(pybind11::init<>())
        .def("init", &emu::Device::Init)
        .def("draw_node", &emu::Device::DrawNode);

    pybind11::class_<emu::Window, emu::PyWindow, pybind11::smart_holder>(m, "Window")
        .def(pybind11::init<const std::string &>())
        .def("init", &emu::Window::Init)
        .def("draw", &emu::Window::Draw);

    m.def("register_peripheral", [](std::string id, std::function<std::shared_ptr<emu::Peripheral>()> fn) {
        emu::Registry::impl::RegisterPeripheralFactory(id, fn);
    }, pybind11::arg("id"), pybind11::arg("factory_fn"));

    m.def("register_device", [](std::string id, std::function<std::shared_ptr<emu::Device>(pybind11::dict)> fn) {
        emu::Registry::impl::RegisterDeviceFactory(id, [fn](std::optional<std::reference_wrapper<const YAML::Node>> config) -> std::shared_ptr<emu::Device> {
            pybind11::gil_scoped_acquire g{};

            return fn(config ? ConvertFromYAML(*config) : pybind11::none());
        });
    }, pybind11::arg("id"), pybind11::arg("factory_fn"));

    m.def("register_window", [](std::shared_ptr<emu::Window> window) {
        emu::Registry::impl::RegisterWindow(std::move(window));
    }, pybind11::arg("window"));
}

namespace emu
{

void PythonManager::Init()
{
    pybind11::initialize_interpreter();
    // Workaround for some GIL issues with pybind11
    // See: https://github.com/pybind/pybind11/discussions/5507
    sThreadState = PyEval_SaveThread();
}

void PythonManager::Deinit()
{
    // TODO objects created in python might outlive the PythonManager, this is not great
    // PyEval_RestoreThread(mThreadState);
    // pybind11::finalize_interpreter();
}


bool PythonManager::LoadPlugins(const std::string& path)
{
    pybind11::gil_scoped_acquire g{};

    for (const auto& file : std::filesystem::directory_iterator(path)) {
        if (file.path().extension() == ".py") {
            EMU_LOG_DEBUG("Loading script: {}", file.path().filename().string());

            pybind11::eval_file(file.path().string());
        }
    }

    return true;
}

void PythonManager::StartPlugins(Emulator* emulator)
{
    for (auto& cb : sStartCallbacks) {
        cb(emulator);
    }
}

void PythonManager::StartFuzzing(Emulator* emulator, std::span<const uint8_t> input)
{
    if (sFuzzingStartCallback) {
        sFuzzingStartCallback(emulator, std::move(input));
    } else {
        EMU_LOG_WARN("Starting fuzzing with no fuzzing callback registered.\n"
                     "To place inputs call emu.on_fuzzing_start to place fuzzing inputs");
    }
}

bool PythonManager::GenerateStub(const std::string& module, const std::string& outputPath)
{
    pybind11::gil_scoped_acquire g{};

    pybind11::exec(std::format(R"(
import pybind11_stubgen

argv = [
    "--exit-code",
    # "--ignore-invalid-expressions=<.*>",
    "--root-suffix=",
    "-o",
    "{}", # output path
    "{}", # module name
]

pybind11_stubgen.main(argv)
    )", outputPath, module));

    return true;
}

void PythonManager::RegisterStartCallback(StartCallbackFn fn)
{
    sStartCallbacks.push_back(std::move(fn));
}

void PythonManager::SetFuzzingStartCallback(FuzzingStartCallbackFn fn)
{
    sFuzzingStartCallback = std::move(fn);   
}

} // namespace emu
