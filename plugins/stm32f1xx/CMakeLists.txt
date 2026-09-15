cmake_minimum_required(VERSION 3.21)

include(../EmuPlugin.cmake)

emu_add_plugin(
    NAME
        stm32f1xx
    SOURCES
        source/main.cpp
        source/peripherals/ADC.cpp
        source/peripherals/DMA.cpp
        source/peripherals/ETH.cpp
        source/peripherals/GPIO.cpp
        source/peripherals/TIM.cpp
        source/peripherals/USART.cpp
)
