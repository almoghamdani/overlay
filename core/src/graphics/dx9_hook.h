#pragma once
#include <d3d9.h>

#include "graphics_hook.h"
#include "utils/hook/hook.h"

typedef IDirect3D9 *(WINAPI *pFnDirect3DCreate9)(UINT SDKVersion);
typedef HRESULT(WINAPI *pFnDirect3DCreate9Ex)(UINT, IDirect3D9Ex **);

namespace overlay {
namespace core {
namespace graphics {

class Dx9Hook : public IGraphicsHook {
 public:
  Dx9Hook();

  virtual bool Hook(HWND dummy_window);
  virtual void Unhook();

 private:
  bool graphics_initiated_;

  utils::hook::Hook present_hook_, present_ex_hook_, swap_chain_present_hook_,
      reset_hook_, reset_ex_hook_;

  bool HookD3d9(IDirect3DDevice9 *device);
  bool HookD3d9ex(IDirect3DDevice9Ex *device);

  bool InitGraphics(IDirect3DDevice9 *device);

  void BeforePresent(IDirect3DDevice9 *device);

  HRESULT DevicePresentHook(IDirect3DDevice9 *device, const RECT *source_rect,
                            const RECT *dest_rect, HWND dest_window_override,
                            const RGNDATA *dirty_region);
  HRESULT SwapChainPresentHook(IDirect3DSwapChain9 *swap_chain,
                               const RECT *source_rect, const RECT *dest_rect,
                               HWND dest_window_override,
                               const RGNDATA *dirty_region, DWORD flags);
  HRESULT DeviceResetHook(IDirect3DDevice9 *device,
                          D3DPRESENT_PARAMETERS *presentation_parameters);
  HRESULT DevicePresentExHook(IDirect3DDevice9Ex *device,
                              const RECT *source_rect, const RECT *dest_rect,
                              HWND dest_window_override,
                              const RGNDATA *dirty_region, DWORD flags);
  HRESULT DeviceResetExHook(IDirect3DDevice9Ex *device,
                            D3DPRESENT_PARAMETERS *presentation_parameters,
                            D3DDISPLAYMODEEX *fullscreen_display_mode);

  friend HRESULT STDMETHODCALLTYPE D3D9DevicePresentHook(
      IDirect3DDevice9 *device, const RECT *sourceRect, const RECT *destRect,
      HWND destWindowOverride, const RGNDATA *dirtyRegion);
  friend HRESULT STDMETHODCALLTYPE D3D9SwapChainPresentHook(
      IDirect3DSwapChain9 *swapChain, const RECT *sourceRect,
      const RECT *destRect, HWND destWindowOverride, const RGNDATA *dirtyRegion,
      DWORD flags);
  friend HRESULT STDMETHODCALLTYPE D3D9DeviceResetHook(
      IDirect3DDevice9 *device, D3DPRESENT_PARAMETERS *presentationParameters);
  friend HRESULT STDMETHODCALLTYPE D3D9ExDevicePresentExHook(
      IDirect3DDevice9Ex *device, const RECT *sourceRect, const RECT *destRect,
      HWND destWindowOverride, const RGNDATA *dirtyRegion, DWORD flags);
  friend HRESULT STDMETHODCALLTYPE D3D9ExDeviceResetExHook(
      IDirect3DDevice9Ex *device, D3DPRESENT_PARAMETERS *presentationParameters,
      D3DDISPLAYMODEEX *fullscreenDisplayMode);
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay