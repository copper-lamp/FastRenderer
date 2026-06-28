#include "TouchInput.h"
#include "EGLHook.h"
#include "FloatOverlay.h"
#include "FileLog.h"
#include <imgui.h>
#include <cstdio>

// ═══════════════════════════════════════════
//  Android touch/mouse/key events → ImGui IO
//  Fixes: proper event ordering, tap vs drag,
//         long-press → right-click mapping
// ═══════════════════════════════════════════

// PreloaderInput touch action constants
constexpr int ACTION_DOWN   = 0;
constexpr int ACTION_UP     = 1;
constexpr int ACTION_MOVE   = 2;
constexpr int ACTION_CANCEL = 3;

// PreloaderInput key action constants
constexpr int KEY_ACTION_DOWN = 0;
constexpr int KEY_ACTION_UP   = 1;

// ─── Long-press → right-click emulation ───
static bool g_touchDown      = false;
static float g_downX         = 0.0f;
static float g_downY         = 0.0f;
static float g_downTime      = 0.0f;
static bool g_longPressFired = false;
static constexpr float LONG_PRESS_SEC = 0.6f;
static constexpr float LONG_PRESS_RADIUS = 20.0f;

bool handleTouchEvent(int action, int pointerId, float x, float y) {
    // Guard: ImGui must be initialized before processing touch events
    if (!IsImGuiInitialized()) return false;

    // ─── Layer 1: FloatOverlay coordinate-based hit test ───
    // This runs on the main thread synchronously and consumes
    // the touch event immediately if it's on the button area.
    // No ImGui IO dependency — pure coordinate math.
    {
        auto& overlay = GetFloatOverlay();
        auto result = overlay.handleTouch(action, x, y,
                                          GetScreenWidth(), GetScreenHeight());
        if (result == FloatOverlay::TouchResult::Tap) {
            overlay.toggleOpen();
            return true;  // consumed
        }
        if (result == FloatOverlay::TouchResult::Consumed) {
            return true;  // consumed (drag in progress)
        }
        // PassThrough → fall through to ImGui
    }

    // ─── Layer 2: Feed event to ImGui IO (for menu UI) ───
    ImGuiIO& io = ImGui::GetIO();
    auto now = ImGui::GetTime();

    switch (action) {
        case ACTION_DOWN: {
            g_touchDown      = true;
            g_downX          = x;
            g_downY          = y;
            g_downTime       = now;
            g_longPressFired = false;

            // Always set mouse position FIRST, then button
            io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
            io.AddMousePosEvent(x, y);
            io.AddMouseButtonEvent(0, true);  // left button down
            break;
        }

        case ACTION_MOVE: {
            // Update mouse position
            io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
            io.AddMousePosEvent(x, y);

            // Check for long-press → right-click
            if (g_touchDown && !g_longPressFired) {
                float dx = x - g_downX;
                float dy = y - g_downY;
                float dist = dx * dx + dy * dy;
                if (dist < LONG_PRESS_RADIUS * LONG_PRESS_RADIUS) {
                    float elapsed = now - g_downTime;
                    if (elapsed >= LONG_PRESS_SEC) {
                        // Fire right-click: release left, press right
                        io.AddMouseButtonEvent(0, false);
                        io.AddMouseButtonEvent(1, true);
                        g_longPressFired = true;
                    }
                }
            }
            break;
        }

        case ACTION_UP: {
            if (g_longPressFired) {
                // Release right button
                io.AddMouseButtonEvent(1, false);
            } else {
                // Normal tap → release left button
                io.AddMousePosEvent(x, y);
                io.AddMouseButtonEvent(0, false);
            }
            g_touchDown      = false;
            g_longPressFired = false;
            break;
        }

        case ACTION_CANCEL: {
            io.AddMouseButtonEvent(0, false);
            io.AddMouseButtonEvent(1, false);
            g_touchDown      = false;
            g_longPressFired = false;
            break;
        }

        default:
            break;
    }

    // If ImGui is interacting (hovering a button, dragging slider, etc.),
    // consume the event so Minecraft doesn't also process it.
    if (io.WantCaptureMouse) {
        return true;  // consumed by FR
    }
    return false;     // pass through to MC
}

// ─── Key event mapping (Android virtual keys) ───
static ImGuiKey mapAndroidKeyToImGui(int keyCode) {
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
        io.AddKeyEvent(key, isKeyDown);
    }

    return io.WantCaptureKeyboard;
}
