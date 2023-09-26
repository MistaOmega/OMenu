//
// Created by jackn on 25/09/2023.
//

#include "hook_dx11.h"
#include "../../console/console.h"
#include "../../utils/utils.h"
#include "../../menu/menu.h"
#include "../detours.h"

static ID3D11Device *gPd3DDevice = nullptr;
static ID3D11DeviceContext *gPd3DDeviceContext = nullptr;
static ID3D11RenderTargetView *gPd3DRenderTarget = nullptr;
static IDXGISwapChain *gPSwapChain = nullptr;

static bool CreateD3D11RenderDevice(HWND hWnd) {
    // Create the D3DDevice
    DXGI_SWAP_CHAIN_DESC swpDesc = {};
    swpDesc.Windowed = TRUE;
    swpDesc.SampleDesc.Count = 1;
    swpDesc.OutputWindow = hWnd;
    swpDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swpDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swpDesc.BufferCount = 2;

    const D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_0,
    };
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_NULL, nullptr, 0, featureLevels, 2,
                                               D3D11_SDK_VERSION, &swpDesc, &gPSwapChain, &gPd3DDevice, nullptr,
                                               nullptr);
    if (hr != S_OK) {
        PRINT_ERROR_COLOR(FOREGROUND_RED | FOREGROUND_INTENSITY,
                          "[CRITICAL] Unable to create D3D11 Device and Swap Chain. [val: %lu]\n", hr)
        return false;
    }

    return true;
}

static void CreateRenderTarget(IDXGISwapChain *pSwapChain) {
    ID3D11Texture2D *pBackBuffer = nullptr;
    pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer) {
        DXGI_SWAP_CHAIN_DESC swpDesc;
        pSwapChain->GetDesc(&swpDesc);
        D3D11_RENDER_TARGET_VIEW_DESC targDesc = {};
        targDesc.Format = static_cast<DXGI_FORMAT>(Utils::ConvertToNonSRGBFormat(swpDesc.BufferDesc.Format));
        targDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

        gPd3DDevice->CreateRenderTargetView(pBackBuffer, &targDesc, &gPd3DRenderTarget);
        pBackBuffer->Release();
    }
}

static void CleanupRenderTarget() {
    if (gPd3DRenderTarget) {
        gPd3DRenderTarget->Release();
        gPd3DRenderTarget = nullptr;
    }
}

static void CleanupD3D11Device() {
    CleanupRenderTarget();

    if (gPSwapChain) {
        gPSwapChain->Release();
        gPSwapChain = nullptr;
    }
    if (gPd3DDeviceContext) {
        gPd3DDeviceContext->Release();
        gPd3DDeviceContext = nullptr;
    }
    if (gPd3DDevice) {
        gPd3DDevice->Release();
        gPd3DDevice = nullptr;
    }

}

static void RenderImGui_DX11(IDXGISwapChain *pSwapChain) {
    if (!ImGui::GetIO().BackendRendererUserData) {
        if (SUCCEEDED(pSwapChain->GetDevice(IID_PPV_ARGS(&gPd3DDevice)))) {
            gPd3DDevice->GetImmediateContext(&gPd3DDeviceContext);
            ImGui_ImplDX11_Init(gPd3DDevice, gPd3DDeviceContext);
        }
    }
    ImGui::GetIO().MouseDrawCursor = Menu::showMenu;
    if (Hooks::Hooked) {
        if (!gPd3DRenderTarget) {
            CreateRenderTarget(pSwapChain);
        }

        if (ImGui::GetCurrentContext() && gPd3DRenderTarget) {
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            Menu::Render();

            ImGui::Render();

            gPd3DDeviceContext->OMSetRenderTargets(1, &gPd3DRenderTarget, nullptr);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        }
    }
}


static std::add_pointer_t<HRESULT WINAPI(IDXGISwapChain *, UINT, UINT)> oPresent;

static HRESULT WINAPI hkPresent(IDXGISwapChain *pSwapChain,
                                UINT SyncInterval,
                                UINT Flags) {
    RenderImGui_DX11(pSwapChain);

    return oPresent(pSwapChain, SyncInterval, Flags);
}

static std::add_pointer_t<HRESULT WINAPI(IDXGISwapChain *, UINT, UINT, const DXGI_PRESENT_PARAMETERS *)> oPresent1;

static HRESULT WINAPI hkPresent1(IDXGISwapChain *pSwapChain,
                                 UINT SyncInterval,
                                 UINT PresentFlags,
                                 const DXGI_PRESENT_PARAMETERS *pPresentParameters) {
    RenderImGui_DX11(pSwapChain);

    return oPresent1(pSwapChain, SyncInterval, PresentFlags, pPresentParameters);
}

static std::add_pointer_t<HRESULT WINAPI(IDXGISwapChain *, UINT, UINT, UINT, DXGI_FORMAT, UINT)> oResizeBuffers;

static HRESULT WINAPI hkResizeBuffers(IDXGISwapChain *pSwapChain,
                                      UINT BufferCount,
                                      UINT Width,
                                      UINT Height,
                                      DXGI_FORMAT NewFormat,
                                      UINT SwapChainFlags) {
    CleanupRenderTarget();

    return oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

static std::add_pointer_t<HRESULT WINAPI(IDXGISwapChain *, UINT, UINT, UINT, DXGI_FORMAT, UINT, const UINT *,
                                         IUnknown *const *)> oResizeBuffers1;

static HRESULT WINAPI hkResizeBuffers1(IDXGISwapChain *pSwapChain,
                                       UINT BufferCount,
                                       UINT Width,
                                       UINT Height,
                                       DXGI_FORMAT NewFormat,
                                       UINT SwapChainFlags,
                                       const UINT *pCreationNodeMask,
                                       IUnknown *const *ppPresentQueue) {
    CleanupRenderTarget();

    return oResizeBuffers1(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags, pCreationNodeMask,
                           ppPresentQueue);
}

static std::add_pointer_t<HRESULT WINAPI(IDXGIFactory *, IUnknown *, DXGI_SWAP_CHAIN_DESC *,
                                         IDXGISwapChain **)> oCreateSwapChain;

static HRESULT WINAPI hkCreateSwapChain(IDXGIFactory *pFactory,
                                        IUnknown *pDevice,
                                        DXGI_SWAP_CHAIN_DESC *pDesc,
                                        IDXGISwapChain **ppSwapChain) {
    CleanupRenderTarget();

    return oCreateSwapChain(pFactory, pDevice, pDesc, ppSwapChain);
}

static std::add_pointer_t<HRESULT WINAPI(IDXGIFactory *, IUnknown *, HWND, const DXGI_SWAP_CHAIN_DESC1 *,
                                         const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *, IDXGIOutput *,
                                         IDXGISwapChain1 **)> oCreateSwapChainForHwnd;

static HRESULT WINAPI hkCreateSwapChainForHwnd(IDXGIFactory *pFactory,
                                               IUnknown *pDevice,
                                               HWND hWnd,
                                               const DXGI_SWAP_CHAIN_DESC1 *pDesc,
                                               const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc,
                                               IDXGIOutput *pRestrictToOutput,
                                               IDXGISwapChain1 **ppSwapChain) {
    CleanupRenderTarget();

    return oCreateSwapChainForHwnd(pFactory, pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);
}

static std::add_pointer_t<HRESULT WINAPI(IDXGIFactory *, IUnknown *, IUnknown *, const DXGI_SWAP_CHAIN_DESC1 *,
                                         IDXGIOutput *, IDXGISwapChain1 **)> oCreateSwapChainForCoreWindow;

static HRESULT WINAPI hkCreateSwapChainForCoreWindow(IDXGIFactory *pFactory,
                                                     IUnknown *pDevice,
                                                     IUnknown *pWindow,
                                                     const DXGI_SWAP_CHAIN_DESC1 *pDesc,
                                                     IDXGIOutput *pRestrictToOutput,
                                                     IDXGISwapChain1 **ppSwapChain) {
    CleanupRenderTarget();

    return oCreateSwapChainForCoreWindow(pFactory, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);
}

static std::add_pointer_t<HRESULT WINAPI(IDXGIFactory *, IUnknown *, const DXGI_SWAP_CHAIN_DESC1 *, IDXGIOutput *,
                                         IDXGISwapChain1 **)> oCreateSwapChainForComposition;

static HRESULT WINAPI hkCreateSwapChainForComposition(IDXGIFactory *pFactory,
                                                      IUnknown *pDevice,
                                                      const DXGI_SWAP_CHAIN_DESC1 *pDesc,
                                                      IDXGIOutput *pRestrictToOutput,
                                                      IDXGISwapChain1 **ppSwapChain) {
    CleanupRenderTarget();

    return oCreateSwapChainForComposition(pFactory, pDevice, pDesc, pRestrictToOutput, ppSwapChain);
}


void DirectX11::Hook(HWND hWnd) {
    if (!CreateD3D11RenderDevice(GetConsoleWindow( ))) {
        PRINT_ERROR_COLOR(FOREGROUND_RED | FOREGROUND_INTENSITY, "[CRITICAL] Failed to create DX11 Rendering Device.")
        return;
    }

    PRINT_COLOR(FOREGROUND_GREEN | FOREGROUND_INTENSITY,
                "DirectX11: Dummy device creation successful. Pointer addresses: ")
    PRINT("gPd3DDevice: 0x%p", gPd3DDevice);
    PRINT("gPSwapChain: 0x%p", gPSwapChain);

    if (gPd3DDevice) {
        Menu::CreateImGuiForWindow(hWnd);

        // Hook
        IDXGIDevice* pDXGIDevice = nullptr;
        gPd3DDevice->QueryInterface(IID_PPV_ARGS(&pDXGIDevice));

        IDXGIAdapter* pDXGIAdapter = nullptr;
        pDXGIDevice->GetAdapter(&pDXGIAdapter);

        IDXGIFactory* pIDXGIFactory = nullptr;
        pDXGIAdapter->GetParent(IID_PPV_ARGS(&pIDXGIFactory));

        if (!pIDXGIFactory) {
            PRINT("[Critical] Unable to get pIDXGIFactory. It's NULL.\n");
            return;
        }

        pIDXGIFactory->Release();
        pDXGIDevice->Release();
        pDXGIAdapter->Release();

        void** pVTable = *reinterpret_cast<void***>(gPSwapChain);
        void** pFactoryVTable = *reinterpret_cast<void***>(pIDXGIFactory);

        void* fnCreateSwapChain = pFactoryVTable[10];
        void* fnCreateSwapChainForHwnd = pFactoryVTable[15];
        void* fnCreateSwapChainForCoreWindow = pFactoryVTable[16];
        void* fnCreateSwapChainForComp = pFactoryVTable[24];

        void* fnPresent = pVTable[8];
        void* fnPresent1 = pVTable[22];

        void* fnResizeBuffers = pVTable[13];
        void* fnResizeBuffers1 = pVTable[39];

        CleanupD3D11Device();

        // Hook IDXGIFactory::CreateSwapChain
        if (MH_CreateHook(reinterpret_cast<void **>(fnCreateSwapChain), reinterpret_cast<void **>(&hkCreateSwapChain),
                          reinterpret_cast<void **>(&oCreateSwapChain)) == MH_OK) {
            PRINT("Hooked IDXGIFactory::CreateSwapChain successfully");
            MH_EnableHook(fnCreateSwapChain);
        }

        // Hook IDXGIFactory2::CreateSwapChainForHwnd
        if (MH_CreateHook(reinterpret_cast<void **>(fnCreateSwapChainForHwnd),
                          reinterpret_cast<void **>(&hkCreateSwapChainForHwnd),
                          reinterpret_cast<void **>(&oCreateSwapChainForHwnd)) == MH_OK) {
            PRINT("Hooked IDXGIFactory2::CreateSwapChainForHwnd successfully");
            MH_EnableHook(fnCreateSwapChainForHwnd);
        }

        // Hook IDXGIFactory2::CreateSwapChainForCoreWindow
        if (MH_CreateHook(reinterpret_cast<void **>(fnCreateSwapChainForCoreWindow),
                          reinterpret_cast<void **>(&hkCreateSwapChainForCoreWindow),
                          reinterpret_cast<void **>(&oCreateSwapChainForCoreWindow)) == MH_OK) {
            PRINT("Hooked IDXGIFactory2::CreateSwapChainForCoreWindow successfully");
            MH_EnableHook(fnCreateSwapChainForCoreWindow);
        }

        // Hook IDXGIFactory2::CreateSwapChainForComposition
        if (MH_CreateHook(reinterpret_cast<void **>(fnCreateSwapChainForComp),
                          reinterpret_cast<void **>(&hkCreateSwapChainForComposition),
                          reinterpret_cast<void **>(&oCreateSwapChainForComposition)) == MH_OK) {
            PRINT("Hooked IDXGIFactory2::CreateSwapChainForComposition successfully");
            MH_EnableHook(fnCreateSwapChainForComp);
        }

        // Hook IDXGISwapChain::ResizeBuffers
        if (MH_CreateHook(reinterpret_cast<void **>(fnResizeBuffers), reinterpret_cast<void **>(&hkResizeBuffers),
                          reinterpret_cast<void **>(&oResizeBuffers)) == MH_OK) {
            PRINT("Hooked IDXGISwapChain::ResizeBuffers successfully");
            MH_EnableHook(fnResizeBuffers);
        }

        // Hook IDXGISwapChain3::ResizeBuffers1
        if (MH_CreateHook(reinterpret_cast<void **>(fnResizeBuffers1), reinterpret_cast<void **>(&hkResizeBuffers1),
                          reinterpret_cast<void **>(&oResizeBuffers1)) == MH_OK) {
            PRINT("Hooked IDXGISwapChain3::ResizeBuffers1 successfully");
            MH_EnableHook(fnResizeBuffers1);
        }

        // Hook IDXGISwapChain::Present
        if (MH_CreateHook(reinterpret_cast<void **>(fnPresent), reinterpret_cast<void **>(&hkPresent),
                          reinterpret_cast<void **>(&oPresent)) == MH_OK) {
            PRINT("Hooked IDXGISwapChain::Present successfully");
            MH_EnableHook(fnPresent);
        }

        // Hook IDXGISwapChain1::Present1
        if (MH_CreateHook(reinterpret_cast<void **>(fnPresent1), reinterpret_cast<void **>(&hkPresent1),
                          reinterpret_cast<void **>(&oPresent1)) == MH_OK) {
            PRINT("Hooked IDXGISwapChain1::Present1 successfully");
            MH_EnableHook(fnPresent1);
        }
        PRINT_COLOR(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "Initialisation complete.\n")
    }

}

void DirectX11::Unhook() {
    if (ImGui::GetCurrentContext()) {
        if (ImGui::GetIO().BackendRendererUserData)
            ImGui_ImplDX11_Shutdown();

        if (ImGui::GetIO().BackendPlatformUserData)
            ImGui_ImplWin32_Shutdown();

        ImGui::DestroyContext();
    }

    CleanupD3D11Device();
}