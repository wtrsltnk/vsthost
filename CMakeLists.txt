cmake_minimum_required(VERSION 3.7)

find_package(OpenGL REQUIRED)

project(vsthost)

set(CMAKE_CXX_STANDARD 14)

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
    RtError.h
    RtMidi.cpp
    RtMidi.h
    common.h
    glad.c
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui_impl_win32_gl2.cpp
    imgui_impl_win32_gl2.h
    imguiutils.cpp
    imguiutils.h
    instrument.cpp
    instrument.h
    main.cpp
    midicontrollers.h
    midievent.cpp
    midievent.h
    region.cpp
    region.h
    state.cpp
    state.h
    track.cpp
    track.h
    trackseditor.cpp
    trackseditor.h
    tracksmanager.cpp
    tracksmanager.h
    vstplugin.cpp
    vstplugin.h
    wasapi.cpp
    wasapi.h
    )

target_compile_features(imgui-vsthost
    PRIVATE cxx_auto_type
    PRIVATE cxx_nullptr
    PRIVATE cxx_range_for
    )

target_link_libraries(imgui-vsthost
    ${OPENGL_LIBRARIES}
    winmm
    )

target_compile_definitions(imgui-vsthost
    PRIVATE -DUNICODE
    PRIVATE -D_WIN32_WINNT=0x602
    PRIVATE -D__WINDOWS_MM__
    )

target_include_directories(imgui-vsthost
    PRIVATE include
    PRIVATE "VST3 SDK"
    PRIVATE imgui
    )
