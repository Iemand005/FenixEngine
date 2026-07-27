cmake_minimum_required(VERSION 3.15)
project(Win7Compat C)

# Build kernel32_win7.lib — a custom import library that maps all kernel32 symbols
# directly to kernel32.dll (no api-ms-win-* forwarding). We prepend it to the link
# path so the linker resolves symbols here first, before hitting the Windows 10 SDK's
# kernel32.lib which forwards through api-ms-win-core-libraryloader-l1-2-0.dll.

if (EMSCRIPTEN)
    return()
endif()

# Find lib.exe (MSVC's library manager)
find_program(LIB_EXE lib.exe
    HINTS
        "$ENV{VCToolsInstallDir}/bin/Hostx64/x64"
        "$ENV{VCToolsInstallDir}/bin/HostX64/x64"
        "C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.16.27023/bin/Hostx64/x64"
    DOC "MSVC lib.exe for building import libraries"
)

if(NOT LIB_EXE)
    message(FATAL_ERROR "lib.exe not found — required to build kernel32_win7.lib")
endif()

# Build the import library
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/kernel32_win7.lib"
    COMMAND ${LIB_EXE}
        /DEF:"${CMAKE_CURRENT_SOURCE_DIR}/kernel32_win7.def"
        /OUT:"${CMAKE_CURRENT_BINARY_DIR}/kernel32_win7.lib"
        /MACHINE:X64
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Building kernel32_win7.lib import library (kernel32.dll direct imports)"
)

add_custom_target(kernel32_win7_lib ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/kernel32_win7.lib")

# Also build the api-ms-win-core-libraryloader-l1-2-0.dll forwarding shim
# as a fallback for any code that explicitly imports from that DLL name.
add_library(api-ms-win-core-libraryloader-l1-2-0 SHARED
    dummy.c
    api-ms-win-core-libraryloader-l1-2-0.def
)

set_target_properties(api-ms-win-core-libraryloader-l1-2-0 PROPERTIES
    PREFIX ""
    SUFFIX ".dll"
    IMPORT_PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Make the shim DLL available globally by adding its output dir to linker search path
# for targets that link it explicitly (or we can just copy it to bin/)
add_custom_command(TARGET api-ms-win-core-libraryloader-l1-2-0 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:api-ms-win-core-libraryloader-l1-2-0>
        "${CMAKE_BINARY_DIR}/bin/$<TARGET_FILE_NAME:api-ms-win-core-libraryloader-l1-2-0>"
    COMMENT "Copying api-ms-win-core-libraryloader-l1-2-0.dll shim to bin/"
)