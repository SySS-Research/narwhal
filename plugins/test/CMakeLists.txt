cmake_minimum_required(VERSION 3.21)

include(../EmuPlugin.cmake)

emu_add_plugin(
    NAME
        test
    SOURCES
        source/main.cpp
)
