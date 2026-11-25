#ifndef OMENU_PLATFORM_X11_H
#define OMENU_PLATFORM_X11_H

// ImGui platform layer for Linux. we poll X11 on our own connection and don't touch the game's events,
// so the game still gets every click and key
namespace LinuxPlatform {
    // call on the render thread before ImGui::NewFrame. pass the X11 window we draw to, or 0 if we don't
    // know it like with EGL, then the mouse comes from whatever's focused. no X server means just display size and delta time
    void NewFrame(int width, int height, unsigned long window);

    void Shutdown();
}

#endif //OMENU_PLATFORM_X11_H
