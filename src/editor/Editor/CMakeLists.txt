cmake_minimum_required(VERSION 3.19)
project(Editor LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets OpenGL OpenGLWidgets LinguistTools)

qt_standard_project_setup()

qt_add_executable(Editor
    WIN32 MACOSX_BUNDLE
    main.cpp
    editorwindow.cpp
    editorwindow.h
    editorwindow.ui
)

qt_add_translations(
    TARGETS Editor
    TS_FILES Editor_en_US.ts
)

target_link_libraries(Editor
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt6::OpenGL
        Qt6::OpenGLWidgets
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
