//
// Created by jackn on 19/09/2023.
//
#include "hook_dx12.h"
#include "../../utils/utils.h"
#include "../../console/console.h"
#include "../../menu/menu.h"
#include "../detours.h"

static int const NUM_BACK_BUFFERS = 3;
static IDXGIFactory4 *gDxgiFactory = nullptr;
static ID3D12Device *gPd3DDevice = nullptr;
static IDXGISwapChain3 *gPSwapChain = nullptr;
static ID3D12DescriptorHeap *gPd3DRtvDescHeap = nullptr;
static ID3D12DescriptorHeap *gPd3DSrvDescHeap = nullptr;
static ID3D12CommandQueue *gPd3DCommandQueue = nullptr;
static ID3D12GraphicsCommandList *gPd3DCommandList = nullptr;
static ID3D12CommandAllocator *gCommandAllocators[NUM_BACK_BUFFERS] = {};
static ID3D12Resource *gMainRenderTargetResource[NUM_BACK_BUFFERS] = {};
static D3D12_CPU_DESCRIPTOR_HANDLE gMainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};

static bool CreateD3D12RenderDevice(HWND hwnd) {
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC1 chainDesc = {};
    chainDesc.BufferCount = NUM_BACK_BUFFERS;
    chainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    chainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    chainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    chainDesc.SampleDesc.Count = 1;
    chainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    // Create device
    D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_0;
    HRESULT device = D3D12CreateDevice(nullptr, level, IID_ID3D12Device, (void **) &gPd3DDevice);
    if (FAILED(device)) {
        //todo error messages for function calls
        return false;
    }

    D3D12_COMMAND_QUEUE_DESC desc = {};
    device = gPd3DDevice->CreateCommandQueue(&desc, IID_ID3D12CommandQueue, (void **) &gPd3DCommandQueue);
    if (FAILED(device)) {
        return false;
    }

    IDXGISwapChain1 *swapChain1 = nullptr;
    device = CreateDXGIFactory1(IID_IDXGIFactory4, (void **) &gDxgiFactory);
    if (FAILED(device)) {
        return false;
    }

    device = gDxgiFactory->CreateSwapChainForHwnd(gPd3DCommandQueue, hwnd, &chainDesc, nullptr, nullptr, &swapChain1);
    if (FAILED(device)) {
        return false;
    }

    device = swapChain1->QueryInterface(IID_IDXGISwapChain3, (void **) &gPSwapChain);
    if (FAILED(device)) {
        return false;
    }

    swapChain1->Release();

    return true;
}

static void CreateRenderTarget(IDXGISwapChain *pSwapChain) {
    for (UINT i = 0; i < NUM_BACK_BUFFERS; ++i) {
        ID3D12Resource *backBuffer = nullptr;
        HRESULT buffer = pSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
        if (SUCCEEDED(buffer)) {
            DXGI_SWAP_CHAIN_DESC chainDesc;
            pSwapChain->GetDesc(&chainDesc);

            D3D12_RENDER_TARGET_VIEW_DESC desc = {};
            desc.Format = static_cast<DXGI_FORMAT>(Utils::ConvertToNonSRGBFormat(chainDesc.BufferDesc.Format));
            desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

            gPd3DDevice->CreateRenderTargetView(backBuffer, &desc, gMainRenderTargetDescriptor[i]);

            gMainRenderTargetResource[i] = backBuffer;
        } else {
            // Handle error, e.g., log or cleanup and return
        }
    }
}

static void ReleaseRenderTargetResources() {
    for (auto &resource: gMainRenderTargetResource)
        if (resource) {
            resource->Release();
            resource = nullptr;
        }
}


static void CleanupDeviceD3D12() {
    ReleaseRenderTargetResources();

    if (gPSwapChain) {
        gPSwapChain->Release();
        gPSwapChain = nullptr;
    }

    for (auto &g_commandAllocator: gCommandAllocators) {
        if (g_commandAllocator) {
            g_commandAllocator->Release();
            g_commandAllocator = nullptr;
        }
    }

    if (gPd3DCommandList) {
        gPd3DCommandList->Release();
        gPd3DCommandList = nullptr;
    }
    if (gPd3DDevice) {
        gPd3DDevice->Release();
        gPd3DDevice = nullptr;
    }
    if (gDxgiFactory) {
        gDxgiFactory->Release();
        gDxgiFactory = nullptr;
    }

    auto ReleaseDescriptors = [](ID3D12DescriptorHeap *&heap) {
        if (heap) {
            heap->Release();
            heap = nullptr;
        }
    };

    ReleaseDescriptors(gPd3DRtvDescHeap);
    ReleaseDescriptors(gPd3DSrvDescHeap);
}

static void BuildRendererUserData(IDXGISwapChain3 *pSwapChain) {
    if (SUCCEEDED(pSwapChain->GetDevice(IID_PPV_ARGS(&gPd3DDevice)))) {
        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            desc.NodeMask = 1;
            desc.NumDescriptors = NUM_BACK_BUFFERS;
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            if (gPd3DDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&gPd3DRtvDescHeap)) != S_OK)
                return;

            SIZE_T rtvDescriptorSize = gPd3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = gPd3DRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
            for (auto &cpuHandle: gMainRenderTargetDescriptor) {
                cpuHandle = rtvHandle;
                rtvHandle.ptr += rtvDescriptorSize;
            }
        }

        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            desc.NumDescriptors = 1;
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            if (gPd3DDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&gPd3DSrvDescHeap)) != S_OK)
                return;
        }

        for (auto &gCommandAllocator: gCommandAllocators)
            if (gPd3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&gCommandAllocator)) !=
                S_OK)
                return;

        if (gPd3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, gCommandAllocators[0], nullptr,
                                           IID_PPV_ARGS(&gPd3DCommandList)) != S_OK ||
            gPd3DCommandList->Close() != S_OK)
            return;

        ImGui_ImplDX12_Init(gPd3DDevice, NUM_BACK_BUFFERS,
                            DXGI_FORMAT_R8G8B8A8_UNORM, gPd3DSrvDescHeap,
                            gPd3DSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
                            gPd3DSrvDescHeap->GetGPUDescriptorHandleForHeapStart());
    }
}

static void RenderImGui_DX12(IDXGISwapChain3 *pSwapChain) {
    if (!ImGui::GetIO().BackendRendererUserData) {
        BuildRendererUserData(pSwapChain);
    }
    ImGui::GetIO().MouseDrawCursor = Menu::showMenu;

    if (Hooks::Hooked) {
        if (!gMainRenderTargetResource[0]) {
            CreateRenderTarget(pSwapChain);
        }

        if (ImGui::GetCurrentContext() && gPd3DCommandQueue && gMainRenderTargetResource[0]) {
            ImGui_ImplDX12_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            Menu::Render();

            ImGui::Render();

            UINT backBufferIdx = pSwapChain->GetCurrentBackBufferIndex();
            ID3D12CommandAllocator *commandAllocator = gCommandAllocators[backBufferIdx];
            commandAllocator->Reset();

            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = gMainRenderTargetResource[backBufferIdx];
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            gPd3DCommandList->Reset(commandAllocator, nullptr);
            gPd3DCommandList->ResourceBarrier(1, &barrier);

            gPd3DCommandList->OMSetRenderTargets(1, &gMainRenderTargetDescriptor[backBufferIdx], FALSE, nullptr);
            gPd3DCommandList->SetDescriptorHeaps(1, &gPd3DSrvDescHeap);
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), gPd3DCommandList);
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            gPd3DCommandList->ResourceBarrier(1, &barrier);
            gPd3DCommandList->Close();

            gPd3DCommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList *const *>(&gPd3DCommandList));
        }
    }
}

// region hook functions
static std::add_pointer_t<HRESULT WINAPI(IDXGISwapChain *, UINT, UINT, UINT, DXGI_FORMAT, UINT)> oResizeBuffers;

static HRESULT WINAPI hkResizeBuffers(IDXGISwapChain *pSwapChain,
                                      UINT BufferCount,
                                      UINT Width,
                                      UINT Height,
                                      DXGI_FORMAT NewFormat,
                                      UINT SwapChainFlags) {
    ReleaseRenderTargetResources();

    return oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

static std::add_pointer_t<HRESULT WINAPI(IDXGISwapChain3 *, UINT, UINT)> oPresent;

static HRESULT WINAPI hkPresent(IDXGISwapChain3 *pSwapChain,
                                UINT SyncInterval,
                                UINT Flags) {
    RenderImGui_DX12(pSwapChain);

    return oPresent(pSwapChain, SyncInterval, Flags);
}

static std::add_pointer_t<HRESULT WINAPI(IDXGISwapChain3 *, UINT, UINT, const DXGI_PRESENT_PARAMETERS *)> oPresent1;

static HRESULT WINAPI hkPresent1(IDXGISwapChain3 *pSwapChain,
                                 UINT SyncInterval,
                                 UINT PresentFlags,
                                 const DXGI_PRESENT_PARAMETERS *pPresentParameters) {
    RenderImGui_DX12(pSwapChain);

    return oPresent1(pSwapChain, SyncInterval, PresentFlags, pPresentParameters);
}

static std::add_pointer_t<HRESULT WINAPI(IDXGISwapChain3 *, UINT, UINT, UINT, DXGI_FORMAT, UINT, const UINT *,
                                         IUnknown *const *)> oResizeBuffers1;

static HRESULT WINAPI hkResizeBuffers1(IDXGISwapChain3 *pSwapChain,
                                       UINT BufferCount,
                                       UINT Width,
                                       UINT Height,
                                       DXGI_FORMAT NewFormat,
                                       UINT SwapChainFlags,
                                       const UINT *pCreationNodeMask,
                                       IUnknown *const *ppPresentQueue) {
    ReleaseRenderTargetResources();

    return oResizeBuffers1(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags, pCreationNodeMask,
                           ppPresentQueue);
}

static std::add_pointer_t<HRESULT WINAPI(IDXGIFactory *, IUnknown *, DXGI_SWAP_CHAIN_DESC *,
                                         IDXGISwapChain **)> oCreateSwapChain;

static HRESULT WINAPI hkCreateSwapChain(IDXGIFactory *pFactory,
                                        IUnknown *pDevice,
                                        DXGI_SWAP_CHAIN_DESC *pDesc,
                                        IDXGISwapChain **ppSwapChain) {
    ReleaseRenderTargetResources();

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
    ReleaseRenderTargetResources();

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
    ReleaseRenderTargetResources();

    return oCreateSwapChainForCoreWindow(pFactory, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);
}

static std::add_pointer_t<HRESULT WINAPI(IDXGIFactory *, IUnknown *, const DXGI_SWAP_CHAIN_DESC1 *, IDXGIOutput *,
                                         IDXGISwapChain1 **)> oCreateSwapChainForComposition;

static HRESULT WINAPI hkCreateSwapChainForComposition(IDXGIFactory *pFactory,
                                                      IUnknown *pDevice,
                                                      const DXGI_SWAP_CHAIN_DESC1 *pDesc,
                                                      IDXGIOutput *pRestrictToOutput,
                                                      IDXGISwapChain1 **ppSwapChain) {
    ReleaseRenderTargetResources();

    return oCreateSwapChainForComposition(pFactory, pDevice, pDesc, pRestrictToOutput, ppSwapChain);
}

static std::add_pointer_t<void WINAPI(ID3D12CommandQueue *, UINT, ID3D12CommandList *)> oExecuteCommandLists;

static void WINAPI hkExecuteCommandLists(ID3D12CommandQueue *pCommandQueue,
                                         UINT NumCommandLists,
                                         ID3D12CommandList *ppCommandLists) {
    if (!gPd3DCommandQueue) {
        gPd3DCommandQueue = pCommandQueue;
    }

    return oExecuteCommandLists(pCommandQueue, NumCommandLists, ppCommandLists);
}
//endregion

//region hook initialisation and release
void DirectX12::Hook(HWND hwnd) {
    // Attempt to create a DirectX 12 rendering device
    if (!CreateD3D12RenderDevice(GetConsoleWindow())) {
        // If creation fails, print an error message and return
        PRINT_ERROR_COLOR(FOREGROUND_RED | FOREGROUND_INTENSITY, "[CRITICAL] Failed to create DX12 Rendering Device.")
        return;
    }

    // Print debug information about DirectX 12 objects
    PRINT_COLOR(FOREGROUND_GREEN | FOREGROUND_INTENSITY,
                "DirectX12: Dummy device creation successful. Pointer addresses: ")
    PRINT("gPd3DDevice: 0x%p", gPd3DDevice);
    PRINT("gDxgiFactory: 0x%p", gDxgiFactory);
    PRINT("gPd3DCommandQueue: 0x%p", gPd3DCommandQueue);
    PRINT("gPSwapChain: 0x%p", gPSwapChain);

    if (gPd3DDevice) {
        // Create ImGui user interface for the specified window
        Menu::CreateImGuiForWindow(hwnd);

        // Hook DirectX 12 functions
        // Extract function pointers from virtual tables
        void **pVTable = *reinterpret_cast<void ***>(gPSwapChain);
        void **pFactoryVTable = *reinterpret_cast<void ***>(gDxgiFactory);
        void **pCommandQueueVTable = *reinterpret_cast<void ***>(gPd3DCommandQueue);

        void *fnPresent = pVTable[8];
        void *fnResizeBuffers = pVTable[13];
        void *fnPresent1 = pVTable[22];
        void *fnResizeBuffers1 = pVTable[39];

        void *fnCreateSwapChain = pFactoryVTable[10];
        void *fnCreateSwapChainForHwnd = pFactoryVTable[15];
        void *fnCreateSwapChainForCoreWindow = pFactoryVTable[16];
        void *fnCreateSwapChainForComp = pFactoryVTable[24];

        void *fnExecuteCommandLists = pCommandQueueVTable[10];

        gPd3DCommandQueue->Release();
        gPd3DCommandQueue = nullptr;

        // Cleanup the DirectX 12 device
        CleanupDeviceD3D12();

        // Enable the created hooks for various DirectX 12 functions

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

        // Hook ID3D12CommandQueue::ExecuteCommandLists
        if (MH_CreateHook(reinterpret_cast<void **>(fnExecuteCommandLists),
                          reinterpret_cast<void **>(&hkExecuteCommandLists),
                          reinterpret_cast<void **>(&oExecuteCommandLists)) == MH_OK) {
            PRINT("Hooked ID3D12CommandQueue::ExecuteCommandLists successfully");
            MH_EnableHook(fnExecuteCommandLists);
        }
        PRINT_COLOR(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "Initialisation complete.\n");

    }
}

void DirectX12::Unhook() {
    // Check if there is a valid ImGui context
    if (ImGui::GetCurrentContext()) {
        // Check if the ImGui backend renderer has user data and shut it down if necessary
        if (ImGui::GetIO().BackendRendererUserData) {
            ImGui_ImplDX12_Shutdown();
        }

        // Check if the ImGui backend platform has user data and shut it down if necessary
        if (ImGui::GetIO().BackendPlatformUserData) {
            ImGui_ImplWin32_Shutdown();
        }

        // Destroy the ImGui context
        ImGui::DestroyContext();
    }

    // Clean up the DirectX 12 device
    CleanupDeviceD3D12();
}

//endregion
