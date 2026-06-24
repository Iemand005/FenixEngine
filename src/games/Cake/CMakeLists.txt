cmake_minimum_required(VERSION 3.15)
project(Cake)

add_subdirectory(${CMAKE_SOURCE_DIR}/../../Engine ${CMAKE_BINARY_DIR}/out/Engine)

set(IMGUI_SRC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(Cake
    ${IMGUI_SOURCE_FILES}
    main.cpp
)

target_include_directories(Cake PRIVATE
    include
    ../..
    ../../../include
    ../../../external
    ../../../external/imgui
    ../../../external/kissfft
    ../../../external/SDL/include
    ../../Engine
)

target_link_libraries(Cake PRIVATE
    Engine
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