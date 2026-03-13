cmake_minimum_required(VERSION 3.21)

project(narwhal LANGUAGES C CXX)

# enable asan
# needs to be disabled when fuzzing
# add_compile_options(-fsanitize=address)
# add_link_options(-fsanitize=address)
# add_compile_options(-fsanitize=thread)
# add_link_options(-fsanitize=thread)

# set(UNICORN_LOGGING ON)
# set(UNICORN_TRACER ON)
set(UNICORN_ARCH "arm" CACHE STRING "" FORCE)
add_subdirectory(external/unicorn)

add_subdirectory(external/argparse)

add_subdirectory(external/ELFIO)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CAPSTONE REQUIRED capstone)

set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 REQUIRED)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)

find_package(yaml-cpp REQUIRED)

add_subdirectory(plugins)

add_executable(narwhal
    "source/Application.cpp"
    "source/main.cpp"

    "source/afl.cpp"
    "source/capstone.cpp"
    "source/common.cpp"
    "source/Configuration.cpp"
    "source/Connector.cpp"
    "source/CoverageCollector.cpp"
    "source/Device.cpp"
    "source/Emulator.cpp"
    "source/FileLoader.cpp"
    "source/GDBStub.cpp"
    "source/Peripheral.cpp"
    "source/Registry.cpp"
    "source/UnicornEngine.cpp"
    "source/UserInterface.cpp"
    "source/Window.cpp"

    "source/devices/EMMC.cpp"
    "source/devices/SerialConsole.cpp"
    "source/devices/ST7789.cpp"
    "source/devices/devices.cpp"

    "source/peripherals/arm/NVIC.cpp"
    "source/peripherals/arm/SCB.cpp"
    "source/peripherals/arm/SysTick.cpp"

    "source/plugins/Plugin.cpp"
    "source/plugins/PluginManager.cpp"
    "source/plugins/PythonManager.cpp"
    "source/plugins/python/PyDevice.cpp"
    "source/plugins/python/PyPeripheral.cpp"
    "source/plugins/python/PyWindow.cpp"

    "source/windows/DisassemblyWindow.cpp"
    "source/windows/EmulationStateWindow.cpp"
    "source/windows/NodeEditorWindow.cpp"
    "source/windows/SerialConsoleWindow.cpp"
    "source/windows/ST7789Window.cpp"
    "source/windows/windows.cpp"

    "external/imgui/imgui_demo.cpp"
    "external/imgui/imgui_draw.cpp"
    "external/imgui/imgui_stacklayout.cpp"
    "external/imgui/imgui_tables.cpp"
    "external/imgui/imgui_widgets.cpp"
    "external/imgui/imgui.cpp"
    "external/imgui/backends/imgui_impl_sdl3.cpp"
    "external/imgui/backends/imgui_impl_sdlrenderer3.cpp"
    "external/imgui/misc/cpp/imgui_stdlib.cpp"
    "external/imgui_bundle/pybind_imgui_internal.cpp"
    "external/imgui_bundle/pybind_imgui.cpp"
    "external/imnodes/imnodes.cpp"
)

target_link_libraries(narwhal PUBLIC
    unicorn
    ${CAPSTONE_LIBRARIES}
    elfio::elfio
    SDL3::SDL3
    yaml-cpp::yaml-cpp
    argparse
    pybind11::embed
)

target_include_directories(narwhal PUBLIC
    source
    ${ELFIO_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imnodes
)

target_compile_options(narwhal PRIVATE
    -Wall -Werror
    # TODO cannot have these as error when using imgui_bundle imgui
    -Wno-error=sign-compare
    -Wno-error=unused-variable
    -Wno-error=format-truncation
    -Wno-error=stringop-truncation
)

# Allow plugins to see all symbols
# TODO it would be nicer to have a stable API they can link against
target_link_options(narwhal
    PRIVATE -Wl,--export-dynamic
)

target_compile_features(narwhal PUBLIC
    cxx_std_23
)

target_compile_definitions(narwhal PUBLIC
    IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    IMGUI_BUNDLE_PYTHON_API
)
