//
// Created by jackn on 20/09/2023.
//

#ifndef OMENU_CONFIG_H
#define OMENU_CONFIG_H

#include "../pch.h"
#include "../hooks/detours.h"
template<typename T>
class MenuVar {
public:
    std::string_view name;
    T value;

    MenuVar(std::string_view name, T valIn) : name(name), value(valIn) {}

    [[nodiscard]] T GetValue() const {
        return value;
    }
    // Function to get a pointer to the value
    T* GetPtr() {
        return &value;
    }

    explicit operator T() const {
        return value;
    }
};

/**
 * This is where you will store your menu variables that ImGUI can interact with
 */
class MenuVars {
public:
    MenuVar<bool> enabled = {"enabled", false};
};

inline MenuVars &menuVars() {
    static MenuVars menuVars;
    return menuVars;
}

#endif //OMENU_CONFIG_H
