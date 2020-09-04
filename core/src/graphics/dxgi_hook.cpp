#include "dxgi_hook.h"

#include <iostream>
#include <loguru.hpp>

#include "core.h"
#include "utils/hook/vtable.h"

namespace overlay {
namespace core {
namespace graphics {

HRESULT STDMETHODCALLTYPE DXGISwapChainPresentHook(IDXGISwapChain *swapChain,
                                                   UINT syncInterval,
                                                   UINT flags) {
  return Core::Get()->get_graphics_manager()->get_dxgi_hook()->PresentHook(
      swapChain, syncInterval, flags);
}

HRESULT STDMETHODCALLTYPE DXGISwapChainResizeBuffersHook(
    IDXGISwapChain *swapChain, UINT bufferCount, UINT width, UINT height,
    DXGI_FORMAT newFormat, UINT swapChainFlags) {
  return Core::Get()
      ->get_graphics_manager()
      ->get_dxgi_hook()
      ->ResizeBuffersHook(swapChain, bufferCount, width, height, newFormat,
                          swapChainFlags);
}

HRESULT STDMETHODCALLTYPE DXGISwapChainResizeTargetHook(
    IDXGISwapChain *swapChain, const DXGI_MODE_DESC *pNewTargetParameters) {
  return Core::Get()->get_graphics_manager()->get_dxgi_hook()->ResizeTargetHook(
      swapChain, pNewTargetParameters);
}

HRESULT STDMETHODCALLTYPE DXGISwapChain1Present1Hook(
    IDXGISwapChain1 *swapChain, UINT syncInterval, UINT presentFlags,
    const DXGI_PRESENT_PARAMETERS *pPresentParameters) {
  return Core::Get()->get_graphics_manager()->get_dxgi_hook()->Present1Hook(
      swapChain, syncInterval, presentFlags, pPresentParameters);
}

DxgiHook::DxgiHook() : graphics_initiated_(false) {}

bool DxgiHook::Hook(HWND dummy_window) {
  bool hooked = false;

  // Try to hook DirectX 10
  if (HookDirectx10(dummy_window)) {
    LOG_F(INFO, "Hooked DirectX 10 successfully!");
    hooked = true;
  }

  // Try to hook DirectX 11
  if (HookDirectx11(dummy_window)) {
    LOG_F(INFO, "Hooked DirectX 11 successfully!");
    hooked = true;
  }

  return hooked;
}

void DxgiHook::Unhook() {
  present_hook_.Remove();
  resize_buffers_hook_.Remove();
  resize_target_hook_.Remove();
  present1_hook_.Remove();
  graphics_initiated_ = false;
}

bool DxgiHook::HookDirectx11(HWND window) {
  DXGI_SWAP_CHAIN_DESC desc = {0};

  IDXGISwapChain *swap_chain = nullptr;

  HMODULE directx11 = 0;
  pFnD3D11CreateDeviceAndSwapChain create_func = nullptr;

  bool hooked = false;

  // Try to get DirectX 11 module (will return only if application had loaded it
  // before)
  if (!(directx11 = GetModuleHandleW(L"d3d11.dll")) ||
      !(create_func = (pFnD3D11CreateDeviceAndSwapChain)GetProcAddress(
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
  if (FAILED(create_func(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
                         D3D11_CREATE_DEVICE_SINGLETHREADED, NULL, 0,
                         D3D11_SDK_VERSION, &desc, &swap_chain, NULL, NULL,
                         NULL))) {
    return false;
  }

  // Try to hook the swap-chain
  hooked = HookSwapChain(swap_chain);

  // Release swap-chain
  swap_chain->Release();

  return hooked;
}

bool DxgiHook::HookDirectx10(HWND window) {
  DXGI_SWAP_CHAIN_DESC desc = {0};

  IDXGISwapChain *swap_chain = nullptr;
  ID3D10Device *device = nullptr;

  HMODULE directx10 = 0;
  pFnD3D10CreateDeviceAndSwapChain create_func = nullptr;

  bool hooked = false;

  // Try to get DirectX 10 module (will return only if application had loaded it
  // before)
  if (!(directx10 = GetModuleHandleW(L"d3d10.dll")) ||
      !(create_func = (pFnD3D10CreateDeviceAndSwapChain)GetProcAddress(
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
  if (FAILED(create_func(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
                         D3D10_SDK_VERSION, &desc, &swap_chain, &device))) {
    return false;
  }

  // Try to hook the swap-chain
  hooked = HookSwapChain(swap_chain);

  // Release swap-chain and device
  swap_chain->Release();
  device->Release();

  return hooked;
}

bool DxgiHook::InitGraphics(IDXGISwapChain *swap_chain) {
  HWND injectWindow = Core::Get()->get_inject_window();

  DXGI_SWAP_CHAIN_DESC swap_desc;

  if (graphics_initiated_) {
    return true;
  }

  // Get the swap-chain's description
  if (FAILED(swap_chain->GetDesc(&swap_desc))) {
    return false;
  }

  // If the swap-chain's output window is the inject window, set the main
  // graphics window to it
  if ((injectWindow != NULL && swap_desc.OutputWindow == injectWindow) ||
      (injectWindow == NULL && swap_desc.OutputWindow != NULL)) {
    Core::Get()->set_graphics_window(swap_desc.OutputWindow);

    graphics_initiated_ = true;

    return true;
  }

  return false;
}

void DxgiHook::BeforePresent(IDXGISwapChain *swap_chain) {
  if (!graphics_initiated_) {
    InitGraphics(swap_chain);
  }

  Core::Get()->get_graphics_manager()->Render();
  Core::Get()->get_graphics_manager()->get_stats_calculator()->Frame();
}

bool DxgiHook::HookSwapChain(IDXGISwapChain *swap_chain) {
  void *swap_chain_present_func = utils::hook::Vtable::GetFunctionPointer(
      swap_chain, DXGI_SWAP_CHAIN_PRESENT_VTABLE_INDEX);
  void *swap_chain_resize_buffers_func =
      utils::hook::Vtable::GetFunctionPointer(
          swap_chain, DXGI_SWAP_CHAIN_RESIZE_BUFFERS_VTABLE_INDEX);
  void *swap_chain_resize_target_func = utils::hook::Vtable::GetFunctionPointer(
      swap_chain, DXGI_SWAP_CHAIN_RESIZE_TARGET_VTABLE_INDEX);

  IDXGISwapChain1 *swap_chain1 = nullptr;
  void *swap_chain1_present1_func = nullptr;

  // Hook present function
  if (!present_hook_.Install(swap_chain_present_func,
                             DXGISwapChainPresentHook)) {
    LOG_F(ERROR,
          "Unable to hook DXGISwapChainPresent function! Swap-chain: %p, "
          "Function: %p",
          swap_chain, swap_chain_present_func);
    return false;
  }

  // Hook resize buffers function
  if (!resize_buffers_hook_.Install(swap_chain_resize_buffers_func,
                                    DXGISwapChainResizeBuffersHook)) {
    LOG_F(ERROR,
          "Unable to hook DXGISwapChainResizeBuffers function! Swap-chain: %p, "
          "Function: %p",
          swap_chain, swap_chain_resize_buffers_func);
    return false;
  }

  // Hook resize target function
  if (!resize_target_hook_.Install(swap_chain_resize_target_func,
                                   DXGISwapChainResizeTargetHook)) {
    LOG_F(ERROR,
          "Unable to hook DXGISwapChainResizeTarget function! Swap-chain: %p, "
          "Function: %p",
          swap_chain, swap_chain_resize_target_func);
    return false;
  }

  // Check if the swap-chain has enhanced swap-chain
  swap_chain->QueryInterface<IDXGISwapChain1>(&swap_chain1);
  if (swap_chain1) {
    // Hook present function
    swap_chain1_present1_func = utils::hook::Vtable::GetFunctionPointer(
        swap_chain1, DXGI_SWAP_CHAIN1_PRESENT1_VTABLE_INDEX);
    if (!present1_hook_.Install(swap_chain1_present1_func,
                                DXGISwapChain1Present1Hook)) {
      LOG_F(ERROR,
            "Unable to hook DXGISwapChain1Present1 function! Swap-chain: %p, "
            "Function: %p",
            swap_chain1, swap_chain1_present1_func);
      return false;
    }
  }

  return true;
}

HRESULT DxgiHook::PresentHook(IDXGISwapChain *swap_chain, UINT sync_interval,
                              UINT flags) {
  HRESULT ret;

  // Call before present handler
  BeforePresent(swap_chain);

  // Call original function
  ret = present_hook_.get_trampoline().CallStdMethod<HRESULT>(
      swap_chain, sync_interval, flags);

  return ret;
}

HRESULT DxgiHook::ResizeBuffersHook(IDXGISwapChain *swap_chain,
                                    UINT buffer_count, UINT width, UINT height,
                                    DXGI_FORMAT new_format,
                                    UINT swap_chain_flags) {
  HRESULT ret;

  // Call original function
  ret = resize_buffers_hook_.get_trampoline().CallStdMethod<HRESULT>(
      swap_chain, buffer_count, width, height, new_format, swap_chain_flags);

  return ret;
}

HRESULT DxgiHook::ResizeTargetHook(
    IDXGISwapChain *swap_chain,
    const DXGI_MODE_DESC *new_target_parameters_ptr) {
  HRESULT ret;

  // Call original function
  ret = resize_target_hook_.get_trampoline().CallStdMethod<HRESULT>(
      swap_chain, new_target_parameters_ptr);

  return ret;
}

HRESULT DxgiHook::Present1Hook(
    IDXGISwapChain1 *swap_chain, UINT sync_interval, UINT present_flags,
    const DXGI_PRESENT_PARAMETERS *present_parameters_ptr) {
  HRESULT ret;

  // Call before present handler
  BeforePresent(swap_chain);

  // Call original function
  ret = present1_hook_.get_trampoline().CallStdMethod<HRESULT>(
      swap_chain, sync_interval, present_flags, present_parameters_ptr);

  return ret;
}

}  // namespace graphics
}  // namespace core
}  // namespace overlay
