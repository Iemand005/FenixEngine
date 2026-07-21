cmake_minimum_required(VERSION 3.15)
project(Engine LANGUAGES CXX C)

option(FE_EXCLUDE_GLFW "Exclude GLFW from the build" OFF)
option(FE_EXCLUDE_JOLT "Exclude JOLT from the build" OFF)
option(FE_INCLUDE_OPENVR "Include OpenVR in the build" OFF)

# set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(KISSFFT_TEST OFF CACHE BOOL "Disable KissFFT tests" FORCE)
set(KISSFFT_TOOLS OFF CACHE BOOL "Disable KissFFT tests" FORCE)  

set(KISSFFT_STATIC ON CACHE BOOL "Build static library" FORCE)

set(EXTERNAL_DIR "${EXTERNAL_DIR}")

# set(GLFW_LIBRARY_TYPE "DYNAMIC" cache STRING "" FORCE)
set(GLFW_LIBRARY_TYPE "STATIC" CACHE STRING "" FORCE)

find_package(Vulkan REQUIRED)

# if(NOT FE_EXCLUDE_GLFW)
	add_subdirectory(${EXTERNAL_DIR}/glfw ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/glfw)
# endif()
if (NOT FE_EXCLUDE_JOLT)
	set(JPH_DEBUG_RENDERER ON CACHE BOOL "Enable debug renderer" FORCE)
	set(CPP_RTTI_ENABLED ON CACHE BOOL "Enable C++ RTTI" FORCE)
	if(MSVC)
		# Disable precompiled headers for external Jolt to avoid /FI empty-argument issues
		set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON CACHE BOOL "Disable precompiled headers for subprojects" FORCE)
	endif()
	add_subdirectory(${EXTERNAL_DIR}/JoltPhysics/Build ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/jolt)
else()
	add_compile_definitions(EXCLUDE_JOLT)
endif()
add_subdirectory(${EXTERNAL_DIR}/SDL ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sdl)
add_subdirectory(${EXTERNAL_DIR}/OpenXR-SDK ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/openxr)
add_subdirectory(${EXTERNAL_DIR}/kissfft ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kissfft)

if (FE_INCLUDE_OPENVR)
	add_subdirectory(${EXTERNAL_DIR}/openvr ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/openvr)
	add_compile_definitions(FE_INCLUDE_OPENVR)
endif()

# Match Qt's /MD(/MDd) runtime to avoid LNK2038 with Jolt
if(MSVC AND TARGET Jolt)
set_property(TARGET Jolt PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/*.c
)


add_library(Renderer STATIC
	glad/glad.c
	Graphics/Renderer.cpp
	window/SDLWindow.cpp
	window/GLFW3Window.cpp
	Graphics/VulkanRenderDevice.cpp
	ModelLoader.cpp
)

target_include_directories(Renderer PRIVATE
	../../include
)

target_include_directories(Renderer PUBLIC
	${EXTERNAL_DIR}/glm
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

	Vulkan::Vulkan
)

if(MSVC)
	target_compile_options(Renderer PRIVATE $<$<CONFIG:Debug>:/Zi>)

	target_link_options(Renderer PRIVATE $<$<CONFIG:Debug>:/DEBUG>)
else()
	target_compile_options(Renderer PRIVATE -g)
endif()

set(IMGUI_DIR ${ENGINE_DIR}/external/imgui)

file(GLOB_RECURSE IMGUI_SOURCE_FILES
	${IMGUI_DIR}/imgui.cpp
	${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
	${IMGUI_DIR}/backends/imgui_impl_sdlgpu3.cpp
	${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
	${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
	${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
	${IMGUI_DIR}/imgui_tables.cpp
	${IMGUI_DIR}/imgui_widgets.cpp
	${IMGUI_DIR}/imgui_demo.cpp
	${IMGUI_DIR}/imgui_draw.cpp
)

set(ENGINE_SOURCE_FILES ${SOURCE_FILES} ${IMGUI_SOURCE_FILES})
if(FE_INCLUDE_OPENVR)
	list(APPEND ENGINE_SOURCE_FILES openvr/OpenVR.cpp)
endif()

add_library(Engine STATIC ${ENGINE_SOURCE_FILES})

if(WIN32)
    set_target_properties(Engine PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )

	if(MSVC)
        target_compile_options(Engine PRIVATE 
            /permissive-   # Turns off MSVC non-standard extensions
            /Zc:preprocessor # Forces modern, standard C++ preprocessor
        )
    endif()

    target_compile_definitions(Engine PRIVATE
        WINRT_LEAN_AND_MEAN
        _AMD64_
		_SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNINGS
    )
endif()


if(FE_EXCLUDE_GLFW)
	target_compile_definitions(Engine PRIVATE FE_EXCLUDE_GLFW)
endif()

target_include_directories(Engine PRIVATE
	../../include
)

target_include_directories(Engine PUBLIC
	${EXTERNAL_DIR}/glm
	${EXTERNAL_DIR}/imgui

	${EXTERNAL_DIR}/glfw/include

	${EXTERNAL_DIR}/cgltf
)

if(NOT FE_EXCLUDE_JOLT)
	target_include_directories(Engine PUBLIC
		${EXTERNAL_DIR}/JoltPhysics
	)
endif()


if(NOT FE_EXCLUDE_JOLT)
	add_compile_definitions(JPH_DEBUG_RENDERER)
endif()

if(WIN32)
	target_link_libraries(Engine PRIVATE gdi32 opengl32 shell32 user32 RuntimeObject windowsapp)
elseif(APPLE)
	target_link_libraries(Engine PRIVATE "-framework OpenGL")
else()
	find_package(OpenGL REQUIRED)
	target_link_libraries(Engine PRIVATE OpenGL::GL)

	find_package(X11 REQUIRED)

	target_link_libraries(Engine PRIVATE
		X11::X11
	)

	find_package(PkgConfig REQUIRED)
	pkg_check_modules(HIDAPI REQUIRED hidapi-hidraw)

	target_link_libraries(Engine INTERFACE ${HIDAPI_LIBRARIES})
	target_include_directories(Engine PUBLIC ${HIDAPI_INCLUDE_DIRS})

endif()


target_link_libraries(Engine PUBLIC
	Renderer
	SDL3::SDL3
	OpenXR::openxr_loader
)

if(NOT FE_EXCLUDE_JOLT)
	target_link_libraries(Engine PUBLIC Jolt::Jolt)
endif()

# TODO: Make the engine only include kiss if audio visualizer engine is requested
target_link_libraries(Engine PRIVATE
	kissfft::kissfft-float
)

if(FE_INCLUDE_OPENVR)
	target_include_directories(Engine PRIVATE "${EXTERNAL_DIR}/openvr/headers")

	if(WIN32)
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			target_link_libraries(Engine PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${EXTERNAL_DIR}/openvr/lib/win64/openvr_api.lib")
			set(OPENVR_DLL "${CMAKE_CURRENT_SOURCE_DIR}/${EXTERNAL_DIR}/openvr/bin/win64/openvr_api.dll")
		else()
			target_link_libraries(Engine PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${EXTERNAL_DIR}/openvr/lib/win32/openvr_api.lib")
			set(OPENVR_DLL "${CMAKE_CURRENT_SOURCE_DIR}/${EXTERNAL_DIR}/openvr/bin/win32/openvr_api.dll")
		endif()

		add_custom_command(TARGET Engine POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${OPENVR_DLL}"
			"$<TARGET_FILE_DIR:Engine>"
		)
	endif()
endif()

if(NOT FE_EXCLUDE_GLFW)
	target_link_libraries(Engine PUBLIC glfw)
endif()


if(MSVC)
	target_compile_options(Engine PRIVATE $<$<CONFIG:Debug>:/Zi>)

	target_link_options(Engine PRIVATE $<$<CONFIG:Debug>:/DEBUG>)
else()
	target_compile_options(Engine PRIVATE -g)
endif()

target_compile_definitions(Engine PRIVATE JPH_DEBUG_RENDERER)


