#ifndef OVERLAY_WINDOW_EVENTS_H
#define OVERLAY_WINDOW_EVENTS_H
#include <cstdint>
#include <cwchar>

namespace overlay {
namespace helper {

enum class WindowEventType { KeyboardInput };

struct WindowEvent {
  WindowEvent(WindowEventType type) : type(type) {}

  WindowEventType type;
};

struct WindowKeyboardInputEvent : public WindowEvent {
  enum class InputType { KeyDown, Char, KeyUp };
  enum class KeyCode {
    KeyBackspace = 0x8,
    KeyTab = 0x9,
    KeyClear = 0xC,
    KeyEnter = 0xD,
    KeyShift = 0x10,
    KeyControl = 0x11,
    KeyAlt = 0x12,
    KeyPause = 0x13,
    KeyCapslock = 0x14,
    KeyEscape = 0x1B,
    KeySpace = 0x20,
    KeyPageUp = 0x21,
    KeyPageDown = 0x22,
    KeyEnd = 0x23,
    KeyHome = 0x24,
    KeyLeft = 0x25,
    KeyUp = 0x26,
    KeyRight = 0x27,
    KeyDown = 0x28,
    KeyPrintScreen = 0x2C,
    KeyInsert = 0x2D,
    KeyDelete = 0x2E,
    Key0 = 0x30,
    Key1,
    Key2,
    Key3,
    Key4,
    Key5,
    Key6,
    Key7,
    Key8,
    Key9,
    KeyA = 0x41,
    KeyB,
    KeyC,
    KeyD,
    KeyE,
    KeyF,
    KeyG,
    KeyH,
    KeyI,
    KeyJ,
    KeyK,
    KeyL,
    KeyM,
    KeyN,
    KeyO,
    KeyP,
    KeyQ,
    KeyR,
    KeyS,
    KeyT,
    KeyU,
    KeyV,
    KeyW,
    KeyX,
    KeyY,
    KeyZ,
    KeyLeftSuper = 0x5B,
    KeyRightSuper = 0x5C,
    KeyMenu = 0x5D,
    KeySleep = 0x5F,
    KeyNumpad0 = 0x60,
    KeyNumpad1,
    KeyNumpad2,
    KeyNumpad3,
    KeyNumpad4,
    KeyNumpad5,
    KeyNumpad6,
    KeyNumpad7,
    KeyNumpad8,
    KeyNumpad9,
    KeyNumpadMultiply = 0x6A,
    KeyNumpadAdd = 0x6B,
    KeyNumpadSubtract = 0x6D,
    KeyNumpadDecimal = 0x6E,
    KeyNumpadDivide = 0x6F,
    KeyF1 = 0x70,
    KeyF2,
    KeyF3,
    KeyF4,
    KeyF5,
    KeyF6,
    KeyF7,
    KeyF8,
    KeyF9,
    KeyF10,
    KeyF11,
    KeyF12,
    KeyF13,
    KeyF14,
    KeyF15,
    KeyF16,
    KeyF17,
    KeyF18,
    KeyF19,
    KeyF20,
    KeyF21,
    KeyF22,
    KeyF23,
    KeyF24,
    KeyNumlcok = 0x90,
    KeyScrolllock = 0x91,
    KeyVolumeMute = 0xAD,
    KeyVolumeDown = 0xAE,
    KeyVolumeUp = 0xAF,
    KeyMediaNextTrack = 0xB0,
    KeyMediaPrevTrack = 0xB1,
    KeyMediaStop = 0xB2,
    KeyMediaPlayPause = 0xB3,
    KeyOem1 = 0xBA,
    KeyOemPlus = 0xBB,
    KeyOemComma = 0xBC,
    KeyOemMinus = 0xBD,
    KeyOemPeriod = 0xBE,
    KeyOem2 = 0xBF,
    KeyOem3 = 0xC0,
    KeyOem4 = 0xDB,
    KeyOem5 = 0xDC,
    KeyOem6 = 0xDD,
    KeyOem7 = 0xDE,
    KeyOem8 = 0xDF,
    KeyOem102 = 0xE2
  };

  WindowKeyboardInputEvent(InputType type, KeyCode key_code)
      : WindowEvent(WindowEventType::KeyboardInput),
        type(type),
        key_code(key_code) {}

  WindowKeyboardInputEvent(InputType type, wchar_t character)
      : WindowEvent(WindowEventType::KeyboardInput),
        type(type),
        character(character) {}

  InputType type;
  union {
    KeyCode key_code;
    wchar_t character;
  };
};

}  // namespace helper
}  // namespace overlay

#endif