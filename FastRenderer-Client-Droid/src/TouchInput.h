#pragma once

// ─── Touch/key input → ImGui IO bridge ───
// Called from PreloaderInput callbacks registered in ModEntry.cpp

bool handleTouchEvent(int action, int pointerId, float x, float y);
bool handleKeyEvent(int keyCode, unsigned int unicodeChar, bool isKeyDown);

// Update display size and delta time from EGLHook
void UpdateImGuiDisplay(int width, int height);
void SetImGuiDeltaTime(float dt);