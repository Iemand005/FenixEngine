cmake_minimum_required(VERSION 3.19)
project(Editor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGL OpenGLWidgets LinguistTools Network)

qt_standard_project_setup()

qt_add_executable(Editor
    # WIN32 MACOSX_BUNDLE
    main.cpp
    editorwindow.cpp
    editorwindow.h
    editorwindow.ui
    enginewidget.h enginewidget.cpp
    mjpegserver.h mjpegserver.cpp
)

qt_add_translations(
    TARGETS Editor
    TS_FILES Editor_en_US.ts
)

add_subdirectory(../Engine build/bin)

target_include_directories(Editor PRIVATE
    ../../include
    ../Engine
)

target_link_libraries(Editor
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt6::OpenGL
        Qt6::OpenGLWidgets
        Qt6::Network

        Engine
)

include(GNUInstallDirs)

install(TARGETS Editor
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET Editor
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

set(RES_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/resources")
set(RES_DEST "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources")

add_custom_command(
    TARGET Editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${RES_SOURCE}"
        "${RES_DEST}"
    COMMENT "Resources kopiëren naar build directory..."
)
