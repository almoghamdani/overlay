#include "dxgi_hook.h"

#include <iostream>
#include <loguru.hpp>

#include "../utils/hook/manager.h"
#include "../utils/hook/vtable.h"

HRESULT DXGISwapChainPresentHook(IDXGISwapChain *swapChain, UINT syncInterval, UINT flags) {
    HRESULT ret;

    // Call original function
    ret = overlay::hook::manager::get_trampoline("DXGISwapChainPresent")
              .call<HRESULT>(swapChain, syncInterval, flags);

    return ret;
}

HRESULT DXGISwapChainResizeBuffersHook(IDXGISwapChain *swapChain, UINT bufferCount, UINT width,
                                       UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags) {
    HRESULT ret;

    // Call original function
    ret = overlay::hook::manager::get_trampoline("DXGISwapChainResizeBuffers")
              .call<HRESULT>(swapChain, bufferCount, width, height, newFormat, swapChainFlags);

    return ret;
}

HRESULT DXGISwapChainResizeTargetHook(IDXGISwapChain *swapChain,
                                      const DXGI_MODE_DESC *pNewTargetParameters) {
    HRESULT ret;

    // Call original function
    ret = overlay::hook::manager::get_trampoline("DXGISwapChainResizeTarget")
              .call<HRESULT>(swapChain, pNewTargetParameters);

    return ret;
}

HRESULT DXGISwapChain1Present1Hook(IDXGISwapChain1 *swapChain, UINT syncInterval, UINT presentFlags,
                                   const DXGI_PRESENT_PARAMETERS *pPresentParameters) {
    HRESULT ret;

    // Call original function
    ret = overlay::hook::manager::get_trampoline("DXGISwapChain1Present1")
              .call<HRESULT>(swapChain, syncInterval, presentFlags, pPresentParameters);

    return ret;
}

bool overlay::core::graphics::dxgi_hook::hook() {
    bool hooked = false;

    // Create a dummy window to create the directx swap chains for
    HWND dummyWindow = CreateWindowA("STATIC", "OVERLAY_DUMMY_WINDOW", WS_POPUP, 0, 0, 2, 2,
                                     HWND_MESSAGE, NULL, NULL, NULL);
    if (!dummyWindow) {
        return false;
    }

    // Try to hook DirectX 10
    if (hook_directx10(dummyWindow)) {
        LOG_F(INFO, "Hooked DirectX 10 successfully!");
        hooked = true;
    }

    // Try to hook DirectX 11
    if (hook_directx11(dummyWindow)) {
        LOG_F(INFO, "Hooked DirectX 11 successfully!");
        hooked = true;
    }

    // Destroy the dummy window
    DestroyWindow(dummyWindow);

    return hooked;
}

bool overlay::core::graphics::dxgi_hook::hook_directx11(HWND window) {
    DXGI_SWAP_CHAIN_DESC desc = {0};

    IDXGISwapChain *swapChain = nullptr;

    HMODULE directx11 = 0;
    pFnD3D11CreateDeviceAndSwapChain createFunc = nullptr;

    bool hooked = false;

    // Try to get DirectX 11 module (will return only if application had loaded it before)
    if (!(directx11 = GetModuleHandleW(L"d3d11.dll")) ||
        !(createFunc = (pFnD3D11CreateDeviceAndSwapChain)GetProcAddress(
              directx11, "D3D11CreateDeviceAndSwapChain"))) {
        return false;
    }

    // Set dummy properties for the dummy swap-chain
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferDesc.Width = 2;
    desc.BufferDesc.Height = 2;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 1;
    desc.OutputWindow = window;
    desc.Windowed = true;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    desc.Flags = 0;

    // Try to create the dummy swap chain
    if (FAILED(createFunc(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_SINGLETHREADED,
                          NULL, 0, D3D11_SDK_VERSION, &desc, &swapChain, NULL, NULL, NULL))) {
        return false;
    }

    // Try to hook the swap-chain
    hooked = hook_swap_chain(swapChain);

    // Release swap-chain
    swapChain->Release();

    return hooked;
}

bool overlay::core::graphics::dxgi_hook::hook_directx10(HWND window) {
    DXGI_SWAP_CHAIN_DESC desc = {0};

    IDXGISwapChain *swapChain = nullptr;
    ID3D10Device *device = nullptr;

    HMODULE directx10 = 0;
    pFnD3D10CreateDeviceAndSwapChain createFunc = nullptr;

    bool hooked = false;

    // Try to get DirectX 10 module (will return only if application had loaded it before)
    if (!(directx10 = GetModuleHandleW(L"d3d10.dll")) ||
        !(createFunc = (pFnD3D10CreateDeviceAndSwapChain)GetProcAddress(
              directx10, "D3D10CreateDeviceAndSwapChain"))) {
        return false;
    }

    // Set dummy properties for the dummy swap-chain
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferDesc.Width = 2;
    desc.BufferDesc.Height = 2;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 1;
    desc.OutputWindow = window;
    desc.Windowed = true;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    desc.Flags = 0;

    // Try to create the dummy swap chain
    if (FAILED(createFunc(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &desc,
                          &swapChain, &device))) {
        return false;
    }

    // Try to hook the swap-chain
    hooked = hook_swap_chain(swapChain);

    // Release swap-chain and device
    swapChain->Release();
    device->Release();

    return hooked;
}

bool overlay::core::graphics::dxgi_hook::hook_swap_chain(IDXGISwapChain *swapChain) {
    void *swapChainPresentFunc =
        hook::vtable::get_function_pointer(swapChain, DXGI_SWAP_CHAIN_PRESENT_VTABLE_INDEX);
    void *swapChainResizeBuffersFunc =
        hook::vtable::get_function_pointer(swapChain, DXGI_SWAP_CHAIN_RESIZE_BUFFERS_VTABLE_INDEX);
    void *swapChainResizeTargetFunc =
        hook::vtable::get_function_pointer(swapChain, DXGI_SWAP_CHAIN_RESIZE_TARGET_VTABLE_INDEX);

    IDXGISwapChain1 *swapChain1 = nullptr;
    void *swapChain1Present1Func = nullptr;

    // Hook present function
    if (!hook::manager::install_hook("DXGISwapChainPresent", swapChainPresentFunc,
                                     DXGISwapChainPresentHook)) {
        LOG_F(ERROR, "Unable to hook DXGISwapChainPresent function! Swap-chain: %p, Function: %p",
              swapChain, swapChainPresentFunc);
        return false;
    }

    // Hook resize buffers function
    if (!hook::manager::install_hook("DXGISwapChainResizeBuffers", swapChainResizeBuffersFunc,
                                     DXGISwapChainResizeBuffersHook)) {
        LOG_F(ERROR,
              "Unable to hook DXGISwapChainResizeBuffers function! Swap-chain: %p, Function: %p",
              swapChain, swapChainResizeBuffersFunc);
        return false;
    }

    // Hook resize target function
    if (!hook::manager::install_hook("DXGISwapChainResizeTarget", swapChainResizeTargetFunc,
                                     DXGISwapChainResizeTargetHook)) {
        LOG_F(ERROR,
              "Unable to hook DXGISwapChainResizeTarget function! Swap-chain: %p, Function: %p",
              swapChain, swapChainResizeTargetFunc);
        return false;
    }

    // Check if the swap-chain has enhanced swap-chain
    swapChain->QueryInterface<IDXGISwapChain1>(&swapChain1);
    if (swapChain1) {
        // Hook present function
        swapChain1Present1Func =
            hook::vtable::get_function_pointer(swapChain1, DXGI_SWAP_CHAIN1_PRESENT1_VTABLE_INDEX);
        if (!hook::manager::install_hook("DXGISwapChain1Present1", swapChain1Present1Func,
                                         DXGISwapChain1Present1Hook)) {
            LOG_F(ERROR,
                  "Unable to hook DXGISwapChain1Present1 function! Swap-chain: %p, Function: %p",
                  swapChain1, swapChain1Present1Func);
            return false;
        }
    }

    return true;
}