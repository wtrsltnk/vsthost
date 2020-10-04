// ImGui GLFW binding with OpenGL (legacy, fixed pipeline)
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)

// Implemented features:
//  [X] User texture binding. Cast 'GLuint' OpenGL texture identifier as void*/ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the opengl3_example/ folder**
// See imgui_impl_glfw.cpp for details.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include <windows.h>
#include <chrono>

typedef struct
{
    bool done;
    std::chrono::high_resolution_clock::time_point startTime;
} GLFWSystem;

typedef struct
{
    HWND hwnd;
    HDC hdc;
    HGLRC hrc;

    int mouseButtonStates[3];

} GLFWwindow;

typedef HANDLE GLFWmonitor;

#define     GLFW_PRESS 1
#define     GLFW_RELEASE 0

#define     GLFW_FOCUSED            0x00020001
#define 	GLFW_CURSOR             0x00033001
#define 	GLFW_CURSOR_NORMAL      0x00034001
#define 	GLFW_CURSOR_HIDDEN      0x00034002
#define 	GLFW_CURSOR_DISABLED    0x00034003

int glfwInit();
void glfwTerminate();
GLFWwindow *glfwCreateWindow(int w, int h, wchar_t const *title, GLFWmonitor *monitor, GLFWwindow *share);
void glfwSwapBuffers(GLFWwindow *window);
void glfwMakeContextCurrent(GLFWwindow *window);
void glfwSwapInterval(int interval);
bool glfwWindowShouldClose(GLFWwindow *window);
bool glfwPollEvents();
void glfwGetFramebufferSize(GLFWwindow *window, int *width, int *height);
void glfwGetWindowSize(GLFWwindow *window, int *width, int *height);
double glfwGetTime();
void glfwGetCursorPos(GLFWwindow *window, double *xpos, double *ypos);
void glfwSetCursorPos(GLFWwindow *window, double xpos, double ypos);
int glfwGetWindowAttrib(GLFWwindow *window, int attrib);
int glfwGetInputMode(GLFWwindow *window, int mode);
void glfwSetInputMode(GLFWwindow *window, int mode, int value);

IMGUI_API bool        ImGui_ImplGlfwGL2_Init(GLFWwindow* window, bool install_callbacks);
IMGUI_API void        ImGui_ImplGlfwGL2_Shutdown();
IMGUI_API void        ImGui_ImplGlfwGL2_NewFrame();
IMGUI_API void        ImGui_ImplGlfwGL2_RenderDrawData(ImDrawData* draw_data);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGui_ImplGlfwGL2_InvalidateDeviceObjects();
IMGUI_API bool        ImGui_ImplGlfwGL2_CreateDeviceObjects();

// GLFW callbacks (registered by default to GLFW if you enable 'install_callbacks' during initialization)
// Provided here if you want to chain callbacks yourself. You may also handle inputs yourself and use those as a reference.
IMGUI_API void        ImGui_ImplGlfw_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
IMGUI_API void        ImGui_ImplGlfw_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
IMGUI_API void        ImGui_ImplGlfw_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
IMGUI_API void        ImGui_ImplGlfw_CharCallback(GLFWwindow* window, unsigned int c);
