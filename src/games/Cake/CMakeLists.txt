cmake_minimum_required(VERSION 3.15)
project(Cake)

add_subdirectory(${CMAKE_SOURCE_DIR}/src/Engine)
add_subdirectory(../../src/Engine)


add_executable(Cake
    ${IMGUI_SOURCE_FILES}
    main.cpp
)

target_include_directories(Cake PRIVATE
    include
    ../../src
    ../../external
    ../../external/imgui
    ../../external/SDL/include
    ../../src/Engine
)

target_link_libraries(Cake PRIVATE
    Engine
    kissfft::kissfft-float
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
target_include_directories(Cake PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
target_link_libraries(Cake PRIVATE ${PIPEWIRE_LIBRARIES})
target_compile_options(Cake PRIVATE ${PIPEWIRE_CFLAGS_OTHER})

set_property(TARGET Cake PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    
if(MSVC)
	target_compile_options(Cake PRIVATE $<$<CONFIG:Debug>:/Zi>)

	target_link_options(Cake PRIVATE $<$<CONFIG:Debug>:/DEBUG>)
endif()