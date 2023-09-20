//
// Created by jackn on 15/09/2023.
//

#ifndef OMENU_CONSOLE_H
#define OMENU_CONSOLE_H

#include "consolecolor.h"

//#define DISABLE_LOGGING // Defining this will disable the debug console

#ifndef DISABLE_LOGGING
// Standard OMENU print
#define PRINT(format, ...) printf("[OMENU] " format "\n", __VA_ARGS__)
// Print a custom leading format like <[Your Hack Name] ESP Loaded> for example, allows differentiation between menu and your own usage
#define PRINT_CUSTOM(name, format, ...) printf(name " " format "\n", __VA_ARGS__);
// Print with an added [ERROR] text
#define PRINT_ERROR(format, ...) {printf("[OMENU] [ERROR] " format "\n", __VA_ARGS__);}

// Standard OMENU print, supports custom color
#define PRINT_COLOR(color, format, ...) { \
    ConsoleColor::SetConsoleTextColor(color); \
    printf("[OMENU] " format "\n", __VA_ARGS__); \
    ConsoleColor::SetConsoleTextColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); \
}

// Print a custom leading format like <[Your Hack Name] ESP Loaded> for example, allows differentiation between menu and your own usage, supports custom color
#define PRINT_CUSTOM_COLOR(color, name, format, ...) { \
    ConsoleColor::SetConsoleTextColor(color); \
    printf(name " " format "\n", __VA_ARGS__); \
    ConsoleColor::SetConsoleTextColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); \
}

// Print with an added [ERROR] text, supports custom color
#define PRINT_ERROR_COLOR(color, format, ...) { \
    ConsoleColor::SetConsoleTextColor(color); \
    printf("[OMENU] [ERROR] " format "\n", __VA_ARGS__); \
    ConsoleColor::SetConsoleTextColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); \
}
#else
#define PRINT(format, ...)
#define PRINT_CUSTOM(name, format, ...);
#define PRINT_ERROR(format, ...)
#define PRINT_COLOR(color, format, ...)
#define PRINT_CUSTOM_COLOR(color, format, ...)
#define PRINT_ERROR_COLOR(color, format, ...)
#endif

namespace DebugConsole {
    bool Alloc();

    void Release();

    void PrintTitle();
}

#endif //OMENU_CONSOLE_H
