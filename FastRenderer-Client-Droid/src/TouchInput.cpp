#include "TouchInput.h"
#include <imgui.h>
#include <cstdio>

// ═══════════════════════════════════════════
//  Android touch/mouse/key events → ImGui IO
// ═══════════════════════════════════════════

// PreloaderInput touch action constants
constexpr int ACTION_DOWN   = 0;
constexpr int ACTION_UP     = 1;
constexpr int ACTION_MOVE   = 2;
constexpr int ACTION_CANCEL = 3;

// PreloaderInput key action constants
constexpr int KEY_ACTION_DOWN = 0;
constexpr int KEY_ACTION_UP   = 1;

bool handleTouchEvent(int action, int pointerId, float x, float y) {
    ImGuiIO& io = ImGui::GetIO();

    switch (action) {
        case ACTION_DOWN:
            io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
            io.AddMouseButtonEvent(0, true);   // left click pressed
            io.AddMousePosEvent((float)x, (float)y);
            break;

        case ACTION_UP:
            io.AddMouseButtonEvent(0, false);  // left click released
            break;

        case ACTION_MOVE:
            io.AddMousePosEvent((float)x, (float)y);
            break;

        case ACTION_CANCEL:
            io.AddMouseButtonEvent(0, false);  // release all
            break;

        default:
            break;
    }

    // If ImGui is interacting (e.g. dragging a slider, clicking a button),
    // consume the event so it doesn't pass through to Minecraft.
    if (io.WantCaptureMouse) {
        return true;  // consumed by FR
    }
    return false;     // pass through to MC
}

// ─── Key event mapping (Android virtual keys) ───
// Android key codes from KeyEvent.KEYCODE_*
// Only used if the system keyboard or external keyboard is connected

static ImGuiKey mapAndroidKeyToImGui(int keyCode) {
    // Common mappings
    switch (keyCode) {
        case 4:   return ImGuiKey_Escape;        // KEYCODE_BACK
        case 66:  return ImGuiKey_Enter;         // KEYCODE_ENTER
        case 67:  return ImGuiKey_Delete;        // KEYCODE_DEL
        case 112: return ImGuiKey_Delete;        // KEYCODE_FORWARD_DEL
        case 21:  return ImGuiKey_LeftArrow;     // KEYCODE_DPAD_LEFT
        case 22:  return ImGuiKey_RightArrow;    // KEYCODE_DPAD_RIGHT
        case 19:  return ImGuiKey_UpArrow;       // KEYCODE_DPAD_UP
        case 20:  return ImGuiKey_DownArrow;     // KEYCODE_DPAD_DOWN
        case 122: return ImGuiKey_Home;          // KEYCODE_MOVE_HOME
        case 123: return ImGuiKey_End;           // KEYCODE_MOVE_END
        case 59:  return ImGuiKey_LeftShift;     // KEYCODE_SHIFT_LEFT
        default:  return ImGuiKey_None;
    }
}

bool handleKeyEvent(int keyCode, unsigned int /*unicodeChar*/, bool isKeyDown) {
    ImGuiIO& io = ImGui::GetIO();

    ImGuiKey key = mapAndroidKeyToImGui(keyCode);
    if (key != ImGuiKey_None) {
        if (isKeyDown) {
            io.AddKeyEvent(key, true);
        } else {
            io.AddKeyEvent(key, false);
        }
    }

    return io.WantCaptureKeyboard;
}