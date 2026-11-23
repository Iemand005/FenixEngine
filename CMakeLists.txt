cmake_minimum_required(VERSION 3.15)
project(EngineRoot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

set(USE_STATIC_MSVC_RUNTIME_LIBRARY ON CACHE BOOL "Use Static MSVC++" FORCE)
# CMAKE_MSVC_RUNTIME_LIBRARY is set by the preset or root CMakeLists.txt

set(KISSFFT_TEST OFF CACHE BOOL "Disable KissFFT tests" FORCE)

set(KISSFFT_STATIC ON CACHE BOOL "Build static library" FORCE)

# add_subdirectory(external/kissfft)

cmake_minimum_required(VERSION 3.15)
project(Engine LANGUAGES CXX C)

option(FE_EXCLUDE_GLFW "Exclude GLFW from the build" OFF)
option(FE_EXCLUDE_JOLT "Exclude JOLT from the build" OFF)
option(FE_EXCLUDE_VULKAN "Exclude Vulkan from the build" OFF)
option(FE_EXCLUDE_OPENXR "Exclude OpenXR from the build" OFF)
option(FE_EXCLUDE_KISSFFT "Exclude kissfft from the build" OFF)
option(FE_EXCLUDE_IMGUI "Exclude ImGui from EditableGame and XRGame UI code" OFF)
option(FE_INCLUDE_OPENVR "Include OpenVR in the build" OFF)

set(SDL_X11_XTEST OFF CACHE BOOL "Disable SDL X11 XTEST dependency" FORCE)

if(EMSCRIPTEN)
    message(STATUS "Emscripten build detected — forcing web-compatible feature set")

    # Normal (non-cache) variables on purpose: CACHE ... FORCE here would leak
    # into sibling projects of the meta build and survive reconfigures.
    set(FE_EXCLUDE_GLFW   ON)
    # set(FE_EXCLUDE_JOLT   ON)
    set(FE_EXCLUDE_VULKAN ON)
    set(FE_EXCLUDE_OPENXR ON)
    set(FE_EXCLUDE_KISSFFT ON)
    set(FE_INCLUDE_OPENVR OFF)

    set(SDL_WASAPI    OFF CACHE BOOL "" FORCE)
    set(SDL_GAMEINPUT OFF CACHE BOOL "" FORCE)
    set(SDL_JOYSTICK  ON  CACHE BOOL "" FORCE)
endif()

if(ANDROID)
    message(STATUS "Android build detected — forcing mobile-compatible feature set")

    # Normal (non-cache) variables: CACHE ... FORCE would leak into sibling
    # projects of the meta build and survive reconfigures.
    set(FE_EXCLUDE_GLFW   ON)
    set(FE_EXCLUDE_VULKAN ON)
    if(NOT DEFINED FE_EXCLUDE_OPENXR)
        set(FE_EXCLUDE_OPENXR OFF)
    endif()
    set(FE_EXCLUDE_KISSFFT OFF)
    set(FE_INCLUDE_OPENVR OFF)
endif()

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

# NOTE: do NOT force SDL_WASAPI / SDL_GAMEINPUT off here. That used to be
# unconditional, which disabled SDL's WASAPI driver (and audio capture) for
# every desktop build. The EMSCRIPTEN branch above turns them off for the web.

# set(EXTERNAL_DIR "${EXTERNAL_DIR}")
set(EXTERNAL_DIR "external")

# set(GLFW_LIBRARY_TYPE "DYNAMIC" cache STRING "" FORCE)
set(GLFW_LIBRARY_TYPE "STATIC" CACHE STRING "" FORCE)

add_definitions(-D_ENABLE_EXTENDED_ALIGNED_STORAGE)

if (NOT FE_EXCLUDE_VULKAN)
    find_package(Vulkan REQUIRED)
    add_definitions(-DFE_HAS_VULKAN)
else()
    add_definitions(-DFE_EXCLUDE_VULKAN)
endif()

if (EMSCRIPTEN)
    add_definitions(-DFE_EXCLUDE_OPENXR)
endif()

if (FE_EXCLUDE_KISSFFT)
    add_definitions(-DFE_EXCLUDE_KISSFFT)
endif()

if (NOT FE_EXCLUDE_GLFW AND NOT EMSCRIPTEN)
	add_subdirectory(${EXTERNAL_DIR}/glfw ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/glfw)
endif()
if (NOT FE_EXCLUDE_JOLT)
	add_compile_definitions(JPH_DEBUG_RENDERER)

	set(JPH_DEBUG_RENDERER ON CACHE BOOL "Enable debug renderer" FORCE)
	set(CPP_RTTI_ENABLED ON CACHE BOOL "Enable C++ RTTI" FORCE)
	set(DEBUG_RENDERER_IN_DEBUG_AND_RELEASE ON CACHE BOOL "Enable debug renderer in Debug and Release builds" FORCE)
	set(DEBUG_RENDERER_IN_DISTRIBUTION ON CACHE BOOL "Enable debug renderer in all builds" FORCE)
	if(MSVC)
		# Disable precompiled headers for external Jolt to avoid /FI empty-argument issues
		set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON CACHE BOOL "Disable precompiled headers for subprojects" FORCE)
	endif()
	add_subdirectory(${EXTERNAL_DIR}/JoltPhysics/Build ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/jolt)
	if(TARGET Jolt)
		target_compile_definitions(Jolt PUBLIC JPH_DEBUG_RENDERER)
	endif()
else()
	add_compile_definitions(EXCLUDE_JOLT)
endif()
add_subdirectory(${EXTERNAL_DIR}/SDL ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sdl)
if (NOT EMSCRIPTEN AND NOT FE_EXCLUDE_OPENXR)
    set(BUILD_WITH_VULKAN_SUPPORT OFF CACHE BOOL "" FORCE)
    set(BUILD_WITH_OPENGL_SUPPORT ON CACHE BOOL "" FORCE)
    set(BUILD_WITH_OPENGL_ES_SUPPORT ON CACHE BOOL "" FORCE)
	add_subdirectory(${EXTERNAL_DIR}/OpenXR-SDK ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/openxr)
endif()
if (NOT EMSCRIPTEN AND NOT FE_EXCLUDE_KISSFFT)
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

set(SOURCE_FILES
	src/accessories/Aura.cpp
	src/Android.cpp
	src/components/Drone.cpp
	src/EditableGame.cpp
	src/engine.cpp
	src/Game.cpp
	src/Graphics/OpenGLGPUBuffers.cpp
	src/Graphics/OpenGLGPUTexture.cpp
	src/Graphics/OpenGLRenderDevice.cpp
	src/ImageLoader.cpp
	src/Mesh.cpp
	src/common/Chunk.cpp
	src/common/ChunkManager.cpp
	src/physics/BasicDebugRenderer.cpp
	src/physics/PhysicsCharacter.cpp
	src/physics/PhysicsFactory.cpp
	src/physics/PhysicsObject.cpp
	src/physics/PhysicsVehicle.cpp
	src/Scene.cpp
	src/Sensors/Accelerometer.cpp
	src/vr/OpenXR.cpp
	src/XRGame.cpp
)

set_source_files_properties(src/components/Drone.cpp PROPERTIES
	SKIP_UNITY_BUILD_INCLUSION ON
)

set(RENDERER_SOURCES
	src/Renderer.cpp
	src/window/SDLWindow.cpp
	src/window/GLFW3Window.cpp
	src/Graphics/VulkanRenderDevice.cpp
	src/Graphics/VulkanDevice.cpp
	src/ModelLoader.cpp
)

if(NOT ANDROID AND NOT EMSCRIPTEN)
	list(APPEND RENDERER_SOURCES src/glad/glad.c)
endif()

if(FE_EXCLUDE_GLFW OR EMSCRIPTEN)
	list(REMOVE_ITEM RENDERER_SOURCES src/window/GLFW3Window.cpp)
endif()

if(FE_EXCLUDE_VULKAN OR EMSCRIPTEN)
	list(REMOVE_ITEM RENDERER_SOURCES src/Graphics/VulkanRenderDevice.cpp src/Graphics/VulkanDevice.cpp)
endif()

add_library(Renderer STATIC ${RENDERER_SOURCES})

if(FE_EXCLUDE_OPENXR OR EMSCRIPTEN)
	target_compile_definitions(Renderer PRIVATE FE_EXCLUDE_OPENXR)
else()
	target_include_directories(Renderer PRIVATE
		${EXTERNAL_DIR}/OpenXR-SDK/include
	)
endif()

target_precompile_headers(Renderer PRIVATE
	"$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp>"
)

target_include_directories(Renderer PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(Renderer PUBLIC
	${EXTERNAL_DIR}/glm
)


if(WIN32)
	target_link_libraries(Renderer PRIVATE gdi32 opengl32 shell32 user32)
elseif(APPLE)
	target_link_libraries(Renderer PRIVATE "-framework OpenGL")
elseif(ANDROID)
	target_link_libraries(Renderer PRIVATE log GLESv2 GLESv3 EGL)
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
	target_link_options(Renderer PRIVATE $<$<CONFIG:Debug>:/DEBUG>)
elseif(NOT EMSCRIPTEN)
	target_compile_options(Renderer PRIVATE -g)
endif()

set(IMGUI_DIR ${EXTERNAL_DIR}/imgui)

set(IMGUI_SOURCE_FILES
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

# Dear ImGui is a vendored third-party library. Its .cpp files define
# IMGUI_DEFINE_MATH_OPERATORS immediately before including imgui.h, which
# imgui_internal.h rejects unless the define came first. Unity merges files into
# one TU, so an engine file that includes imgui.h earlier in the blob breaks
# these. Keep each ImGui file in its own TU.
set_source_files_properties(${IMGUI_SOURCE_FILES} PROPERTIES
	SKIP_UNITY_BUILD_INCLUSION ON
)

if(NOT FE_EXCLUDE_VULKAN)
	list(APPEND IMGUI_SOURCE_FILES ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp)
endif()

# Same reason as above; keep the opt-out in place for the Vulkan backend too,
# which is appended after the first set_source_files_properties call.
set_source_files_properties(${IMGUI_SOURCE_FILES} PROPERTIES
	SKIP_UNITY_BUILD_INCLUSION ON
)

set(ENGINE_SOURCE_FILES ${SOURCE_FILES} ${IMGUI_SOURCE_FILES})

if(FE_EXCLUDE_JOLT)
	list(REMOVE_ITEM ENGINE_SOURCE_FILES
		src/physics/PhysicsVehicle.cpp
		src/physics/BasicDebugRenderer.cpp
	)
endif()

if(FE_INCLUDE_OPENVR)
	list(APPEND ENGINE_SOURCE_FILES src/vr/OpenVR.cpp)
endif()

add_library(Engine STATIC ${ENGINE_SOURCE_FILES})

if (FE_EXCLUDE_IMGUI)
    target_compile_definitions(Engine PRIVATE FE_EXCLUDE_IMGUI)
endif()

target_precompile_headers(Engine PRIVATE
	"$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp>"
)

if(WIN32)
    set_target_properties(Engine PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )

	if(MSVC)
        target_compile_options(Engine PRIVATE 
            /permissive-   # Turns off MSVC non-standard extensions
        )
        # Modern preprocessor needs MSVC 19.25+ (VS2019 16.5+); v141 doesn't support it
        if(MSVC_VERSION GREATER_EQUAL 1925)
            target_compile_options(Engine PRIVATE /Zc:preprocessor)
        endif()
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

if(FE_EXCLUDE_OPENXR)
	target_compile_definitions(Engine PUBLIC FE_EXCLUDE_OPENXR)
endif()

target_include_directories(Engine PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${EXTERNAL_DIR}/cgltf
)

target_include_directories(Engine PUBLIC
	${EXTERNAL_DIR}/glm
	${EXTERNAL_DIR}/imgui
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/src/common
)

if(NOT FE_EXCLUDE_OPENXR AND NOT EMSCRIPTEN)
	target_include_directories(Engine PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/${EXTERNAL_DIR}/OpenXR-SDK/include
	)
endif()

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
elseif(ANDROID)
	target_link_libraries(Engine PRIVATE android log GLESv2 GLESv3 EGL)
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

if(NOT FE_EXCLUDE_JOLT)
	target_link_libraries(Engine PUBLIC Jolt::Jolt)
endif()

if(FE_EXCLUDE_KISSFFT)
	target_compile_definitions(Engine PUBLIC FE_EXCLUDE_KISSFFT)
endif()

# kissfft only if not Emscripten and not excluded
if(NOT EMSCRIPTEN AND NOT FE_EXCLUDE_KISSFFT)
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
	target_link_options(Engine PRIVATE $<$<CONFIG:Debug>:/DEBUG>)
elseif(NOT EMSCRIPTEN)
	target_compile_options(Engine PRIVATE -g)
endif()

target_compile_definitions(Renderer PRIVATE JPH_DEBUG_RENDERER)
target_compile_definitions(Engine PRIVATE JPH_DEBUG_RENDERER)


if(EMSCRIPTEN)
    # Debug: symbols + source maps
    target_compile_options(Renderer PRIVATE
        $<$<CONFIG:Debug>:-g -fdebug-compilation-dir="${CMAKE_SOURCE_DIR}">
    )
    target_link_options(Renderer PRIVATE
        $<$<CONFIG:Debug>:-g -gsource-map>
    )
    target_compile_options(Renderer PRIVATE
        $<$<CONFIG:Release>:-Oz -flto>
    )
    target_link_options(Renderer PRIVATE
        $<$<CONFIG:Release>:-Oz -flto>
    )

    target_compile_options(Engine PRIVATE
        $<$<CONFIG:Debug>:-g -fdebug-compilation-dir="${CMAKE_SOURCE_DIR}">
    )
    target_link_options(Engine PRIVATE
        $<$<CONFIG:Debug>:-g -gsource-map>
    )
    target_compile_options(Engine PRIVATE
        $<$<CONFIG:Release>:-Oz -flto>
    )
    target_link_options(Engine PRIVATE
        $<$<CONFIG:Release>:-Oz -flto>
    )
endif()


if(EMSCRIPTEN OR ANDROID)
    # When building for Emscripten/Android, only build the Engine library
    message(STATUS "Cross-platform build (${CMAKE_SYSTEM_NAME}): skipping game targets")
    return()
endif()

if(MSVC)
    add_compile_options(/EHsc /FS /nologo)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

file(GLOB_RECURSE SOURCE_FILES
    # src/extern/*.c
    # src/extern/imgui/*.cpp
    # src/Engine/*.cpp
    src/games/Annihilation/*.cpp
)

set(IMGUI_DIR external/imgui)

set(IMGUI_SRC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    # ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)




message(STATUS "Project source directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "Build output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CMAKE_BUILD_TYPE}}")

# Dear ImGui is a vendored third-party library. Its .cpp files define
# IMGUI_DEFINE_MATH_OPERATORS immediately before including imgui.h, which
# imgui_internal.h hard-errors on unless that define came first. A unity blob
# that pulls a game/engine file including imgui.h ahead of them breaks every
# ImGui file after it, so keep each ImGui file in its own translation unit.
set_source_files_properties(${IMGUI_SOURCE_FILES} PROPERTIES
	SKIP_UNITY_BUILD_INCLUSION ON
)