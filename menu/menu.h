//
// Created by jackn on 19/09/2023.
//

#ifndef OMENU_MENU_H
#define OMENU_MENU_H

#include "../pch.h"
#include "../dependencies/imgui/imgui.h"

namespace Menu {
    void CreateImGuiForWindow(HWND hWnd);

    void Render();

    void CreateStyle();

    inline bool showMenu = true;
}

#endif //OMENU_MENU_H
