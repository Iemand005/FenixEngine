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

# Match Qt's /MD(/MDd) runtime to avoid LNK2038 with Jolt
if(MSVC AND TARGET Jolt)
  set_property(TARGET Jolt PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

file(GLOB_RECURSE SOURCE_FILES
    glad/*.c
    *.cpp
)


add_library(Engine STATIC ${SOURCE_FILES})

target_include_directories(Engine PRIVATE
    ../../include
)

target_include_directories(Engine PUBLIC
    ../../external/glm
    ../../external/imgui
)

target_link_libraries(Engine PRIVATE
    gdi32
    opengl32
    shell32
    user32
    gdi32
)

target_link_libraries(Engine PUBLIC
    glfw
    SDL3::SDL3
    OpenXR::openxr_loader
    Jolt
)
