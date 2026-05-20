cmake_minimum_required(VERSION 3.21)

include(../EmuPlugin.cmake)

find_package(MbedTLS REQUIRED)

emu_add_plugin(
    NAME
        samd5x
    SOURCES
        source/main.cpp
        source/peripherals/OSCCTRL.cpp
        source/peripherals/SERCOM.cpp
        source/peripherals/TRNG.cpp
)

target_link_libraries(samd5x PRIVATE
    MbedTLS::mbedtls
)
