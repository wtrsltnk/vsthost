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

add_subdirectory(thirdparty)

add_executable(minimal-vsthost
    "src/minimal_vst2x_host.cpp"
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
    "src/historymanager.cpp"
    "src/historymanager.h"
    "src/imgui_impl_win32_gl2.cpp"
    "src/imgui_impl_win32_gl2.h"
    "src/imguiutils.cpp"
    "src/imguiutils.h"
    "src/main.cpp"
    "src/notepreviewservice.cpp"
    "src/notepreviewservice.h"
    "src/state.cpp"
    "src/state.h"
    "src/ui/abstracttimelineeditor.cpp"
    "src/ui/abstracttimelineeditor.h"
    "src/ui/docs.md"
    "src/ui/inspectorwindow.cpp"
    "src/ui/inspectorwindow.h"
    "src/ui/noteseditor.cpp"
    "src/ui/noteseditor.h"
    "src/ui/pianowindow.cpp"
    "src/ui/pianowindow.h"
    "src/ui/trackseditor.cpp"
    "src/ui/trackseditor.h"
    "src/wasapi.cpp"
    "src/wasapi.h"
)

target_compile_features(imgui-vsthost
    PRIVATE cxx_auto_type
    PRIVATE cxx_nullptr
    PRIVATE cxx_range_for
    PRIVATE cxx_std_20
)

target_link_libraries(imgui-vsthost
    ${OPENGL_LIBRARIES}
    tracks-domain
    sqlite
    winmm
    RtMidi
    glad
    glm
    imgui
    spdlog
    yaml-cpp
    fmt
    boolinq
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

add_library(tracks-domain
    "include/instrument.h"
    "include/ipluginservice.h"
    "include/midicontrollers.h"
    "include/midievent.h"
    "include/midinote.h"
    "include/region.h"
    "include/song.h"
    "include/track.h"
    "include/tracksmanager.h"
    "include/tracksserializer.h"
    "include/vstplugin.h"
    "include/pluginservice.h"
    "src/tracks-domain/base64.cpp"
    "src/tracks-domain/base64.h"
    "src/tracks-domain/instrument.cpp"
    "src/tracks-domain/midievent.cpp"
    "src/tracks-domain/midinote.cpp"
    "src/tracks-domain/region.cpp"
    "src/tracks-domain/song.cpp"
    "src/tracks-domain/track.cpp"
    "src/tracks-domain/tracksmanager.cpp"
    "src/tracks-domain/tracksserializer.cpp"
    "src/tracks-domain/vstplugin.cpp"
    "src/tracks-domain/pluginservice.cpp"
    "src/widestringconversions.cpp"
)

target_compile_features(tracks-domain
    PRIVATE cxx_auto_type
    PRIVATE cxx_nullptr
    PRIVATE cxx_range_for
    PRIVATE cxx_std_20
)

target_include_directories(tracks-domain
    PRIVATE include
    PRIVATE "VST3 SDK"
)

target_link_libraries(tracks-domain
    spdlog
    yaml-cpp
    fmt
    glm
    sqlite
    boolinq
)

target_compile_definitions(tracks-domain
    PRIVATE -DUNICODE
    PRIVATE -D_WIN32_WINNT=0x602
)
