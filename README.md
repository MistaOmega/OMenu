<h1 align="center">OMenu️</h1>
<p align="center">
<img alt="Static Badge" src="https://img.shields.io/badge/Language-C++-61DAFB?logo=c%2B%2B">
<img alt="Static Badge" src="https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-blue">
 <img alt="Version" src="https://img.shields.io/badge/version-1.3.3-hotpink.svg?cacheSeconds=2592000" />
  <a href="https://choosealicense.com/licenses/mit/" target="_blank">
    <img alt="License: MIT" src="https://img.shields.io/badge/License-MIT-yellow.svg" />
  </a>
</p>

> ImGUI menu injection template for various graphic rendering frameworks 

## Compatible APIs
  - DirectX9 (DirectX9Ex)
  - DirectX10
  - DirectX11
  - DirectX12
  - OpenGL
  - Linux native: OpenGL via GLX or EGL (desktop GL, not GLES)
  - Proton/Wine: everything above, via the Windows DLL
  
## Purpose
This is a menu template upon which you can build any mod menu you like \
It will find the injected process window, and attempt to hook with your specified API.
You are given a small ImGUI window with a title and exit button, along with a preset class for defining hack variables called MenuVars (in config.h)

## Usage
- Clone this repo
- Open library.cpp
- Find and change the rendering framework to your game's framework in library.cpp under DllMain E.G to: 
```C++
Rendering::SetFramework(Framework::DIRECTX9);
```
- Build and inject the DLL into your game, ensure you build for the correct platform, x86 for 32 bit games, x64 for 64 bit games
- Proceed to build your menu from this framework

## Linux

### Proton / Wine games
Proton games are Windows processes, so they use the Windows DLL. Cross-compile it with MinGW (`sudo pacman -S mingw-w64-gcc` on Arch, `mingw-w64` on Debian/Ubuntu):
```sh
cmake --preset mingw-x64 && cmake --build --preset mingw-x64   # build/mingw-x64/OMenu.dll
```
Use `mingw-x86` for 32-bit games. Set the framework in library.cpp to what the game uses (DirectX goes through DXVK/VKD3D, which is fine), then inject it with a Windows injector running inside the same Wine prefix.

### Native Linux games
```sh
cmake --preset linux-x64 && cmake --build --preset linux-x64   # build/linux-x64/libOMenu.so
```
Load it with `LD_PRELOAD`, e.g. as Steam launch options: `LD_PRELOAD=/path/to/libOMenu.so %command%`. There's no framework to set: it hooks GLX and EGL swaps automatically. Don't preload it into Proton games; use the DLL.

Linux limitations:
- OpenGL only. GLES and Vulkan games get no overlay.
- Input is polled from X11 (XWayland is fine), so the game still receives your clicks and keys while the menu is open. Pure-Wayland games get the overlay but no hotkeys or mouse input.
- END / EXIT hides the overlay for the rest of the session; a preloaded library can't be unloaded.

## Key information
- You'll find the menu style under menu->menu.cpp->CreateStyle()
- Menu rendering is done within menu->menu.cpp->Render(), this is where you'll notice the menu button being drawn. Your menu design will go here, move it to a seperate function if you wish.
- You can disable the debug console from within console->console.h, uncomment the DISABLE_LOGGING macro
- You will find custom logging macros in console.h including PRINT_CUSTOM and PRINT_CUSTOM_COLOR you can use these to print standard, or colored text using the following format:
```C++
PRINT_CUSTOM_COLOR(FOREGROUND_RED, "[YourMenuName]", "ESP Loaded");
PRINT_CUSTOM("[YourMenuName]", "ESP Loaded");
```
Which will print `[YourMenuName] ESP Loaded` in red, and your normal terminal background color respectively
- This was written in JetBrain's CLion using CMakeLists, which defines build instructions for the project. `CMakePresets.json` has presets for Linux and MinGW builds

## How it works

### DirectX
By creating a `placeholder` or `dummy` device and swapchain (specifically for DirectX10 and later) and linking them to the console window handle. the program obtains a reference to the `vTable` in order to access the API's function addresses. It then release these resources as they're not used after acquiring the vTable. From there, `MinHook` is used to hook into specified methods, meaning the `ImGUI` code can be rendered.

### OpenGL
Easier than DirectX that's for sure. OpenGL exports wglSwapBuffers, which is then hooked much like using DirectX. But instead of creating a dummy device, the program gets the module handle of opengl32, then gets the exported function address directly. \
From there, Minhook hooks in the custom method which handles the rendering of the menu.
## Author

👤 **MistaOmega**

* Github: [@MistaOmega](https://github.com/MistaOmega)
  
## Todo
- [x] DX10
- [x] DX11
- [x] OpenGL
- Vulkan

Note that the ImGUI implementation classes for these APIs are already in the project, but there is no hooking code for these functions yet.

## License
This project is licenced under the [MIT](https://choosealicense.com/licenses/mit/) licence.

## Dependencies
[ImGui](https://github.com/ocornut/imgui) - ocornut - Used to render the menu itself.  
[MinHook](https://github.com/TsudaKageyu/minhook) - TsudaKageyu - Used to hook the graphic API methods.  
[Vulkan SDK](https://vulkan.lunarg.com/) - for the Vulkan API.
