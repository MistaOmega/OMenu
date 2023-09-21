//
// Created by jackn on 15/09/2023.
//
#include "hook_dx9.h"
#include "../../console/console.h"
#include "../detours.h"
#include "../../menu/menu.h"

static IDirect3D9 *g_pD3D = nullptr;
static IDirect3DDevice9 *g_pd3dDevice = nullptr;

static void CleanupDeviceD3D9() {
    if (g_pD3D) {
        g_pD3D->Release();
        g_pD3D = nullptr;
    }
    if (g_pd3dDevice) {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }
}

static void Render(IDirect3DDevice9 *pDevice);

// Reset hook
static std::add_pointer_t<HRESULT WINAPI(IDirect3DDevice9 *, D3DPRESENT_PARAMETERS *)> oReset;

static HRESULT WINAPI HookedReset(IDirect3DDevice9 *pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters) {
    return oReset(pDevice, pPresentationParameters);
}

// Present hook
static std::add_pointer_t<HRESULT WINAPI(IDirect3DDevice9 *, const RECT *, const RECT *, HWND,
                                         const RGNDATA *)> oPresent;

static HRESULT WINAPI HookedPresent(IDirect3DDevice9 *pDevice, const RECT *pSourceRect, const RECT *pDestRect,
                                    HWND hDestWindowOverride, const RGNDATA *pDirtyRegion) {
    Render(pDevice);
    return oPresent(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

// PresentEx hook
static std::add_pointer_t<HRESULT WINAPI(IDirect3DDevice9 *, const RECT *, const RECT *, HWND, const RGNDATA *,
                                         DWORD)> oPresentEx;

static HRESULT WINAPI HookedPresentEx(IDirect3DDevice9 *pDevice, const RECT *pSourceRect, const RECT *pDestRect,
                                      HWND hDestWindowOverride, const RGNDATA *pDirtyRegion, DWORD dwFlags) {
    Render(pDevice);
    return oPresentEx(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
}

//ResetEx hook
static std::add_pointer_t<HRESULT WINAPI(IDirect3DDevice9 *, D3DPRESENT_PARAMETERS *, D3DDISPLAYMODEEX *)> oResetEx;

static HRESULT WINAPI hkResetEx(IDirect3DDevice9 *pDevice,
                                D3DPRESENT_PARAMETERS *pPresentationParameters,
                                D3DDISPLAYMODEEX *pFullscreenDisplayMode) {
    return oResetEx(pDevice, pPresentationParameters, pFullscreenDisplayMode);
}

static void render(IDirect3DDevice9 *pDevice) {
    // Fixes menu being too 'white' on games likes 'CS:S'.
    DWORD SRGBWriteEnable;
    pDevice->GetRenderState(D3DRS_SRGBWRITEENABLE, &SRGBWriteEnable);
    pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, false);
    if (pDevice->BeginScene() == D3D_OK) {
        pDevice->EndScene();
    }
    pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, SRGBWriteEnable);
}


static bool CreateD3D9RenderDevice(HWND hWnd) {
    g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!g_pD3D) {
        PRINT_ERROR_COLOR(FOREGROUND_RED | FOREGROUND_INTENSITY,
                          "[CRITICAL] Failed to create DirectX9 Rendering Device.")
        return false;
    }

    D3DPRESENT_PARAMETERS d3dpp = {};
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.BackBufferCount = 1;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    D3DCAPS9 caps;
    HRESULT hr = g_pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
    if (hr != D3D_OK) {
        PRINT_ERROR_COLOR(FOREGROUND_RED, "DirectX9: Error on GetDeviceCaps. [%lu]\n", hr);
        return false;
    }

    DWORD behaviorFlags = 0;
    if (caps.VertexProcessingCaps != 0) {
        behaviorFlags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
    } else {
        behaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    }

    hr = g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, behaviorFlags, &d3dpp, &g_pd3dDevice);
    if (hr != D3D_OK) {
        PRINT_ERROR("DirectX9: Error on CreateDevice. [%lu]\n", hr);
        return false;
    }

    return true;

}

static void Render(IDirect3DDevice9 *pDevice) {
    if (!ImGui::GetIO().BackendRendererUserData) {
        ImGui_ImplDX9_Init(pDevice);
    }
    ImGui::GetIO().MouseDrawCursor = Menu::showMenu;

    if (Hooks::hooked && ImGui::GetCurrentContext()) {

        DWORD SRGBWriteEnable;
        pDevice->GetRenderState(D3DRS_SRGBWRITEENABLE, &SRGBWriteEnable);
        //disable SRGB Gamma Correction for menu. Fixes issues with menu rendering with too much brightness on some games
        pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, false);

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        Menu::Render();

        ImGui::EndFrame();
        if (pDevice->BeginScene() == D3D_OK) {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            pDevice->EndScene();
        }

        // Reset SRGB Gamma Correction to default
        pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, SRGBWriteEnable);
    }
}

namespace DirectX9 {
    /**
     * Initialize DirectX, set up hooks for DirectX functions, and perform resource management.
     *
     * This function initializes the DirectX environment, sets up hooks for specific DirectX functions,
     * and manages resources associated with DirectX. It is typically used for DirectX hooking within
     * a larger application.
     *
     * @param hWnd Handle to the window associated with the DirectX device.
     */
    void Hook(HWND hWnd) {
        // Attempt to create a Direct3D device
        if (!CreateD3D9RenderDevice(GetConsoleWindow())) {
            PRINT_ERROR_COLOR(FOREGROUND_RED | FOREGROUND_INTENSITY,
                              "[CRITICAL] Failed to create DirectX9 Rendering Device.");
            CleanupDeviceD3D9();
            return;
        }

        // display information about the DirectX interface and device to console
        PRINT_COLOR(FOREGROUND_GREEN | FOREGROUND_INTENSITY,
                    "DirectX9: Dummy device creation successful. Pointer addresses: ");
        PRINT("g_pD3D: 0x%p", g_pD3D);
        PRINT("gPd3DDevice: 0x%p", g_pd3dDevice);

        if (g_pd3dDevice) {
            // Hook into DirectX functions
            Menu::CreateImGuiForWindow(hWnd);
            // Retrieve the virtual function table (vtable) of the device
            void **pVTable = *reinterpret_cast<void ***>(g_pd3dDevice);

            // Get function pointers for various DirectX functions
            void *fnReset = pVTable[16];
            void *fnPresent = pVTable[17];
            void *fnPresentEx = pVTable[121];
            void *fnResetEx = pVTable[132];

            // Perform any necessary cleanup or resource release for DirectX objects
            CleanupDeviceD3D9();

            //verify status, enable hooks and print to console
            if (MH_CreateHook(reinterpret_cast<void **>(fnReset), reinterpret_cast<void **>(&HookedReset),
                              reinterpret_cast<void **>(&oReset)) == MH_OK) {
                MH_EnableHook(fnReset);
                PRINT("Hooked IDirect3DDevice9::Reset successfully");
            }
            if (MH_CreateHook(reinterpret_cast<void **>(fnResetEx), reinterpret_cast<void **>(&hkResetEx),
                              reinterpret_cast<void **>(&oResetEx)) == MH_OK) {
                MH_EnableHook(fnResetEx);
                PRINT("Hooked IDirect3DDevice9Ex::ResetEx successfully");
            }
            if (MH_CreateHook(reinterpret_cast<void **>(fnPresent), reinterpret_cast<void **>(&HookedPresent),
                              reinterpret_cast<void **>(&oPresent)) == MH_OK) {
                MH_EnableHook(fnPresent);
                PRINT("Hooked IDirect3DDevice9::Present successfully");
            }
            if (MH_CreateHook(reinterpret_cast<void **>(fnPresentEx), reinterpret_cast<void **>(&HookedPresentEx),
                              reinterpret_cast<void **>(&oPresentEx)) == MH_OK) {
                MH_EnableHook(fnPresentEx);
                PRINT("Hooked IDirect3DDevice9Ex::PresentEx successfully");
            }
            PRINT_COLOR(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "Initialisation complete.\n");
        }
    }

    void Unhook() {
        if (ImGui::GetCurrentContext()) {
            if (ImGui::GetIO().BackendRendererUserData)
                ImGui_ImplDX9_Shutdown();

            if (ImGui::GetIO().BackendPlatformUserData)
                ImGui_ImplWin32_Shutdown();

            ImGui::DestroyContext();
        }
    }
}