//
// Created by jackn on 15/09/2023.
//

#include "../pch.h"
#ifdef _WIN32
#include "../dependencies/imgui/imgui_impl_win32.h"
#endif

#ifndef OMENU_DETOURS_H
#define OMENU_DETOURS_H
namespace Hooks {
#ifdef _WIN32
    void Create();

    void Release();
#endif

    inline bool Hooked = true;
}

#endif //OMENU_DETOURS_H
