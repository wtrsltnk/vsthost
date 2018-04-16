cmake_minimum_required(VERSION 3.7)

find_package(OpenGL REQUIRED)

project(vsthost)

add_executable(minimal-vsthost
    minimal_vst2x_host.cpp
    )

target_compile_features(minimal-vsthost
    PRIVATE cxx_auto_type
    PRIVATE cxx_nullptr
    PRIVATE cxx_range_for
    )

target_link_libraries(minimal-vsthost
    ${OPENGL_LIBRARIES}
    )

target_compile_definitions(minimal-vsthost
    PRIVATE -DUNICODE
    PRIVATE -D_WIN32_WINNT=0x602
    )

add_executable(imgui-vsthost
    main.cpp
    imgui_impl_win32_gl2.cpp
    imgui_impl_win32_gl2.h
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_demo.cpp
    vstplugin.cpp
    wasapi.cpp
    glad.c
    )

target_compile_features(imgui-vsthost
    PRIVATE cxx_auto_type
    PRIVATE cxx_nullptr
    PRIVATE cxx_range_for
    )

target_link_libraries(imgui-vsthost
    ${OPENGL_LIBRARIES}
    )

target_compile_definitions(imgui-vsthost
    PRIVATE -DUNICODE
    PRIVATE -D_WIN32_WINNT=0x602
    )

target_include_directories(imgui-vsthost
    PRIVATE include
    PRIVATE "VST3 SDK"
    PRIVATE imgui
    )
