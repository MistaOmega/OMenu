//
// Created by jackn on 19/09/2023.
//

#ifndef OMENU_MENU_H
#define OMENU_MENU_H

#include "../pch.h"

namespace Menu {
    // false if we already have a context
    bool CreateContext();

#ifdef _WIN32
    void CreateImGuiForWindow(HWND hWnd);
#endif

    void Render();

    void CreateStyle();

    inline bool showMenu = true;
}

#endif //OMENU_MENU_H
