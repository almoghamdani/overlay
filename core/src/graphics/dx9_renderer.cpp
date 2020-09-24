#include "dx9_renderer.h"

#include <loguru.hpp>

namespace overlay {
namespace core {
namespace graphics {

Dx9Renderer::Dx9Renderer(IDirect3DDevice9 *device)
    : device_(device),
      d3dx9_module_(LoadLibraryA("d3dx9_43.dll")),
      sprite_drawer_(nullptr) {}

Dx9Renderer::~Dx9Renderer() {
  if (d3dx9_module_) {
    FreeLibrary(d3dx9_module_);
  }
}

bool Dx9Renderer::Init() {
  pFnD3DXCreateSprite create_sprite_func = nullptr;

  if (!d3dx9_module_ ||
      !(create_sprite_func = (pFnD3DXCreateSprite)GetProcAddress(
            d3dx9_module_, "D3DXCreateSprite"))) {
    LOG_F(ERROR, "Unable to load D3DX dll!");
    return false;
  }

  // Create sprite drawer
  if (FAILED(create_sprite_func(device_, &sprite_drawer_))) {
    LOG_F(ERROR, "Unable to create D3DX sprite drawer!");
    return false;
  }

  LOG_F(INFO, "DirectX 9 renderer initiated with device %p.", device_);

  return true;
}

IDirect3DTexture9 *Dx9Renderer::CreateTexture(Rect rect,
                                              std::vector<uint8_t> &buffer) {
  IDirect3DTexture9 *sprite_texture = nullptr;
  D3DLOCKED_RECT texture_rect;

  // Verify buffer size
  if (buffer.size() != (rect.width * rect.height * sizeof(uint32_t))) {
    return nullptr;
  }

  // Create the texture
  if (FAILED(device_->CreateTexture((UINT)rect.width, (UINT)rect.height, 1,
                                    D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8,
                                    D3DPOOL_DEFAULT, &sprite_texture, 0))) {
    return nullptr;
  }

  // Lock the entire texture as a rectangle
  if (FAILED(sprite_texture->LockRect(0, &texture_rect, 0, D3DLOCK_DISCARD))) {
    return nullptr;
  }

  // Copy the buffer data to the rect data
  for (uint64_t line = 0; line < rect.height; line++) {
    memcpy((uint8_t *)texture_rect.pBits + line * texture_rect.Pitch,
           (uint32_t *)buffer.data() + line * rect.width,
           rect.width * sizeof(uint32_t));
  }

  // Unlock the texture data
  if (FAILED(sprite_texture->UnlockRect(0))) {
    return nullptr;
  }

  return sprite_texture;
}

void Dx9Renderer::RenderSprites(
    const std::vector<std::shared_ptr<Sprite>> &sprites) {
  // Release old textures
  ReleaseTextures();

  // Start rendering
  device_->BeginScene();
  sprite_drawer_->Begin(D3DXSPRITE_ALPHABLEND);

  // Draw sprites
  for (const auto &sprite : sprites) {
    DrawSprite(sprite);
  }

  // End rendering
  sprite_drawer_->End();
  device_->EndScene();
}

void Dx9Renderer::OnResize() {
  ReleaseTextures();

  if (sprite_drawer_ != nullptr) {
    sprite_drawer_->OnLostDevice();
    sprite_drawer_->OnResetDevice();
  }
}

void Dx9Renderer::DrawSprite(const std::shared_ptr<Sprite> &sprite) {
  if (sprite == nullptr) {
    return;
  }

  D3DXVECTOR3 sprite_pos((FLOAT)sprite->rect.x, (FLOAT)sprite->rect.y, 0);
  RECT sprite_rect = {0, 0, (LONG)sprite->rect.width,
                      (LONG)sprite->rect.height};

  // Create texture if needed
  if (sprite->texture == nullptr) {
    sprite->texture = CreateTexture(sprite->rect, sprite->buffer);
  }

  // Draw the sprite
  if (sprite->texture != nullptr) {
    sprite_drawer_->Draw((IDirect3DTexture9 *)sprite->texture, &sprite_rect,
                         NULL, &sprite_pos, 0xffffffff);
  }
}

}  // namespace graphics
}  // namespace core
}  // namespace overlay