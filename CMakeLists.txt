cmake_minimum_required(VERSION 3.15)
project(MyProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set this early - affects where executables go
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/release")

if(MSVC)
    add_compile_options(/EHsc /FS /nologo)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

file(GLOB_RECURSE SOURCE_FILES
    src/extern/*.c
    src/extern/imgui/*.cpp
    src/engine/*.cpp
    src/games/Annihilation/*.cpp
)

add_executable(main ${SOURCE_FILES})

# For Visual Studio (if you ever open the project in VS)
set_target_properties(main PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# For VSCode CMake Tools specifically
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(LIB_DIR "${CMAKE_SOURCE_DIR}/lib/debug")
else()
    set(LIB_DIR "${CMAKE_SOURCE_DIR}/lib/release")
endif()

# Compile options per configuration
target_compile_options(main PRIVATE
    $<$<CONFIG:Debug>:/MDd /Zi>
    $<$<CONFIG:Release>:/MD /O2>
)

target_compile_definitions(main PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
)

target_include_directories(main PRIVATE
    include
    src
    src/extern
    third_party/JoltPhysics
)

if(WIN32)
    target_link_directories(main PRIVATE "${LIB_DIR}")
    
    target_link_libraries(main PRIVATE
        gdi32
        glfw3
        opengl32
        shell32
        user32
        Jolt
    )
    
    target_link_options(main PRIVATE
        $<$<CONFIG:Debug>:/DEBUG>
    )
endif()

# Create a post-build script to help with debugging
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Copy any needed files to build directory
    add_custom_command(TARGET main POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Debug executable built at: $<TARGET_FILE:main>"
        COMMAND ${CMAKE_COMMAND} -E echo "To debug in VSCode, set cwd to: ${CMAKE_SOURCE_DIR}"
    )
endif()

message(STATUS "Project source directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "Build output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CMAKE_BUILD_TYPE}}")