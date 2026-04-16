cmake_minimum_required(VERSION 3.15)
project(editor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

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
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "Use Static MSVC++" FORCE)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

add_subdirectory(src/Engine)

if(MSVC)
    add_compile_options(/EHsc /FS /nologo)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

file(GLOB_RECURSE SOURCE_FILES
    # src/extern/*.c
    # src/extern/imgui/*.cpp
    # src/engine/*.cpp
    src/games/Annihilation/*.cpp
)

add_executable(main ${SOURCE_FILES})


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
    src/engine
)

file(GLOB_RECURSE EDITOR_SOURCE_FILES
    external/imgui/imgui.cpp
    external/imgui/backends/imgui_impl_sdl3.cpp
    external/imgui/backends/imgui_impl_sdlgpu3.cpp
    external/imgui/backends/imgui_impl_sdlrenderer3.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp

    src/games/Chizzle/*.cpp
)

add_executable(editor ${EDITOR_SOURCE_FILES})

target_include_directories(editor PRIVATE
    include
    src
    external
    src/engine
)


file(GLOB_RECURSE POOL_SOURCE_FILES
    external/imgui/imgui.cpp
    external/imgui/backends/imgui_impl_sdl3.cpp
    external/imgui/backends/imgui_impl_sdlgpu3.cpp
    external/imgui/backends/imgui_impl_sdlrenderer3.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp

    src/games/Pool/*.cpp
)

add_executable(pool ${POOL_SOURCE_FILES})

target_include_directories(pool PRIVATE
    include
    src
    external
    external/imgui
    src/engine
)

if(WIN32)
    # Main game
    target_link_directories(main PRIVATE "${LIB_DIR}")
    
    target_link_libraries(main PRIVATE
        Engine
    )
    
    if(MSVC)
        target_link_options(main PRIVATE
            $<$<CONFIG:Debug>:/DEBUG>
        )
    endif()

    # Base editor

    target_link_directories(editor PRIVATE "${LIB_DIR}")
    
    target_link_libraries(editor PRIVATE
        Engine
    )


    # Pool test game
    target_link_directories(pool PRIVATE "${LIB_DIR}")
    
    target_link_libraries(pool PRIVATE
        Engine
    )
    
endif()




if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(TARGET main POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Debug executable built at: $<TARGET_FILE:main>"
        COMMAND ${CMAKE_COMMAND} -E echo "To debug in VSCode, set cwd to: ${CMAKE_SOURCE_DIR}"
    )
endif()




message(STATUS "Project source directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "Build output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CMAKE_BUILD_TYPE}}")
