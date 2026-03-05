cmake_minimum_required(VERSION 3.15)
project(Engine LANGUAGES CXX C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

set(EXTERNAL_DIR "../../external/")

add_subdirectory(../../external/glfw ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/glfw)
add_subdirectory(../../external/JoltPhysics/Build ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/jolt)
add_subdirectory(../../external/SDL ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sdl)
add_subdirectory(../../external/OpenXR-SDK ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/openxr)

file(GLOB_RECURSE SOURCE_FILES
    src/extern/*.c
    src/extern/imgui/*.cpp
    src/engine/*.cpp
)


add_library(engine ${SOURCE_FILES})