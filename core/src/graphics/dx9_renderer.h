#pragma once
#include <d3d9.h>
#include <d3dx9.h>

#include "graphics_renderer.h"

typedef HRESULT(WINAPI *pFnD3DXCreateSprite)(LPDIRECT3DDEVICE9 pDevice,
                                             LPD3DXSPRITE *ppSprite);

namespace overlay {
namespace core {
namespace graphics {

class Dx9Renderer : public IGraphicsRenderer {
 public:
  Dx9Renderer(IDirect3DDevice9 *device);
  ~Dx9Renderer();

  virtual bool Init();
  virtual void RenderSprites(const std::vector<Sprite> &sprites);

 private:
  IDirect3DDevice9 *device_;

  HMODULE d3dx9_module_;

  ID3DXSprite *sprite_drawer_;

  void DrawSprite(const Sprite &sprite);
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay