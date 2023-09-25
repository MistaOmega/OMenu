//
// Created by jackn on 15/09/2023.
//
#include "console.h"
#include "../hooks/detours.h"

// Function to calculate the centering padding
int CalculatePadding(int consoleWidth, int lineLength) {
    return (consoleWidth - lineLength) / 2;
}

namespace DebugConsole {
    bool Alloc() {
#ifndef DISABLE_LOGGING
        if (AllocConsole()) {
            SetConsoleTitleA("OHook - Console");

            FILE *pFile;
            if (freopen_s(&pFile, "CONIN$", "r", stdin) == 0 &&
                freopen_s(&pFile, "CONOUT$", "w", stdout) == 0 &&
                freopen_s(&pFile, "CONOUT$", "w", stderr) == 0) {
                ::ShowWindow(GetConsoleWindow(), SW_SHOW);
                ::BringWindowToTop(GetConsoleWindow());
                PrintTitle();
                return true;
            }
        }

        return false;
#endif
    }

    void Release() {
#ifndef DISABLE_LOGGING
        fclose(stdin);
        fclose(stdout);
        fclose(stderr);
        if (!Hooks::Hooked) {
            FreeConsole();
            PostMessage(GetConsoleWindow(), WM_CLOSE, 0,
                        0); // force close the window, if we re-inject we get a console back
        } else {
            FreeConsole();
        }
#endif
    }

    void PrintTitle() {
        // Get the console handle and determine the console width (Windows-specific)
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hConsole, &csbi);
        int consoleWidth = csbi.dwSize.X;

        std::vector<int> rainbowColors = {
                FOREGROUND_RED | FOREGROUND_INTENSITY,
                FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
                FOREGROUND_GREEN | FOREGROUND_INTENSITY,
                FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
                FOREGROUND_BLUE | FOREGROUND_INTENSITY,
                FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY
        };

        // Define the ASCII art text
        std::vector<std::string> titleLines = {
                "   ____    __  __   ______   _   _   _    _ ",
                R"(  / __ \  |  \/  | |  ____| | \ | | | |  | |)",
                " | |  | | | \\  / | | |__    |  \\| | | |  | |",
                " | |  | | | |\\/| | |  __|   | . ` | | |  | |",
                " | |__| | | |  | | | |____  | |\\  | | |__| |",
                R"(  \____/  |_|  |_| |______| |_| \_|  \____/ )",
                "                                            "
        };


        // Print top separator line
        std::cout << std::string(consoleWidth, '*') << std::endl;

        // Iterate through each line of the title and apply rainbow colors with padding
        for (const std::string &line: titleLines) {
            int padding = CalculatePadding(consoleWidth, static_cast<int>(line.length()));

            // Print the padding spaces with rainbow colors
            for (int i = 0; i < padding; i++) {
                ConsoleColor::SetConsoleTextColor(rainbowColors[i % rainbowColors.size()]);
                std::cout << " ";
            }

            // Print the line with rainbow colors
            for (size_t i = 0; i < line.size(); i++) {
                ConsoleColor::SetConsoleTextColor(rainbowColors[i % rainbowColors.size()]);
                std::cout << line[i];
            }

            // Reset the text color to default
            ConsoleColor::SetConsoleTextColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            std::cout << std::endl;
        }

        // Print bottom separator line
        std::cout << std::string(consoleWidth, '*') << std::endl;

    }
}