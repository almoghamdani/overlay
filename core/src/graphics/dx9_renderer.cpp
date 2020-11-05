#include "dx9_renderer.h"

#include <loguru/loguru.hpp>

namespace overlay {
namespace core {
namespace graphics {

Dx9Renderer::Dx9Renderer(IDirect3DDevice9 *device)
    : d3dx9_module_(LoadLibraryA("d3dx9_43.dll")),
      device_(device),
      sprite_drawer_(nullptr) {}

Dx9Renderer::~Dx9Renderer() {
  if (d3dx9_module_) {
    FreeLibrary(d3dx9_module_);
  }
}

bool Dx9Renderer::Init() {
  pFnD3DXCreateSprite create_sprite_func = nullptr;

  D3DPRESENT_PARAMETERS present_parameters;
  IDirect3DSwapChain9 *swap_chain = nullptr;

  // Try to get the swap chain
  if (FAILED(device_->GetSwapChain(0, &swap_chain))) {
    return false;
  }

  // Try to get the present parameters
  if (FAILED(swap_chain->GetPresentParameters(&present_parameters))) {
    swap_chain->Release();
    return false;
  }

  set_width(present_parameters.BackBufferWidth);
  set_height(present_parameters.BackBufferHeight);
  set_fullscreen(!present_parameters.Windowed);

  // Release swap chain
  swap_chain->Release();

  if (!d3dx9_module_ ||
      !(create_sprite_func = (pFnD3DXCreateSprite)GetProcAddress(
            d3dx9_module_, "D3DXCreateSprite"))) {
    DLOG_F(ERROR, "Unable to load D3DX dll!");
    return false;
  }

  // Create sprite drawer
  if (FAILED(create_sprite_func(device_, &sprite_drawer_))) {
    DLOG_F(ERROR, "Unable to create D3DX sprite drawer!");
    return false;
  }

  DLOG_F(INFO,
         "DirectX 9 renderer initiated with device %p. Window size: %dx%d, "
         "Fullscreen: %s.",
         device_, get_width(), get_height(),
         is_fullscreen() ? "True" : "False");

  return true;
}

IDirect3DTexture9 *Dx9Renderer::CreateTextureFromSolidColor(Rect rect,
                                                            Color color) {
  IDirect3DTexture9 *sprite_texture = nullptr;

  uint32_t rgba_color = ((uint32_t)0xff << 24) + ((uint32_t)color.red << 16) +
                        ((uint32_t)color.green << 8) + color.blue;
  std::string buffer;

  // Create buffer with the color
  buffer.resize(rect.width * rect.height * sizeof(uint32_t));
  for (size_t i = 0; i < rect.width * rect.height; i++) {
    *((uint32_t *)buffer.data() + i) = rgba_color;
  }

  // Create the texture
  if (FAILED(device_->CreateTexture((UINT)rect.width, (UINT)rect.height, 1,
                                    D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8,
                                    D3DPOOL_DEFAULT, &sprite_texture, 0))) {
    return nullptr;
  }

  // Copy the buffer data to the texture
  if (!CopyBufferToTexture(sprite_texture, rect, buffer)) {
    sprite_texture->Release();
    return nullptr;
  }

  return sprite_texture;
}

IDirect3DTexture9 *Dx9Renderer::CreateTextureFromBuffer(Rect rect,
                                                        std::string &buffer) {
  IDirect3DTexture9 *sprite_texture = nullptr;

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

  // Copy the buffer data to the texture
  if (!CopyBufferToTexture(sprite_texture, rect, buffer)) {
    sprite_texture->Release();
    return nullptr;
  }

  return sprite_texture;
}

bool Dx9Renderer::CopyBufferToTexture(IDirect3DTexture9 *texture, Rect rect,
                                      std::string &buffer) const {
  D3DLOCKED_RECT texture_rect;

  // Lock the entire texture as a rectangle
  if (FAILED(texture->LockRect(0, &texture_rect, 0, D3DLOCK_DISCARD))) {
    return false;
  }

  // Copy the buffer data to the rect data
  for (uint64_t line = 0; line < rect.height; line++) {
    memcpy((uint8_t *)texture_rect.pBits + line * texture_rect.Pitch,
           (uint32_t *)buffer.data() + line * rect.width,
           rect.width * sizeof(uint32_t));
  }

  // Unlock the texture data
  if (FAILED(texture->UnlockRect(0))) {
    return false;
  }

  return true;
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

void Dx9Renderer::OnResize(uint32_t width, uint32_t height, bool fullscreen) {
  set_width(width);
  set_height(height);
  set_fullscreen(fullscreen);
  DLOG_F(INFO, "Device Reset: Window size: %dx%d, Fullscreen: %s.", get_width(),
         get_height(), is_fullscreen() ? "True" : "False");

  // Release all textures
  ReleaseTextures();

  if (sprite_drawer_ != nullptr) {
    sprite_drawer_->OnLostDevice();
    sprite_drawer_->OnResetDevice();
  }
}

void Dx9Renderer::DrawSprite(const std::shared_ptr<Sprite> &sprite) {
  if (sprite == nullptr || sprite->opacity == 0) {
    return;
  }

  if (sprite->fill_target) {
    sprite->rect = TargetFillRect();
  }

  D3DXVECTOR3 sprite_pos((FLOAT)sprite->rect.x, (FLOAT)sprite->rect.y, 0);
  RECT sprite_rect = {0, 0, (LONG)sprite->rect.width,
                      (LONG)sprite->rect.height};

  // Create texture if needed
  if (sprite->texture == nullptr) {
    if (sprite->solid_color) {
      sprite->texture =
          CreateTextureFromSolidColor(sprite->rect, sprite->color);
    } else {
      sprite->texture = CreateTextureFromBuffer(sprite->rect, sprite->buffer);
    }
  } else if (!sprite->solid_color && sprite->buffer_updated &&
             (sprite->buffer.size() ==
              (sprite->rect.width * sprite->rect.height * sizeof(uint32_t)))) {
    CopyBufferToTexture((IDirect3DTexture9 *)sprite->texture, sprite->rect,
                        sprite->buffer);
    sprite->buffer_updated = false;
  }

  // Draw the sprite
  if (sprite->texture != nullptr) {
    sprite_drawer_->Draw(
        (IDirect3DTexture9 *)sprite->texture, &sprite_rect, NULL, &sprite_pos,
        0x00ffffff + ((uint32_t)(sprite->opacity * 0xff) << 24));
  }
}

}  // namespace graphics
}  // namespace core
}  // namespace overlay