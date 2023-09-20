//
// Created by jackn on 20/09/2023.
//

#include "consolecolor.h"

void ConsoleColor::SetConsoleTextColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}
