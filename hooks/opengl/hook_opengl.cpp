//
// Created by jackn on 21/09/2023.
//

#include "hook_opengl.h"

#include "../../menu/menu.h"
#include "../detours.h"
#include "../../console/console.h"

// Define a pointer to the original function
static std::add_pointer_t<BOOL WINAPI(HDC)> oWglSwapBuffers;
static BOOL WINAPI hkWglSwapBuffers(HDC hDC) {
    if (Hooks::hooked && ImGui::GetCurrentContext()) {
        if (!ImGui::GetIO().BackendRendererUserData) {
            ImGui_ImplOpenGL3_Init();
        }
        ImGui::GetIO().MouseDrawCursor = Menu::showMenu; // Draw a mouse cursor, useful for games like Assault Cube, which don't have mouse input

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        Menu::Render();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    // Call the original function to maintain normal behavior
    return oWglSwapBuffers(hDC);
}

void OpenGL::Hook(HWND hWnd) {
    // Get the module handle for the OpenGL library (opengl32.dll)
    HMODULE hOpenGLModule = GetModuleHandleA("opengl32.dll");

    if (hOpenGLModule) {
        PRINT_COLOR(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "OpenGL32 base address: 0x%p\n", hOpenGLModule);
    }

    // Get a pointer to the function "wglSwapBuffers" from opengl32.dll
    void* fnWglSwapBuffers = reinterpret_cast<void*>(GetProcAddress(hOpenGLModule, "wglSwapBuffers"));

    //init menu
    Menu::CreateImGuiForWindow(hWnd);

    // Create a hook for the "wglSwapBuffers" function and enable if successful
    if (MH_CreateHook(reinterpret_cast<void**>(fnWglSwapBuffers), reinterpret_cast<void**>(&hkWglSwapBuffers), reinterpret_cast<void**>(&oWglSwapBuffers)) == MH_OK) {
        MH_EnableHook(reinterpret_cast<void**>(fnWglSwapBuffers));
        PRINT("Hooked wglSwapBuffers successfully!");
    }
}

void OpenGL::Unhook() {
    if (ImGui::GetCurrentContext( )) {
        if (ImGui::GetIO( ).BackendRendererUserData)
            ImGui_ImplOpenGL3_Shutdown( );

        if (ImGui::GetIO( ).BackendPlatformUserData)
            ImGui_ImplWin32_Shutdown( );

        ImGui::DestroyContext( );
    }
}
