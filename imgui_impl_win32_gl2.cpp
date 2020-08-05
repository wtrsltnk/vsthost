#include <imgui.h>
#include "imgui_impl_win32_gl2.h"

#include <glad/glad.h>
#include <GL/wglext.h>
#include <iostream>

// GLFW data
static GLFWSystem   g_System;
static GLFWwindow*  g_Window = NULL;
static double       g_Time = 0.0f;
static bool         g_MouseJustPressed[3] = { false, false, false };
//static GLFWcursor*  g_MouseCursors[ImGuiMouseCursor_COUNT] = { 0 };

// OpenGL data
static GLuint       g_FontTexture = 0;

wchar_t const szAppName[] = L"ImGuiExample";

// OpenGL2 Render function.
// (this used to be set in io.RenderDrawListsFn and called by ImGui::Render(), but you can now call this directly from your main loop)
// Note that this implementation is little overcomplicated because we are saving/setting up/restoring every OpenGL state explicitly, in order to be able to run within any OpenGL engine that doesn't do so. 
void ImGui_ImplGlfwGL2_RenderDrawData(ImDrawData* draw_data)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
        return;
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    // We are using the OpenGL fixed pipeline to make the example code simpler to read!
    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers, polygon fill.
    GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    GLint last_polygon_mode[2]; glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box); 
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_TEXTURE_2D);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound

    // Setup viewport, orthographic projection matrix
    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f, -1.0f, +1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
        const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;
        glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, pos)));
        glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, uv)));
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, col)));

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer);
            }
            idx_buffer += pcmd->ElemCount;
        }
    }

    // Restore modified state
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindTexture(GL_TEXTURE_2D, (GLuint)last_texture);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
    glPolygonMode(GL_FRONT, (GLenum)last_polygon_mode[0]); glPolygonMode(GL_BACK, (GLenum)last_polygon_mode[1]);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
    glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
}

static char const *ImGui_ImplGlfwGL2_GetClipboardText(void* user_data)
{
    return "";
}

static void ImGui_ImplGlfwGL2_SetClipboardText(void* user_data, const char* text)
{ }

void ImGui_ImplGlfw_MouseButtonCallback(GLFWwindow*, int button, int action, int /*mods*/)
{
    if (action == GLFW_PRESS && button >= 0 && button < 3)
        g_MouseJustPressed[button] = true;
}

void ImGui_ImplGlfw_ScrollCallback(GLFWwindow*, double xoffset, double yoffset)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheelH += (float)xoffset;
    io.MouseWheel += (float)yoffset;
}

void ImGui_ImplGlfw_KeyCallback(GLFWwindow*, int key, int, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (action == GLFW_PRESS)
        io.KeysDown[key] = true;
    if (action == GLFW_RELEASE)
        io.KeysDown[key] = false;

    (void)mods; // Modifiers are not reliable across systems
    io.KeyCtrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
    io.KeyShift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
    io.KeyAlt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
    io.KeySuper = false;
}

void ImGui_ImplGlfw_CharCallback(GLFWwindow*, unsigned int c)
{
    ImGuiIO& io = ImGui::GetIO();
    if (c > 0 && c < 0x10000)
        io.AddInputCharacter((unsigned short)c);
}

bool ImGui_ImplGlfwGL2_CreateDeviceObjects()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

    // Upload texture to graphics system
    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGenTextures(1, &g_FontTexture);
    glBindTexture(GL_TEXTURE_2D, g_FontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;

    // Restore state
    glBindTexture(GL_TEXTURE_2D, last_texture);

    return true;
}

void    ImGui_ImplGlfwGL2_InvalidateDeviceObjects()
{
    if (g_FontTexture)
    {
        glDeleteTextures(1, &g_FontTexture);
        ImGui::GetIO().Fonts->TexID = 0;
        g_FontTexture = 0;
    }
}

static void ImGui_ImplGlfw_InstallCallbacks(GLFWwindow* window)
{
}

bool    ImGui_ImplGlfwGL2_Init(GLFWwindow* window, bool install_callbacks)
{
    g_Window = window;

    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = VK_TAB;                     // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
    io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
    io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
    io.KeyMap[ImGuiKey_Home] = VK_HOME;
    io.KeyMap[ImGuiKey_End] = VK_END;
    io.KeyMap[ImGuiKey_Insert] = VK_INSERT;
    io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
    io.KeyMap[ImGuiKey_Space] = VK_SPACE;
    io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
    io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
    io.KeyMap[ImGuiKey_A] = 'A';
    io.KeyMap[ImGuiKey_C] = 'C';
    io.KeyMap[ImGuiKey_V] = 'V';
    io.KeyMap[ImGuiKey_X] = 'X';
    io.KeyMap[ImGuiKey_Y] = 'Y';
    io.KeyMap[ImGuiKey_Z] = 'Z';

    io.SetClipboardTextFn = ImGui_ImplGlfwGL2_SetClipboardText;
    io.GetClipboardTextFn = ImGui_ImplGlfwGL2_GetClipboardText;
    io.ClipboardUserData = g_Window;
#ifdef _WIN32
    io.ImeWindowHandle = g_Window->hwnd;
#endif

    // Load cursors
    // FIXME: GLFW doesn't expose suitable cursors for ResizeAll, ResizeNESW, ResizeNWSE. We revert to arrow cursor for those.
//    g_MouseCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
//    g_MouseCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
//    g_MouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
//    g_MouseCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
//    g_MouseCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
//    g_MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
//    g_MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);

    if (install_callbacks)
    {
        ImGui_ImplGlfw_InstallCallbacks(window);
    }

    return true;
}

void ImGui_ImplGlfwGL2_Shutdown()
{
//    // Destroy GLFW mouse cursors
//    for (ImGuiMouseCursor cursor_n = 0; cursor_n < ImGuiMouseCursor_COUNT; cursor_n++)
//    {
//        glfwDestroyCursor(g_MouseCursors[cursor_n]);
//    }
//    memset(g_MouseCursors, 0, sizeof(g_MouseCursors));

    // Destroy OpenGL objects
    ImGui_ImplGlfwGL2_InvalidateDeviceObjects();
}

void ImGui_ImplGlfwGL2_NewFrame()
{
    if (!g_FontTexture)
    {
        ImGui_ImplGlfwGL2_CreateDeviceObjects();
    }

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    glfwGetWindowSize(g_Window, &w, &h);
    glfwGetFramebufferSize(g_Window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)w, (float)h);
    io.DisplayFramebufferScale = ImVec2(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0);

    // Setup time step
    double current_time =  glfwGetTime();
    io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f/60.0f);
    g_Time = current_time;

    // Setup inputs
    // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
    if (glfwGetWindowAttrib(g_Window, GLFW_FOCUSED))
    {
//        if (io.WantCaptureMouse)
//        {
//            glfwSetCursorPos(g_Window, (double)io.MousePos.x, (double)io.MousePos.y);   // Set mouse position if requested by io.WantMoveMouse flag (used when io.NavMovesTrue is enabled by user and using directional navigation)
//        }
//        else
        {
            double mouse_x, mouse_y;
            glfwGetCursorPos(g_Window, &mouse_x, &mouse_y);
            io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);
        }
    }
    else
    {
        io.MousePos = ImVec2(-FLT_MAX,-FLT_MAX);
    }

    for (int i = 0; i < 3; i++)
    {
        // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        io.MouseDown[i] = g_MouseJustPressed[i] || g_Window->mouseButtonStates[i] != 0;
        g_MouseJustPressed[i] = false;
    }

    // Update OS/hardware mouse cursor if imgui isn't drawing a software cursor
    ImGuiMouseCursor cursor = ImGui::GetMouseCursor();
    if (io.MouseDrawCursor || cursor == ImGuiMouseCursor_None)
    {
//        glfwSetInputMode(g_Window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }
    else
    {
//        glfwSetCursor(g_Window, g_MouseCursors[cursor] ? g_MouseCursors[cursor] : g_MouseCursors[ImGuiMouseCursor_Arrow]);
//        glfwSetInputMode(g_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    // Start the frame. This call will update the io.WantCaptureMouse, io.WantCaptureKeyboard flag that you can use to dispatch inputs (or not) to your application.
    ImGui::NewFrame();
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto window = reinterpret_cast<GLFWwindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_CREATE:
    {
        window = reinterpret_cast<GLFWwindow*>(((LPCREATESTRUCT)lParam)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)((LPCREATESTRUCT)lParam)->lpCreateParams);
        break;
    }
    case WM_CLOSE:
    {
        PostQuitMessage(0);
        break;
    }
    case WM_KEYDOWN:
    {
        ImGui_ImplGlfw_KeyCallback(window, wParam, 0, GLFW_PRESS, 0);
        break;
    }
    case WM_KEYUP:
    {
        ImGui_ImplGlfw_KeyCallback(window, wParam, 0, GLFW_RELEASE, 0);
        break;
    }
    case WM_CHAR:
    {
        ImGui_ImplGlfw_CharCallback(window, char(wParam));
        break;
    }
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    {
        ImGui_ImplGlfw_MouseButtonCallback(window, 0, message != WM_LBUTTONUP ? GLFW_PRESS : GLFW_RELEASE, 0);
        g_Window->mouseButtonStates[0] = (message == WM_LBUTTONDOWN);
        break;
    }
    case WM_MBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    {
        ImGui_ImplGlfw_MouseButtonCallback(window, 2, message != WM_MBUTTONUP ? GLFW_PRESS : GLFW_RELEASE, 0);
        g_Window->mouseButtonStates[2] = (message == WM_MBUTTONDOWN);
        break;
    }
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    {
        ImGui_ImplGlfw_MouseButtonCallback(window, 1, message != WM_RBUTTONUP ? GLFW_PRESS : GLFW_RELEASE, 0);
        g_Window->mouseButtonStates[1] = (message == WM_RBUTTONDOWN);
        break;
    }
    case WM_MOUSEWHEEL:
    {
        ImGui_ImplGlfw_ScrollCallback(window, 0.0, (SHORT) HIWORD(wParam) / (double) WHEEL_DELTA);
        break;
    }
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

int glfwInit()
{
    WNDCLASS wc;
    if (GetClassInfo(GetModuleHandle(NULL), szAppName, &wc))
    {
        return 0;
    }

    ZeroMemory(&wc, sizeof wc);
    wc.hInstance     = GetModuleHandle(NULL);
    wc.lpszClassName = szAppName;
    wc.lpfnWndProc   = (WNDPROC)WndProc;
    wc.style         = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    wc.hbrBackground = NULL;
    wc.lpszMenuName  = NULL;
    wc.hIcon         = LoadIcon(NULL, IDI_WINLOGO);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

    if (FALSE == RegisterClass(&wc))
    {
        std::cerr << "Failed to register window class for slut\n";
        return 0;
    }

    g_System.done = false;
    g_System.startTime = std::chrono::high_resolution_clock::now();

    return 1;
}

void glfwTerminate()
{
    UnregisterClass(szAppName, GetModuleHandle(NULL));
}

static void getFullWindowSize(DWORD style, DWORD exStyle,
                              int clientWidth, int clientHeight,
                              int* fullWidth, int* fullHeight)
{
    RECT rect = { 0, 0, clientWidth, clientHeight };
    AdjustWindowRectEx(&rect, style, FALSE, exStyle);
    *fullWidth = rect.right - rect.left;
    *fullHeight = rect.bottom - rect.top;
}

GLFWwindow *glfwCreateWindow(int w, int h, wchar_t const *title, GLFWmonitor *monitor, GLFWwindow *share)
{
    auto window = new GLFWwindow();

    for (int i = 0; i < 3; i++)
    {
        window->mouseButtonStates[i] = 0;
    }

    int fullWidth, fullHeight;
    auto styleEx = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    auto style = WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

    getFullWindowSize(style, styleEx, w, h, &fullWidth, &fullHeight);
    window->hwnd = CreateWindowEx(
                styleEx,
                szAppName,
                title,
                style,
                CW_USEDEFAULT, CW_USEDEFAULT,
                fullWidth, fullHeight,
                0,
                0,
                GetModuleHandle(NULL),
                reinterpret_cast<LPVOID>(window));

    if (window->hwnd == NULL)
    {
        return nullptr;
    }

    window->hdc = GetWindowDC(window->hwnd);

    if (window->hdc == NULL)
    {
        DestroyWindow(window->hwnd);

        return nullptr;
    }

    PIXELFORMATDESCRIPTOR pfd;

    ZeroMemory(&pfd, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 16;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int format = ChoosePixelFormat(window->hdc, &pfd);

    if (format == 0)
    {
        ReleaseDC(window->hwnd, window->hdc);
        DestroyWindow(window->hwnd);

        return nullptr;
    }

    if (SetPixelFormat(window->hdc, format, &pfd) == FALSE)
    {
        ReleaseDC(window->hwnd, window->hdc);
        DestroyWindow(window->hwnd);

        return nullptr;
    }

    window->hrc = wglCreateContext(window->hdc);

    if (window->hrc == NULL)
    {
        ReleaseDC(window->hwnd, window->hdc);
        DestroyWindow(window->hwnd);

        return nullptr;
    }

    wglMakeCurrent(window->hdc, window->hrc);

    gladLoadGL();

    return window;
}

void glfwSwapBuffers(GLFWwindow *window)
{
    SwapBuffers(window->hdc);
}

void glfwMakeContextCurrent(GLFWwindow *window)
{
    wglMakeCurrent(window->hdc, window->hrc);
}

void glfwSwapInterval(int interval)
{ }

bool glfwWindowShouldClose(GLFWwindow *window)
{
    return g_System.done;
}

void glfwPollEvents()
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            g_System.done = true;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void glfwGetFramebufferSize(GLFWwindow *window, int *width, int *height)
{
    RECT rect;
    if(GetClientRect(window->hwnd, &rect))
    {
        *width = int(rect.right - rect.left);
        *height = int(rect.bottom - rect.top);
    }
}

void glfwGetWindowSize(GLFWwindow *window, int *width, int *height)
{
    RECT rect;
    if(GetClientRect(window->hwnd, &rect))
    {
        *width = int(rect.right - rect.left);
        *height = int(rect.bottom - rect.top);
    }
}

double glfwGetTime()
{
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - g_System.startTime).count();

    return double(ms / 1000.0);
}

void glfwGetCursorPos(GLFWwindow *window, double *xpos, double *ypos)
{
    POINT point;
    GetCursorPos(&point);
    ScreenToClient(window->hwnd, &point);
    *xpos = point.x;
    *ypos = point.y;
}

void glfwSetCursorPos(GLFWwindow *window, double xpos, double ypos)
{
    POINT point;
    point.x = (LONG)xpos;
    point.y = (LONG)ypos;
    ClientToScreen(window->hwnd, &point);
    SetCursorPos(point.x, point.y);
}

int glfwGetWindowAttrib(GLFWwindow *window, int attrib)
{
    if (attrib == GLFW_FOCUSED)
    {
        return GetFocus() == window->hwnd ? 1 : 0;
    }

    return 0 ;
}

int glfwGetInputMode(GLFWwindow *window, int mode)
{
    return 0;
}

void glfwSetInputMode(GLFWwindow *window, int mode, int value)
{ }
