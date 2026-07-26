cmake_minimum_required(VERSION 3.15)
project(Engine LANGUAGES CXX C)

option(FE_EXCLUDE_GLFW "Exclude GLFW from the build" OFF)
option(FE_EXCLUDE_JOLT "Exclude JOLT from the build" OFF)
option(FE_EXCLUDE_VULKAN "Exclude Vulkan from the build" OFF)
option(FE_EXCLUDE_OPENXR "Exclude OpenXR from the build" OFF)
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

set(SDL_WASAPI OFF CACHE BOOL "" FORCE)
set(SDL_GAMEINPUT OFF CACHE BOOL "" FORCE)
set(SDL_JOYSTICK ON CACHE BOOL "" FORCE)

# set(EXTERNAL_DIR "${EXTERNAL_DIR}")
set(EXTERNAL_DIR "../../external")

# set(GLFW_LIBRARY_TYPE "DYNAMIC" cache STRING "" FORCE)
set(GLFW_LIBRARY_TYPE "STATIC" CACHE STRING "" FORCE)

add_definitions(-D_ENABLE_EXTENDED_ALIGNED_STORAGE)

if (NOT FE_EXCLUDE_VULKAN)
    find_package(Vulkan REQUIRED)
    add_definitions(-DFE_HAS_VULKAN)
else()
    add_definitions(-DFE_EXCLUDE_VULKAN)
endif()

if (NOT FE_EXCLUDE_GLFW AND NOT EMSCRIPTEN)
	add_subdirectory(${EXTERNAL_DIR}/glfw ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/glfw)
endif()
if (NOT FE_EXCLUDE_JOLT AND NOT EMSCRIPTEN)
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
if (NOT EMSCRIPTEN AND NOT FE_EXCLUDE_OPENXR)
	add_subdirectory(${EXTERNAL_DIR}/OpenXR-SDK ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/openxr)
	add_subdirectory(${EXTERNAL_DIR}/kissfft ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kissfft)
endif()

if (FE_INCLUDE_OPENVR)
	add_subdirectory(${EXTERNAL_DIR}/openvr ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/openvr)
	add_compile_definitions(FE_INCLUDE_OPENVR)
endif()

# Match static MSVC runtime to avoid LNK2038 across all subprojects
if(MSVC)
    if(TARGET Jolt)
        set_property(TARGET Jolt PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
    if(TARGET openxr_loader AND NOT FE_EXCLUDE_OPENXR)
        set_property(TARGET openxr_loader PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/*.c
)


set(RENDERER_SOURCES
	glad/glad.c
	Renderer.cpp
	window/SDLWindow.cpp
	window/GLFW3Window.cpp
	Graphics/VulkanRenderDevice.cpp
	ModelLoader.cpp
)

if(FE_EXCLUDE_GLFW OR EMSCRIPTEN)
	list(REMOVE_ITEM RENDERER_SOURCES window/GLFW3Window.cpp)
endif()

if(FE_EXCLUDE_VULKAN OR EMSCRIPTEN)
	list(REMOVE_ITEM RENDERER_SOURCES Graphics/VulkanRenderDevice.cpp)
endif()

add_library(Renderer STATIC ${RENDERER_SOURCES})

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

	if(NOT EMSCRIPTEN)
		find_package(X11 REQUIRED)

		target_link_libraries(Renderer PRIVATE
			X11::X11
		)
	endif()
endif()


target_link_libraries(Renderer PUBLIC
	SDL3::SDL3
)

if(NOT FE_EXCLUDE_GLFW AND NOT EMSCRIPTEN)
	target_link_libraries(Renderer PUBLIC glfw)
endif()

if(NOT FE_EXCLUDE_VULKAN)
	target_link_libraries(Renderer PUBLIC Vulkan::Vulkan)
endif()

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
	${IMGUI_DIR}/imgui_tables.cpp
	${IMGUI_DIR}/imgui_widgets.cpp
	${IMGUI_DIR}/imgui_demo.cpp
	${IMGUI_DIR}/imgui_draw.cpp
)

if(NOT FE_EXCLUDE_VULKAN)
	list(APPEND IMGUI_SOURCE_FILES ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp)
endif()

set(ENGINE_SOURCE_FILES ${SOURCE_FILES} ${IMGUI_SOURCE_FILES})

# Exclude incompatible sources for Emscripten builds
if(EMSCRIPTEN)
	list(REMOVE_ITEM ENGINE_SOURCE_FILES
		${CMAKE_CURRENT_SOURCE_DIR}/XRGame.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/Graphics/VulkanRenderDevice.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/window/GLFW3Window.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/physics/PhysicsFactory.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/physics/PhysicsVehicle.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/physics/BasicDebugRenderer.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/Sensors/Accelerometer.cpp
	)
endif()

if(FE_EXCLUDE_VULKAN)
	list(REMOVE_ITEM ENGINE_SOURCE_FILES
		${CMAKE_CURRENT_SOURCE_DIR}/Graphics/VulkanRenderDevice.cpp
	)
endif()

if(FE_EXCLUDE_GLFW)
	list(REMOVE_ITEM ENGINE_SOURCE_FILES
		${CMAKE_CURRENT_SOURCE_DIR}/window/GLFW3Window.cpp
	)
endif()

if(FE_EXCLUDE_JOLT)
	list(REMOVE_ITEM ENGINE_SOURCE_FILES
		${CMAKE_CURRENT_SOURCE_DIR}/physics/PhysicsFactory.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/physics/PhysicsFactory.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/physics/PhysicsVehicle.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/physics/BasicDebugRenderer.cpp
	)
endif()

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
	add_compile_definitions(FE_EXCLUDE_GLFW)
endif()

if(FE_EXCLUDE_VULKAN)
	add_compile_definitions(FE_EXCLUDE_VULKAN)
endif()

target_include_directories(Engine PRIVATE
	../../include
	${EXTERNAL_DIR}/cgltf
)

target_include_directories(Engine PUBLIC
	${EXTERNAL_DIR}/glm
	${EXTERNAL_DIR}/imgui
)

if(NOT FE_EXCLUDE_GLFW AND NOT EMSCRIPTEN)
	target_include_directories(Engine PUBLIC
		${EXTERNAL_DIR}/glfw/include
	)
endif()

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

	if(NOT EMSCRIPTEN)
		find_package(X11 REQUIRED)

		target_link_libraries(Engine PRIVATE
			X11::X11
		)

		find_package(PkgConfig REQUIRED)
		pkg_check_modules(HIDAPI REQUIRED hidapi-hidraw)

		target_link_libraries(Engine INTERFACE ${HIDAPI_LIBRARIES})
		target_include_directories(Engine PUBLIC ${HIDAPI_INCLUDE_DIRS})
	endif()
endif()


target_link_libraries(Engine PUBLIC
	Renderer
	SDL3::SDL3
)

if(NOT FE_EXCLUDE_OPENXR AND NOT EMSCRIPTEN)
	target_link_libraries(Engine PUBLIC OpenXR::openxr_loader)
endif()

if(NOT FE_EXCLUDE_JOLT AND NOT EMSCRIPTEN)
	target_link_libraries(Engine PUBLIC Jolt::Jolt)
endif()

# kissfft only if not Emscripten (not built for WebAssembly)
if(NOT EMSCRIPTEN)
	target_link_libraries(Engine PRIVATE
		kissfft::kissfft-float
	)
endif()

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


