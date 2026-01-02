cmake_minimum_required(VERSION 3.15)
project(MyProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/EHsc /FS /nologo)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/debug)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/release)
endif()

file(GLOB_RECURSE SOURCE_FILES
    src/ext/*.c
    src/ext/imgui/*.cpp
    src/engine/*.cpp
    src/games/Annihilation/*.cpp
)

add_executable(main ${SOURCE_FILES})

target_include_directories(main PRIVATE
    include
    src
    src/ext
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
    
    target_link_directories(main PRIVATE
        lib/debug
    )
    
    target_link_libraries(main PRIVATE Jolt)
endif()