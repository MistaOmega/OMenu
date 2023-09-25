#include "pch.h"
#include "console/console.h"
#include "utils/rendering_framework.h"
#include "hooks/detours.h"
#include "dependencies/minhook/MinHook.h"

BOOL WINAPI OnProcessAttach(HMODULE hMod) {
    DebugConsole::Alloc();
    if (Rendering::GetFramework() == NONE) {
        PRINT_ERROR_COLOR(FOREGROUND_RED | FOREGROUND_INTENSITY, "No backend set, please press enter to exit");
        std::cin.get();

        FreeLibraryAndExitThread(hMod, 0);
        return 0;
    }

    MH_Initialize();
    Hooks::Create();
    return 0;
}

BOOL WINAPI OnProcessDetach(HMODULE hMod) {
    Hooks::Release();
    MH_Uninitialize();
    DebugConsole::Release();
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);

        Rendering::SetFramework(Framework::DIRECTX11);

        HANDLE hHandle = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(OnProcessAttach), hinstDLL,
                                      0, nullptr);
        if (hHandle != nullptr) {
            CloseHandle(hHandle);
        }
    } else if (fdwReason == DLL_PROCESS_DETACH && !lpReserved) {
        OnProcessDetach(nullptr);
    }

    return TRUE;
}

