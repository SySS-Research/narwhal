# This is based on ImHex's plugin SDK

macro(emu_add_plugin)
    # Parse arguments
    set(options )
    set(oneValueArgs NAME)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(EMU_PLUGIN
        "${options}" "${oneValueArgs}" "${multiValueArgs}"
        ${ARGN}
    )

    # Create project
    project(${EMU_PLUGIN_NAME} LANGUAGES C CXX)

    # Create library
    add_library(${EMU_PLUGIN_NAME} MODULE
        ${EMU_PLUGIN_SOURCES}
    )

    # Use C++23
    target_compile_features(${EMU_PLUGIN_NAME}
        PUBLIC cxx_std_23
    )

    # Allow plugin to include emu files
    # TODO shared includes
    target_include_directories(${EMU_PLUGIN_NAME}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/source
            ${CMAKE_SOURCE_DIR}/external/unicorn/include
    )

    # Output under plugins/ without lib prefix
    set_target_properties(${EMU_PLUGIN_NAME}
        PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
        PREFIX ""
    )
endmacro()
