//
// Created by jackn on 20/09/2023.
//

#include "../pch.h"
#include "consolecolor.h"

#ifdef _WIN32
void ConsoleColor::SetConsoleTextColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}
#else
void ConsoleColor::SetConsoleTextColor(int color) {
    // ANSI has red and blue the other way round to Windows, so swap them
    int rgb = ((color & FOREGROUND_RED) ? 1 : 0) |
              ((color & FOREGROUND_GREEN) ? 2 : 0) |
              ((color & FOREGROUND_BLUE) ? 4 : 0);
    if (rgb == 7 && !(color & FOREGROUND_INTENSITY)) {
        std::fputs("\033[0m", stdout);
        return;
    }
    std::printf("\033[%dm", ((color & FOREGROUND_INTENSITY) ? 90 : 30) + rgb);
}
#endif
