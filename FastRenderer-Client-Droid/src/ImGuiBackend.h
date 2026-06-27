#pragma once
#include <string>

// ─── ImGui OpenGL ES 3.0 backend ───

bool InitImGuiFromGLContext();        // Called once at first eglSwapBuffers hook
bool BeginImGuiFrame(int width, int height, float deltaTime);
void EndImGuiFrame();
void DestroyImGuiBackend();