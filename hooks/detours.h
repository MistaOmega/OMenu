//
// Created by jackn on 15/09/2023.
//

#include "../pch.h"
#include "../dependencies/imgui/imgui_impl_win32.h"

#ifndef OMENU_DETOURS_H
#define OMENU_DETOURS_H
namespace Hooks {
    void Create();

    void Release();

    inline bool Hooked = true;
}

#endif //OMENU_DETOURS_H
