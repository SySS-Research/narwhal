cmake_minimum_required(VERSION 3.21)

include(../EmuPlugin.cmake)

emu_add_plugin(
    NAME
        psoc62
    SOURCES
        source/main.cpp
        source/peripherals/BACKUP.cpp
        source/peripherals/CPUSS.cpp
        source/peripherals/SCB.cpp
        source/peripherals/SRSS.cpp
)
