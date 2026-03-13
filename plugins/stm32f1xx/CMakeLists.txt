cmake_minimum_required(VERSION 3.21)

include(../EmuPlugin.cmake)

emu_add_plugin(
    NAME
        stm32f1xx
    SOURCES
        source/main.cpp
        source/peripherals/DMA.cpp
        source/peripherals/USART.cpp
)
