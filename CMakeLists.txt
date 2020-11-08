cmake_minimum_required(VERSION 3.14)

include(cmake/CPM.cmake)

find_package(OpenGL REQUIRED)

CPMAddPackage(
    NAME spdlog
    GITHUB_REPOSITORY gabime/spdlog
    VERSION 1.7.0)

CPMAddPackage(
  NAME yaml-cpp
  GITHUB_REPOSITORY jbeder/yaml-cpp
  # 0.6.2 uses deprecated CMake syntax
  VERSION 0.6.3
  # 0.6.3 is not released yet, so use a recent commit
  GIT_TAG 012269756149ae99745b6dafefd415843d7420bb
  OPTIONS
    "YAML_CPP_BUILD_TESTS Off"
    "YAML_CPP_BUILD_CONTRIB Off"
    "YAML_CPP_BUILD_TOOLS Off"
)

CPMAddPackage(
  NAME fmt
  GIT_TAG 6.1.2
  GITHUB_REPOSITORY fmtlib/fmt
)

CPMAddPackage(
  NAME glm
  GITHUB_REPOSITORY g-truc/glm
  GIT_TAG 0.9.9.7
)

project(vsthost)

set(CMAKE_CXX_STANDARD 14)

add_library(RtMidi
    thirdparty/RtMidi/include/RtError.h
    thirdparty/RtMidi/include/RtMidi.h
    thirdparty/RtMidi/RtMidi.cpp)

target_include_directories(RtMidi
    PUBLIC thirdparty/RtMidi/include)

target_compile_definitions(RtMidi
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
    thirdparty/imgui/imgui_draw.cpp
    thirdparty/imgui/imgui_widgets.cpp
    thirdparty/ImGuiFileDialog/ImGuiFileDialog/ImGuiFileDialog.cpp
    thirdparty/ImGuiFileDialog/ImGuiFileDialog/ImGuiFileDialog.h
    thirdparty/ImGuiFileDialog/ImGuiFileDialog/ImGuiFileDialogConfig.h
    thirdparty/ImGuiFileDialog/CustomFont.cpp)

target_include_directories(imgui
    PUBLIC thirdparty/imgui
    PUBLIC thirdparty/ImGuiFileDialog/ImGuiFileDialog
    PUBLIC thirdparty/ImGuiFileDialog
    PUBLIC thirdparty/dirent/include)

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
    arpeggiator.cpp
    arpeggiator.h
    arpeggiatorpreviewservice.cpp
    arpeggiatorpreviewservice.h
    base64.cpp
    base64.h
    common.h
    imgui_impl_win32_gl2.cpp
    imgui_impl_win32_gl2.h
    imguiutils.cpp
    imguiutils.h
    inspectorwindow.cpp
    inspectorwindow.h
    instrument.cpp
    instrument.h
    ivstpluginservice.h
    main.cpp
    midicontrollers.h
    midievent.cpp
    midievent.h
    midinote.cpp
    midinote.h
    notepreviewservice.cpp
    notepreviewservice.h
    noteseditor.cpp
    noteseditor.h
    pianowindow.cpp
    pianowindow.h
    region.cpp
    region.h
    song.cpp
    song.h
    state.cpp
    state.h
    track.cpp
    track.h
    trackseditor.cpp
    trackseditor.h
    tracksmanager.cpp
    tracksmanager.h
    tracksserializer.cpp
    tracksserializer.h
    vstplugin.cpp
    vstplugin.h
    wasapi.cpp
    wasapi.h
    win32vstpluginservice.cpp
    win32vstpluginservice.h
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
    glm
    imgui
    spdlog
    yaml-cpp
    fmt
    )

target_compile_definitions(imgui-vsthost
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
