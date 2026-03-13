cmake_minimum_required(VERSION 3.21)

include(../EmuPlugin.cmake)

find_package(MbedTLS REQUIRED)

emu_add_plugin(
    NAME
        stm32h7xx
    SOURCES
        "source/main.cpp"
        "source/peripherals/ADC.cpp"
        "source/peripherals/CRYP.cpp"
        "source/peripherals/FLASH.cpp"
        "source/peripherals/FMC.cpp"
        "source/peripherals/GPIO.cpp"
        "source/peripherals/MDMA.cpp"
        "source/peripherals/OTG.cpp"
        "source/peripherals/PWR.cpp"
        "source/peripherals/RCC.cpp"
        "source/peripherals/SDMMC.cpp"
        "source/peripherals/USART.cpp"
)

target_link_libraries(stm32h7xx PRIVATE
    MbedTLS::mbedtls
)
