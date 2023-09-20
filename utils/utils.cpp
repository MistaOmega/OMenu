//
// Created by jackn on 15/09/2023.
//

#include "utils.h"
#include "../console/console.h"
#include "../hooks/detours.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase; // NOLINT(*-reserved-identifier)
static DWORD __stdcall _UnloadDLL(LPVOID lpParam) {
    FreeLibraryAndExitThread(Utils::GetCurrentImageBase(), 0);
    return 0;
}

namespace Utils {
    void UnloadDLL() {
        HANDLE hThread = CreateThread(nullptr, 0, _UnloadDLL, nullptr, 0, nullptr);
        if (hThread != nullptr) CloseHandle(hThread);
    }

    HMODULE GetCurrentImageBase() {
        return (HINSTANCE) (&__ImageBase);
    }

    HWND GetProcessWindow() {
        HWND hWnd = nullptr;

        while (!hWnd) {
            EnumWindows([](HWND handle, LPARAM lParam) -> BOOL {
                const auto isMainWindow = [](HWND hwnd) {
                    return GetWindow(hwnd, GW_OWNER) == nullptr && IsWindowVisible(hwnd);
                };

                DWORD pID = 0;
                GetWindowThreadProcessId(handle, &pID);

                if (GetCurrentProcessId() != pID || !isMainWindow(handle) || handle == GetConsoleWindow())
                    return TRUE;

                *reinterpret_cast<HWND *>(lParam) = handle;
                return FALSE;
            }, reinterpret_cast<LPARAM>(&hWnd));

            if (!hWnd) {
                PRINT("Waiting for window to appear.\n");
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }

        std::string name(128, '\0'); // Create a string with 128 characters

        // Get the window text and store it in the string
        int textLength = GetWindowTextA(hWnd, &name[0], 128);

        if (textLength > 0) {
            name.resize(textLength); // Resize the string to the actual text length
            PRINT_COLOR(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "Found window: %s \n", name.c_str());
        } else {
            PRINT_ERROR_COLOR(FOREGROUND_RED, "Failed to retrieve the window name.\n");
        }

        return hWnd;
    }

    int ConvertToNonSRGBFormat(int format) {
        static const std::unordered_map<int, int> formatMap = {
                {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_R8G8B8A8_UNORM},
        };
        auto it = formatMap.find(format);
        if (it != formatMap.end()) {
            return it->second;
        }
        return format;
    }

}