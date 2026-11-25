//
// Created by jackn on 15/09/2023.
//

#ifndef OMENU_UTILS_H
#define OMENU_UTILS_H

namespace Utils {
#ifdef _WIN32
    HMODULE GetCurrentImageBase();

    HWND GetProcessWindow();

    int ConvertToNonSRGBFormat(int format);
#endif

    // can't unload a preloaded lib on Linux, this just stops the overlay and the hooks pass everything through
    void UnloadDLL();
}

#endif //OMENU_UTILS_H
