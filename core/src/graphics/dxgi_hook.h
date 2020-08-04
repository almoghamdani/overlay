#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>

typedef HRESULT(WINAPI *pFnD3D10CreateDeviceAndSwapChain)(
    IDXGIAdapter *pAdapter, D3D10_DRIVER_TYPE DriverType, HMODULE Software,
    UINT Flags, UINT SDKVersion, DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
    IDXGISwapChain **ppSwapChain, ID3D10Device **ppDevice);

typedef HRESULT (*pFnD3D11CreateDeviceAndSwapChain)(
    IDXGIAdapter *pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software,
    UINT Flags, const D3D_FEATURE_LEVEL *pFeatureLevels, UINT FeatureLevels,
    UINT SDKVersion, const DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
    IDXGISwapChain **ppSwapChain, ID3D11Device **ppDevice,
    D3D_FEATURE_LEVEL *pFeatureLevel, ID3D11DeviceContext **ppImmediateContext);

#define DXGI_SWAP_CHAIN_PRESENT_VTABLE_INDEX 8
#define DXGI_SWAP_CHAIN_RESIZE_BUFFERS_VTABLE_INDEX 13
#define DXGI_SWAP_CHAIN_RESIZE_TARGET_VTABLE_INDEX 14
#define DXGI_SWAP_CHAIN1_PRESENT1_VTABLE_INDEX 22

namespace overlay {
namespace core {
namespace graphics {

class DxgiHook {
 public:
  DxgiHook();

  bool Hook();

 private:
  bool graphics_initiated_;

  bool HookSwapChain(IDXGISwapChain *swap_chain);

  bool HookDirectx11(HWND window);
  bool HookDirectx10(HWND window);

  bool InitGraphics(IDXGISwapChain *swap_chain);

  void BeforePresent(IDXGISwapChain *swap_chain);

  HRESULT PresentHook(IDXGISwapChain *swap_chain, UINT sync_interval,
                      UINT flags);
  HRESULT ResizeBuffersHook(IDXGISwapChain *swap_chain, UINT buffer_count,
                            UINT width, UINT height, DXGI_FORMAT new_format,
                            UINT swap_chain_flags);
  HRESULT ResizeTargetHook(IDXGISwapChain *swap_chain,
                           const DXGI_MODE_DESC *new_target_parameters_ptr);
  HRESULT Present1Hook(IDXGISwapChain1 *swap_chain, UINT sync_interval,
                       UINT present_flags,
                       const DXGI_PRESENT_PARAMETERS *present_parameters_ptr);

  friend HRESULT DXGISwapChainPresentHook(IDXGISwapChain *swapChain,
                                          UINT syncInterval, UINT flags);
  friend HRESULT DXGISwapChainResizeBuffersHook(IDXGISwapChain *swapChain,
                                                UINT bufferCount, UINT width,
                                                UINT height,
                                                DXGI_FORMAT newFormat,
                                                UINT swapChainFlags);
  friend HRESULT DXGISwapChainResizeTargetHook(
      IDXGISwapChain *swapChain, const DXGI_MODE_DESC *pNewTargetParameters);
  friend HRESULT DXGISwapChain1Present1Hook(
      IDXGISwapChain1 *swapChain, UINT syncInterval, UINT presentFlags,
      const DXGI_PRESENT_PARAMETERS *pPresentParameters);
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay