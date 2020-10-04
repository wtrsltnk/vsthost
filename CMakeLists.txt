cmake_minimum_required(VERSION 3.14)

find_package(OpenGL REQUIRED)

project(vsthost)

set(CMAKE_CXX_STANDARD 14)

add_library(RtMidi
    thirdparty/RtMidi/include/RtError.h
    thirdparty/RtMidi/include/RtMidi.h
    thirdparty/RtMidi/RtMidi.cpp)

target_include_directories(RtMidi
    PUBLIC thirdparty/RtMidi/include)

target_compile_definitions(RtMidi
    PRIVATE -DUNICODE
    PRIVATE -D_WIN32_WINNT=0x602
    PRIVATE -D__WINDOWS_MM__
    )

target_link_libraries(RtMidi
    winmm
    )

add_library(glad
    thirdparty/glad/glad.c)

target_include_directories(glad
    PUBLIC thirdparty/glad/include)

add_library(imgui
    thirdparty/imgui/imgui.cpp
    thirdparty/imgui/imgui_demo.cpp
    thirdparty/imgui/imgui_draw.cpp)

target_include_directories(imgui
    PUBLIC thirdparty/imgui)

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
    abstracttimelineeditor.cpp
    abstracttimelineeditor.h
    common.h
    imgui_impl_win32_gl2.cpp
    imgui_impl_win32_gl2.h
    imguiutils.cpp
    imguiutils.h
    inspectorwindow.cpp
    inspectorwindow.h
    instrument.cpp
    instrument.h
    main.cpp
    midicontrollers.h
    midievent.cpp
    midievent.h
    midinote.cpp
    midinote.h
    noteseditor.cpp
    noteseditor.h
    pianowindow.cpp
    pianowindow.h
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
    PRIVATE cxx_std_20
    )

target_link_libraries(imgui-vsthost
    ${OPENGL_LIBRARIES}
    winmm
    RtMidi
    glad
    imgui
    )

target_compile_definitions(imgui-vsthost
    PRIVATE -DUNICODE
    PRIVATE -D_WIN32_WINNT=0x602
    PRIVATE -D__WINDOWS_MM__
    PRIVATE -DTEST_YOUR_CODE
    PRIVATE -DNOMINMAX
    )

target_include_directories(imgui-vsthost
    PRIVATE include
    PRIVATE "VST3 SDK"
    PRIVATE thirdparty/IconFontCppHeaders/include
    )
