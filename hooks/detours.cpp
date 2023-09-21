//
// Created by jackn on 15/09/2023.
//
#include "detours.h"
#include "dx9/hook_dx9.h"
#include "dx12/hook_dx12.h"
#include "opengl/hook_opengl.h"
#include "../utils/rendering_framework.h"
#include "../utils/utils.h"
#include "../menu/menu.h"

static HWND gHWindow = nullptr;

static WNDPROC wndProc;

LRESULT ImGUIWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ImGuiIO &io = ImGui::GetIO();
    // Handle uMsg hotkeys and window destroy state
    if (uMsg == WM_KEYDOWN) {
        if (wParam == VK_INSERT) {
            Menu::showMenu = !Menu::showMenu;
            return true;
        } else if (wParam == VK_END) {
            Hooks::hooked = false;
            Utils::UnloadDLL();
            return true;
        }
    }
    if(Menu::showMenu) {
        // handle window interactions when menu showing
        switch (uMsg) {
            case WM_LBUTTONDOWN:
                io.MouseDown[0] = true;
                return true;
            case WM_LBUTTONUP:
                io.MouseDown[0] = false;
                return true;
            case WM_RBUTTONDOWN:
                io.MouseDown[1] = true;
                return true;
            case WM_RBUTTONUP:
                io.MouseDown[1] = false;
                return true;
            case WM_MBUTTONDOWN:
                io.MouseDown[2] = true;
                return true;
            case WM_MBUTTONUP:
                io.MouseDown[2] = false;
                return true;
            case WM_XBUTTONDOWN:
                if ((GET_KEYSTATE_WPARAM(wParam) & MK_XBUTTON1) == MK_XBUTTON1)
                    io.MouseDown[3] = true;
                else if ((GET_KEYSTATE_WPARAM(wParam) & MK_XBUTTON2) == MK_XBUTTON2)
                    io.MouseDown[4] = true;
                return true;
            case WM_XBUTTONUP:
                if ((GET_KEYSTATE_WPARAM(wParam) & MK_XBUTTON1) == MK_XBUTTON1)
                    io.MouseDown[3] = false;
                else if ((GET_KEYSTATE_WPARAM(wParam) & MK_XBUTTON2) == MK_XBUTTON2)
                    io.MouseDown[4] = false;
                return true;
            case WM_MOUSEWHEEL:
                io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
                return true;
            case WM_MOUSEMOVE:
                io.MousePos.x = (signed short) (lParam);
                io.MousePos.y = (signed short) (lParam >> 16);
                return true;
            case WM_KEYDOWN:
                if (wParam < 256)
                    io.KeysDown[wParam] = true;
                return true;
            case WM_KEYUP:
                if (wParam < 256)
                    io.KeysDown[wParam] = false;
                return true;
            case WM_CHAR:
                if (wParam > 0 && wParam < 0x10000)
                    io.AddInputCharacter((unsigned short) wParam);
                return true;
            default:
                return 0;
        }
    }
    return 0;
}

static LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return ImGUIWndProc(hWnd, uMsg, wParam, lParam) ? 1L : CallWindowProc(wndProc, hWnd, uMsg, wParam, lParam);
}

namespace Hooks {
    void Create() {
        gHWindow = Utils::GetProcessWindow();

#ifdef DISABLE_LOGGING
        bool bNoConsole = GetConsoleWindow( ) == nullptr;
        if (bNoConsole) {
            AllocConsole( );
        }
#endif
        Framework framework = Rendering::GetFramework();
        switch (framework) {
            case DIRECTX9:
                DirectX9::Hook(gHWindow);
                break;
            case DIRECTX12:
                DirectX12::Hook(gHWindow);
                break;
            case OPENGL:
                OpenGL::Hook(gHWindow);
                break;
            case NONE:
                break;
        }

#ifdef DISABLE_LOGGING_CONSOLE
        if (bNoConsole) {
            FreeConsole( );
        }
#endif
        wndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(gHWindow, GWLP_WNDPROC,
                                                             reinterpret_cast<LONG_PTR>(WndProc)));
    }


    void Release() {
        if (wndProc) {
            SetWindowLongPtr(gHWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(wndProc));
        }

        MH_DisableHook(MH_ALL_HOOKS);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        Framework framework = Rendering::GetFramework();
        switch (framework) {
            case DIRECTX9:
                DirectX9::Unhook();
                break;
            case DIRECTX12:
                DirectX12::Unhook();
                break;
            case OPENGL:
                OpenGL::Unhook();
                break;
            case NONE:
                break;
        }
    }
}