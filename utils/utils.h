//
// Created by jackn on 15/09/2023.
//

#ifndef OMENU_UTILS_H
#define OMENU_UTILS_H

namespace Utils {
    HMODULE GetCurrentImageBase();

    HWND GetProcessWindow();

    int ConvertToNonSRGBFormat(int format);

    void UnloadDLL();
}

#endif //OMENU_UTILS_H
