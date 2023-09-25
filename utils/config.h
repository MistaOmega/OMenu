//
// Created by jackn on 20/09/2023.
//

#ifndef OMENU_CONFIG_H
#define OMENU_CONFIG_H

#include "../pch.h"
#include "../hooks/detours.h"

#define MENUVAR(type, name, value) MenuVar<type> name = {#name, value}

template<typename T>
class MenuVar {
public:
    std::string_view name;
    std::unique_ptr<T> val;
    int32_t size;

    MenuVar(std::string_view name, T valIn) : name(name) {
        val = std::make_unique<T>(valIn);
        size = sizeof(T);
    }

    /*
     * Having non-explicit operators means that we could imply type conversion depending on what "T" is, which shouldn't happen in this case.
     * However, making it explicit and calling the type explicitly with the operator from other classes is good form or something, so. Here.
     */
    explicit operator T() { return *val; }

    explicit operator T *() { return &*val; }

    explicit operator T() const { return *val; }
};

/**
 * This is where you will store your menu variables that ImGUI can interact with
 * as per the defined macro, they should be made as the following
 *
 * MENUVAR(type, name, value) where type is the variable type, name is the variable name, and value is the initial value of the variable
 */
class MenuVars {
    MENUVAR(bool, enabled, false);
};

inline MenuVars &menuVars() {
    try {
        static MenuVars menuVars;
        return menuVars;
    }
    catch (...) {
        Hooks::Hooked = false;
    };
}

#endif //OMENU_CONFIG_H
