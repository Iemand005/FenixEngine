cmake_minimum_required(VERSION 3.15)
project(Engine LANGUAGES CXX C)

option(FE_EXCLUDE_GLFW "Exclude GLFW from the build" OFF)

# set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

set(KISSFFT_TEST OFF CACHE BOOL "Disable KissFFT tests" FORCE)
set(KISSFFT_TOOLS OFF CACHE BOOL "Disable KissFFT tests" FORCE)  

set(KISSFFT_STATIC ON CACHE BOOL "Build static library" FORCE)

set(EXTERNAL_DIR "../../external/")

# set(GLFW_LIBRARY_TYPE "DYNAMIC" cache STRING "" FORCE)
set(GLFW_LIBRARY_TYPE "STATIC" CACHE STRING "" FORCE)

# if(NOT FE_EXCLUDE_GLFW)
    add_subdirectory(../../external/glfw ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/glfw)
# endif()
add_subdirectory(../../external/JoltPhysics/Build ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/jolt)
add_subdirectory(../../external/SDL ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sdl)
add_subdirectory(../../external/OpenXR-SDK ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/openxr)
add_subdirectory(../../external/kissfft ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kissfft)

# Match Qt's /MD(/MDd) runtime to avoid LNK2038 with Jolt
if(MSVC AND TARGET Jolt)
  set_property(TARGET Jolt PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
)


add_library(Engine STATIC ${SOURCE_FILES})

if(FE_EXCLUDE_GLFW)
    target_compile_definitions(Engine PRIVATE FE_EXCLUDE_GLFW)
endif()

target_include_directories(Engine PRIVATE
    ../../include
)

target_include_directories(Engine PUBLIC
    ../../external/glm
    ../../external/imgui

    ../../external/glfw/include
)

# target_link_libraries(Engine PRIVATE
#     gdi32
#     opengl32
#     shell32
#     user32
#     gdi32
# )

if(WIN32)
    target_link_libraries(Engine PRIVATE gdi32 opengl32 shell32 user32)
elseif(APPLE)
    target_link_libraries(Engine PRIVATE "-framework OpenGL")
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(Engine PRIVATE OpenGL::GL)

    find_package(X11 REQUIRED)

    target_link_libraries(Engine PRIVATE
        X11::X11
    )
endif()


target_link_libraries(Engine PUBLIC
    SDL3::SDL3
    OpenXR::openxr_loader
    Jolt
)

# TODO: Make the engine only include kiss if audio visualizer engine is requested
target_link_libraries(Engine PRIVATE
    kissfft::kissfft-float
)

if(NOT FE_EXCLUDE_GLFW)
    target_link_libraries(Engine PUBLIC glfw)
endif()


if(MSVC)
    target_compile_options(Engine PRIVATE $<$<CONFIG:Debug>:/Zi>)

    target_link_options(Engine PRIVATE $<$<CONFIG:Debug>:/DEBUG>)
else()
    target_compile_options(Engine PRIVATE -g)
endif()


add_library(Renderer STATIC
    glad/glad.c
    Renderer.cpp
    window/SDLWindow.cpp
    window/GLFW3Window.cpp
)

target_include_directories(Renderer PRIVATE
    ../../include
)

target_include_directories(Renderer PUBLIC
    ../../external/glm
)


if(WIN32)
    target_link_libraries(Renderer PRIVATE gdi32 opengl32 shell32 user32)
elseif(APPLE)
    target_link_libraries(Renderer PRIVATE "-framework OpenGL")
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(Renderer PRIVATE OpenGL::GL)

    find_package(X11 REQUIRED)

    target_link_libraries(Renderer PRIVATE
        X11::X11
    )
endif()


target_link_libraries(Renderer PUBLIC
    glfw
    SDL3::SDL3
)

if(MSVC)
    target_compile_options(Renderer PRIVATE $<$<CONFIG:Debug>:/Zi>)

    target_link_options(Renderer PRIVATE $<$<CONFIG:Debug>:/DEBUG>)
else()
    target_compile_options(Renderer PRIVATE -g)
endif()
