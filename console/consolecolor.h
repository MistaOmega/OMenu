//
// Created by jackn on 20/09/2023.
//

#ifndef OMENU_CONSOLECOLOR_H
#define OMENU_CONSOLECOLOR_H

#ifndef _WIN32
// same values as wincon.h, keeps the colour args working on Linux
#define FOREGROUND_BLUE      0x0001
#define FOREGROUND_GREEN     0x0002
#define FOREGROUND_RED       0x0004
#define FOREGROUND_INTENSITY 0x0008
#endif

namespace ConsoleColor {
    void SetConsoleTextColor(int color);
}


#endif //OMENU_CONSOLECOLOR_H
