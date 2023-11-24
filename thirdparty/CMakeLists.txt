
set(CMAKE_CXX_STANDARD 14)

add_library(RtMidi
    "RtMidi/include/RtError.h"
    "RtMidi/include/RtMidi.h"
    "RtMidi/RtMidi.cpp"
)

target_include_directories(RtMidi
    PUBLIC RtMidi/include
)

target_compile_definitions(RtMidi
    PRIVATE -D_WIN32_WINNT=0x602
    PRIVATE -D__WINDOWS_MM__
)

target_link_libraries(RtMidi
    winmm
)

add_library(sqlite
    "sqlite/sqlite3.c"
    "sqlite/include/sqlite3.h"
    "sqlite/include/sqlitelib.h"
)

target_include_directories(sqlite
    PUBLIC sqlite/include)

add_library(glad
    "glad/glad.c"
)

target_include_directories(glad
    PUBLIC glad/include)

add_library(imgui
    "ImGuiFileDialog/CustomFont.cpp"
    "ImGuiFileDialog/ImGuiFileDialog/ImGuiFileDialog.cpp"
    "ImGuiFileDialog/ImGuiFileDialog/ImGuiFileDialog.h"
    "ImGuiFileDialog/ImGuiFileDialog/ImGuiFileDialogConfig.h"
    "imgui/imgui.cpp"
    "imgui/imgui_demo.cpp"
    "imgui/imgui_draw.cpp"
    "imgui/imgui_tables.cpp"
    "imgui/imgui_widgets.cpp"
)

target_include_directories(imgui
    PUBLIC imgui
    PUBLIC ImGuiFileDialog/ImGuiFileDialog
    PUBLIC ImGuiFileDialog
    PUBLIC dirent/include
)
