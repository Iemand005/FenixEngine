cmake_minimum_required(VERSION 3.15)
project(Cake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KISSFFT_TEST OFF CACHE BOOL "Disable KissFFT tests" FORCE)

set(KISSFFT_STATIC ON CACHE BOOL "Build static library" FORCE)

add_subdirectory(${CMAKE_SOURCE_DIR}/../../Engine ${CMAKE_BINARY_DIR}/out/Engine)

set(IMGUI_DIR ../../../external/imgui)

file(GLOB_RECURSE IMGUI_SOURCE_FILES
	${IMGUI_DIR}/imgui.cpp
	${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
	${IMGUI_DIR}/backends/imgui_impl_sdlgpu3.cpp
	${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
	${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
	${IMGUI_DIR}/imgui_tables.cpp
	${IMGUI_DIR}/imgui_widgets.cpp
	${IMGUI_DIR}/imgui_demo.cpp
	${IMGUI_DIR}/imgui_draw.cpp
)

add_executable(Cake
	WIN32
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

if(NOT WIN32)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
target_include_directories(Cake PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
target_link_libraries(Cake PRIVATE ${PIPEWIRE_LIBRARIES})
target_compile_options(Cake PRIVATE ${PIPEWIRE_CFLAGS_OTHER})

find_package(X11 REQUIRED)

target_link_libraries(Cake PRIVATE X11::X11)
endif()

set_property(TARGET Cake PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
	
if(MSVC)
	target_compile_options(Cake PRIVATE $<$<CONFIG:Debug>:/Zi>)

	target_link_options(Cake PRIVATE $<$<CONFIG:Debug>:/DEBUG>)

	add_compile_options(/RTC1)
endif()

# set_target_properties(Cake PROPERTIES SUFFIX ".scr")
