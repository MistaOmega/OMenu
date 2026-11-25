#include "utils.h"
#include "../console/console.h"
#include "../hooks/detours.h"

namespace Utils {
    void UnloadDLL() {
        Hooks::Hooked = false;
        PRINT("Overlay disabled; the library stays loaded until the game exits.");
    }
}
