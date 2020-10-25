#include "dx9_hook.h"

#include <loguru.hpp>

#include "core.h"
#include "dx9_renderer.h"
#include "utils/hook/vtable.h"

namespace overlay {
namespace core {
namespace graphics {

HRESULT STDMETHODCALLTYPE D3D9DevicePresentHook(IDirect3DDevice9 *device,
                                                const RECT *sourceRect,
                                                const RECT *destRect,
                                                HWND destWindowOverride,
                                                const RGNDATA *dirtyRegion) {
  return Core::Get()->get_graphics_manager()->get_dx9_hook()->DevicePresentHook(
      device, sourceRect, destRect, destWindowOverride, dirtyRegion);
}

HRESULT STDMETHODCALLTYPE
D3D9SwapChainPresentHook(IDirect3DSwapChain9 *swapChain, const RECT *sourceRect,
                         const RECT *destRect, HWND destWindowOverride,
                         const RGNDATA *dirtyRegion, DWORD flags) {
  return Core::Get()
      ->get_graphics_manager()
      ->get_dx9_hook()
      ->SwapChainPresentHook(swapChain, sourceRect, destRect,
                             destWindowOverride, dirtyRegion, flags);
}

HRESULT STDMETHODCALLTYPE D3D9DeviceResetHook(
    IDirect3DDevice9 *device, D3DPRESENT_PARAMETERS *presentationParameters) {
  return Core::Get()->get_graphics_manager()->get_dx9_hook()->DeviceResetHook(
      device, presentationParameters);
}

HRESULT STDMETHODCALLTYPE D3D9ExDevicePresentExHook(
    IDirect3DDevice9Ex *device, const RECT *sourceRect, const RECT *destRect,
    HWND destWindowOverride, const RGNDATA *dirtyRegion, DWORD flags) {
  return Core::Get()
      ->get_graphics_manager()
      ->get_dx9_hook()
      ->DevicePresentExHook(device, sourceRect, destRect, destWindowOverride,
                            dirtyRegion, flags);
}

HRESULT STDMETHODCALLTYPE D3D9ExDeviceResetExHook(
    IDirect3DDevice9Ex *device, D3DPRESENT_PARAMETERS *presentationParameters,
    D3DDISPLAYMODEEX *fullscreenDisplayMode) {
  return Core::Get()->get_graphics_manager()->get_dx9_hook()->DeviceResetExHook(
      device, presentationParameters, fullscreenDisplayMode);
}

Dx9Hook::Dx9Hook() : graphics_initiated_(false) {}

bool Dx9Hook::Hook(HWND dummy_window) {
  bool hooked = false;

  HMODULE directx9 = 0;
  pFnDirect3DCreate9 d3d9_create_func = nullptr;
  pFnDirect3DCreate9Ex d3d9ex_create_func = nullptr;

  IDirect3D9 *d3d9 = nullptr;
  IDirect3DDevice9 *d3d9_device = nullptr;

  IDirect3D9Ex *d3d9ex = nullptr;
  IDirect3DDevice9Ex *d3d9ex_device = nullptr;

  D3DPRESENT_PARAMETERS d3d9_present_parameters = {0};

  // Try to get DirectX 9 module (will return only if application had loaded it
  // before)
  if (!(directx9 = GetModuleHandleW(L"d3d9.dll")) ||
      !((d3d9ex_create_func = (pFnDirect3DCreate9Ex)GetProcAddress(
             directx9, "Direct3DCreate9Ex")) ||
        (d3d9_create_func = (pFnDirect3DCreate9)GetProcAddress(
             directx9, "Direct3DCreate9")))) {
    return false;
  }

  // Create a DirectX 9Ex instance if available
  if (d3d9ex_create_func) {
    d3d9ex_create_func(D3D_SDK_VERSION, &d3d9ex);
  }

  // If DirectX 9Ex not available, try create DirectX 9 instance
  if (!d3d9ex) {
    d3d9 = d3d9_create_func(D3D_SDK_VERSION);
  }

  if (!d3d9 && !d3d9ex) {
    return false;
  }

  // Set dummy parameters for creating a DirectX 9 device
  d3d9_present_parameters.Windowed = TRUE;
  d3d9_present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
  d3d9_present_parameters.BackBufferFormat = D3DFMT_UNKNOWN;
  d3d9_present_parameters.MultiSampleType = D3DMULTISAMPLE_NONE;
  d3d9_present_parameters.BackBufferWidth = 2;
  d3d9_present_parameters.BackBufferHeight = 2;
  d3d9_present_parameters.BackBufferCount = 1;
  d3d9_present_parameters.hDeviceWindow = dummy_window;
  d3d9_present_parameters.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

  // Create DirectX 9Ex device
  if (d3d9ex && SUCCEEDED(d3d9ex->CreateDeviceEx(
                    D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, dummy_window,
                    D3DCREATE_HARDWARE_VERTEXPROCESSING,
                    &d3d9_present_parameters, NULL, &d3d9ex_device))) {
    if (HookD3d9ex(d3d9ex_device)) {
      hooked = HookD3d9(d3d9ex_device);
    }

    d3d9ex_device->Release();
  } else if (d3d9 && SUCCEEDED(d3d9->CreateDevice(
                         D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, dummy_window,
                         D3DCREATE_HARDWARE_VERTEXPROCESSING,
                         &d3d9_present_parameters, &d3d9_device))) {
    hooked = HookD3d9(d3d9_device);

    d3d9_device->Release();
  }

  if (d3d9ex) {
    d3d9ex->Release();
  }

  if (d3d9) {
    d3d9->Release();
  }

  return hooked;
}

void Dx9Hook::Unhook() {
  if (graphics_initiated_) {
    Core::Get()->get_graphics_manager()->set_renderer(nullptr);
  }

  present_hook_.Remove();
  swap_chain_present_hook_.Remove();
  reset_hook_.Remove();
  present_ex_hook_.Remove();
  reset_ex_hook_.Remove();

  graphics_initiated_ = false;
}

bool Dx9Hook::HookD3d9(IDirect3DDevice9 *device) {
  void *present_func = utils::hook::Vtable::GetFunctionPointer(
      device, D3D9_DEVICE_PRESENT_VTABLE_INDEX);
  void *reset_func = utils::hook::Vtable::GetFunctionPointer(
      device, D3D9_DEVICE_RESET_VTABLE_INDEX);

  IDirect3DSwapChain9 *swap_chain = nullptr;
  void *swap_chain_present_func = nullptr;

  // Get swap-chain
  if (FAILED(device->GetSwapChain(0, &swap_chain))) {
    return false;
  }
  swap_chain_present_func = utils::hook::Vtable::GetFunctionPointer(
      swap_chain, D3D9_SWAP_CHAIN_PRESENT_VTABLE_INDEX);

  // Hook present function
  if (!present_hook_.Install(present_func, D3D9DevicePresentHook)) {
    DLOG_F(ERROR,
           "Unable to hook D3D9DevicePresent function! Device: %p, "
           "Function: %p",
           device, present_func);
    return false;
  }

  // Hook swap-chain present function
  if (!swap_chain_present_hook_.Install(swap_chain_present_func,
                                        D3D9SwapChainPresentHook)) {
    DLOG_F(ERROR,
           "Unable to hook D3D9SwapChainPresent function! Swap-chain: %p, "
           "Function: %p",
           swap_chain, swap_chain_present_func);
    return false;
  }

  // Hook reset function
  if (!reset_hook_.Install(reset_func, D3D9DeviceResetHook)) {
    DLOG_F(ERROR,
           "Unable to hook D3D9DeviceReset function! Device: %p, "
           "Function: %p",
           device, reset_func);
    return false;
  }

  swap_chain->Release();

  return true;
}

bool Dx9Hook::InitGraphics(IDirect3DDevice9 *device) {
  HWND injectWindow = Core::Get()->get_inject_window();

  IDirect3DSwapChain9 *swap_chain = nullptr;
  D3DPRESENT_PARAMETERS present_parameters;

  std::unique_ptr<Dx9Renderer> renderer = nullptr;

  if (graphics_initiated_) {
    return true;
  }

  // Get the swap-chain and the present parameters
  if (FAILED(device->GetSwapChain(0, &swap_chain)) ||
      FAILED(swap_chain->GetPresentParameters(&present_parameters))) {
    return false;
  }
  swap_chain->Release();

  // If the swap-chain's output window is the inject window, set the main
  // graphics window to it
  if ((injectWindow != NULL &&
       present_parameters.hDeviceWindow == injectWindow) ||
      (injectWindow == NULL && present_parameters.hDeviceWindow != NULL)) {
    // Create DirectX 9 Renderer
    renderer = std::make_unique<Dx9Renderer>(device);
    if (!renderer->Init()) {
      return false;
    }
    Core::Get()->get_graphics_manager()->set_renderer(
        std::move(std::unique_ptr<IGraphicsRenderer>(
            static_cast<IGraphicsRenderer *>(renderer.release()))));

    Core::Get()->set_graphics_window(present_parameters.hDeviceWindow);

    graphics_initiated_ = true;

    return true;
  }

  return false;
}

void Dx9Hook::BeforePresent(IDirect3DDevice9 *device) {
  if (!graphics_initiated_) {
    InitGraphics(device);
  }

  Core::Get()->get_graphics_manager()->Render();
  Core::Get()->get_graphics_manager()->get_stats_calculator()->Frame();
}

void Dx9Hook::OnReset(IDirect3DDevice9 *device,
                      D3DPRESENT_PARAMETERS *presentation_parameters) {
  if (graphics_initiated_) {
    Core::Get()->get_graphics_manager()->OnResize(
        presentation_parameters->BackBufferWidth,
        presentation_parameters->BackBufferHeight,
        !presentation_parameters->Windowed);
  }
}

bool Dx9Hook::HookD3d9ex(IDirect3DDevice9Ex *device) {
  void *present_ex_func = utils::hook::Vtable::GetFunctionPointer(
      device, D3D9EX_DEVICE_PRESENTEX_VTABLE_INDEX);
  void *reset_ex_func = utils::hook::Vtable::GetFunctionPointer(
      device, D3D9EX_DEVICE_RESETEX_VTABLE_INDEX);

  // Hook present function
  if (!present_ex_hook_.Install(present_ex_func, D3D9ExDevicePresentExHook)) {
    DLOG_F(ERROR,
           "Unable to hook D3D9ExDevicePresentEx function! Device: %p, "
           "Function: %p",
           device, present_ex_func);
    return false;
  }

  // Hook reset function
  if (!reset_ex_hook_.Install(reset_ex_func, D3D9ExDeviceResetExHook)) {
    DLOG_F(ERROR,
           "Unable to hook D3D9ExDeviceResetEx function! Device: %p, "
           "Function: %p",
           device, reset_ex_func);
    return false;
  }

  return true;
}

HRESULT Dx9Hook::DevicePresentHook(IDirect3DDevice9 *device,
                                   const RECT *source_rect,
                                   const RECT *dest_rect,
                                   HWND dest_window_override,
                                   const RGNDATA *dirty_region) {
  HRESULT ret;

  BeforePresent(device);

  ret = present_hook_.get_trampoline().CallStdMethod<HRESULT>(
      device, source_rect, dest_rect, dest_window_override, dirty_region);

  return ret;
}

HRESULT Dx9Hook::SwapChainPresentHook(IDirect3DSwapChain9 *swap_chain,
                                      const RECT *source_rect,
                                      const RECT *dest_rect,
                                      HWND dest_window_override,
                                      const RGNDATA *dirty_region,
                                      DWORD flags) {
  HRESULT ret;

  IDirect3DDevice9 *device = nullptr;

  if (SUCCEEDED(swap_chain->GetDevice(&device))) {
    BeforePresent(device);
  }

  // Release device
  device->Release();

  ret = swap_chain_present_hook_.get_trampoline().CallStdMethod<HRESULT>(
      swap_chain, source_rect, dest_rect, dest_window_override, dirty_region,
      flags);

  return ret;
}

HRESULT Dx9Hook::DeviceResetHook(
    IDirect3DDevice9 *device, D3DPRESENT_PARAMETERS *presentation_parameters) {
  HRESULT ret;

  OnReset(device, presentation_parameters);

  ret = reset_hook_.get_trampoline().CallStdMethod<HRESULT>(
      device, presentation_parameters);

  return ret;
}

HRESULT Dx9Hook::DevicePresentExHook(IDirect3DDevice9Ex *device,
                                     const RECT *source_rect,
                                     const RECT *dest_rect,
                                     HWND dest_window_override,
                                     const RGNDATA *dirty_region, DWORD flags) {
  HRESULT ret;

  BeforePresent(device);

  ret = present_ex_hook_.get_trampoline().CallStdMethod<HRESULT>(
      device, source_rect, dest_rect, dest_window_override, dirty_region,
      flags);

  return ret;
}

HRESULT Dx9Hook::DeviceResetExHook(
    IDirect3DDevice9Ex *device, D3DPRESENT_PARAMETERS *presentation_parameters,
    D3DDISPLAYMODEEX *fullscreen_display_mode) {
  HRESULT ret;

  OnReset(device, presentation_parameters);

  ret = reset_ex_hook_.get_trampoline().CallStdMethod<HRESULT>(
      device, presentation_parameters, fullscreen_display_mode);

  return ret;
}

}  // namespace graphics
}  // namespace core
}  // namespace overlay