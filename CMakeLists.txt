cmake_minimum_required(VERSION 3.15)
project(MyProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/EHsc /FS /nologo)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(/MDd /Zi)
    add_compile_definitions(_DEBUG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/debug)
else()
    add_compile_options(/MD /O2)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/release)
endif()

file(GLOB_RECURSE SOURCE_FILES
    src/extern/*.c
    src/extern/imgui/*.cpp
    src/engine/*.cpp
    src/games/Annihilation/*.cpp
)

add_executable(main ${SOURCE_FILES})

# SET WORKING DIRECTORY HERE - AFTER add_executable
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(main PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
    message(STATUS "Debug: Working directory set to project root: ${CMAKE_SOURCE_DIR}")
else()
    set_target_properties(main PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
    message(STATUS "Release: Working directory set to project root: ${CMAKE_SOURCE_DIR}")
endif()

target_include_directories(main PRIVATE
    include
    src
    src/extern
    third_party/JoltPhysics
)

if(WIN32)
    target_link_libraries(main PRIVATE
        gdi32
        glfw3
        opengl32
        shell32
        user32
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_directories(main PRIVATE lib/debug)
        target_link_options(main PRIVATE /DEBUG)
    else()
        target_link_directories(main PRIVATE lib/release)
    endif()
    
    target_link_libraries(main PRIVATE Jolt)
endif()