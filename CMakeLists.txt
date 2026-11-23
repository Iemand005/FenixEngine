cmake_minimum_required(VERSION 3.15)
project(EngineRoot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MINGW OR UNIX)
    # Remove any Windows/MSVC flags that might have been added
    string(REGEX REPLACE "/[MD][d]?" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REGEX REPLACE "/Zi" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    
    # Set proper GCC flags
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()


set(JPH_USE_DX12 OFF CACHE BOOL "Use DirectX" FORCE)
set(JPH_USE_DXC OFF CACHE BOOL "Use DXC shader compiler" FORCE)
set(JPH_USE_VK OFF CACHE BOOL "Use Vulkan" FORCE)
set(JPH_USE_MTL OFF CACHE BOOL "Use Metal" FORCE)

set(USE_STATIC_MSVC_RUNTIME_LIBRARY ON CACHE BOOL "Use Static MSVC++" FORCE)
# CMAKE_MSVC_RUNTIME_LIBRARY is set by the preset or root CMakeLists.txt

set(KISSFFT_TEST OFF CACHE BOOL "Disable KissFFT tests" FORCE)

set(KISSFFT_STATIC ON CACHE BOOL "Build static library" FORCE)

# add_subdirectory(external/kissfft)

add_subdirectory(src/Engine)

if(EMSCRIPTEN OR ANDROID)
    # When building for Emscripten/Android, only build the Engine library
    message(STATUS "Cross-platform build (${CMAKE_SYSTEM_NAME}): skipping game targets")
    return()
endif()

if(MSVC)
    add_compile_options(/EHsc /FS /nologo)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

file(GLOB_RECURSE SOURCE_FILES
    # src/extern/*.c
    # src/extern/imgui/*.cpp
    # src/Engine/*.cpp
    src/games/Annihilation/*.cpp
)

set(IMGUI_DIR external/imgui)

set(IMGUI_SRC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    # ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(main ${SOURCE_FILES} ${IMGUI_SRC})

target_precompile_headers(main PRIVATE
	"$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/pch.hpp>"
)

set_property(TARGET main PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# For VSCode CMake Tools specifically
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(LIB_DIR "${CMAKE_SOURCE_DIR}/lib/debug")
else()
    set(LIB_DIR "${CMAKE_SOURCE_DIR}/lib/release")
endif()

target_compile_definitions(main PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
)

target_include_directories(main PRIVATE
    include
    src
    external
    external/imgui
    external/SDL/include
    src/Engine
)

if(FALSE)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
target_include_directories(AudioTest PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
target_link_libraries(AudioTest PRIVATE ${PIPEWIRE_LIBRARIES})
target_compile_options(AudioTest PRIVATE ${PIPEWIRE_CFLAGS_OTHER})
endif()

set_property(TARGET AudioTest PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    
if(MSVC)
	target_link_options(main PRIVATE $<$<CONFIG:Debug>:/DEBUG>)

	target_link_options(AudioTest PRIVATE $<$<CONFIG:Debug>:/DEBUG>)
endif()
    
# Main game
target_link_directories(main PRIVATE "${LIB_DIR}")

target_link_libraries(main PRIVATE Engine)


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(TARGET main POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Debug executable built at: $<TARGET_FILE:main>"
        COMMAND ${CMAKE_COMMAND} -E echo "To debug in VSCode, set cwd to: ${CMAKE_SOURCE_DIR}"
    )
endif()




message(STATUS "Project source directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "Build output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CMAKE_BUILD_TYPE}}")

# Dear ImGui is a vendored third-party library. Its .cpp files define
# IMGUI_DEFINE_MATH_OPERATORS immediately before including imgui.h, which
# imgui_internal.h hard-errors on unless that define came first. A unity blob
# that pulls a game/engine file including imgui.h ahead of them breaks every
# ImGui file after it, so keep each ImGui file in its own translation unit.
set_source_files_properties(${IMGUI_SOURCE_FILES} PROPERTIES
	SKIP_UNITY_BUILD_INCLUSION ON
)