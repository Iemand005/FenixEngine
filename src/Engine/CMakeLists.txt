cmake_minimum_required(VERSION 3.15)
project(Engine LANGUAGES CXX C)

# set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

set(EXTERNAL_DIR "../../external/")

# set(GLFW_LIBRARY_TYPE "DYNAMIC" cache STRING "" FORCE)
set(GLFW_LIBRARY_TYPE "STATIC" CACHE STRING "" FORCE)

add_subdirectory(../../external/glfw ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/glfw)
add_subdirectory(../../external/JoltPhysics/Build ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/jolt)
add_subdirectory(../../external/SDL ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sdl)
add_subdirectory(../../external/OpenXR-SDK ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/openxr)

file(GLOB_RECURSE SOURCE_FILES
    ../extern/*.c
    ../extern/imgui/*.cpp
    ./*.cpp
)


add_library(engine STATIC ${SOURCE_FILES})

target_include_directories(engine PRIVATE
    ../../include
)

target_link_libraries(engine PUBLIC
    gdi32
    glfw
    opengl32
    shell32
    user32
    gdi32
    Jolt
    SDL3::SDL3
    OpenXR::openxr_loader
)
