//
// Created by jackn on 15/09/2023.
//

#ifndef OMENU_PCH_H
#define OMENU_PCH_H
#define WIN32_LEAN_AND_MEAN // Excludes Cryptography, DDE, RPC, Shell, and Windows Sockets

#include <windows.h>
#include <memory>
#include <vector>
#include <string_view>
#include <thread>
#include <string>
#include <unordered_map>
#include <iostream>
#include <TlHelp32.h>
#include <windef.h>
#include <d3d9.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3d11.h>
#include <d3d10_1.h>
#include <d3d10.h>
#include <dxgi1_2.h>

// Dependencies
#include "dependencies/imgui/imgui.h"
#include "dependencies/imgui/imgui_impl_opengl3.h"
#include "dependencies/imgui/imgui_impl_win32.h"
#include "dependencies/minhook/MinHook.h"
#include "dependencies/imgui/imgui_impl_dx9.h"
#include "dependencies/imgui/imgui_impl_dx10.h"
#include "dependencies/imgui/imgui_impl_dx11.h"
#include "dependencies/imgui/imgui_impl_dx12.h"

#endif //OMENU_PCH_H
