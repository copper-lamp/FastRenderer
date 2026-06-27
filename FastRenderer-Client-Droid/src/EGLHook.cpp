#include "EGLHook.h"
#include "ImGuiBackend.h"
#include <pl/memory/Hook.h>   // GlossHook: pl_hook, pl_unhook

// ═══════════════════════════════════════════
//  Hook eglSwapBuffers
// ═══════════════════════════════════════════

using EglSwapBuffers_t = EGLBoolean (*)(EGLDisplay dpy, EGLSurface surface);
static EglSwapBuffers_t oEglSwapBuffers = nullptr;

static bool g_imguiInitialized = false;
static int  g_screenWidth  = 0;
static int  g_screenHeight = 0;
static float g_deltaTime   = 0.016f;  // ~60fps default

// Called by TouchInput to update these values
void UpdateImGuiDisplay(int width, int height) {
    g_screenWidth  = width;
    g_screenHeight = height;
}

// Called by TouchInput to set delta time
void SetImGuiDeltaTime(float dt) {
    g_deltaTime = dt;
}

static EGLBoolean hkEglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (!g_imguiInitialized) {
        // First call: initialize ImGui with this GL context
        g_imguiInitialized = InitImGuiFromGLContext();
        if (g_imguiInitialized) {
            // Get display size from EGL query
            EGLint w = 0, h = 0;
            eglQuerySurface(dpy, surface, EGL_WIDTH,  &w);
            eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
            g_screenWidth  = (int)w;
            g_screenHeight = (int)h;
        }
    }

    if (g_imguiInitialized && g_screenWidth > 0 && g_screenHeight > 0) {
        // Save MC GL state
        GlState savedState;
        SaveGlState(savedState);

        // Clear shader state that might interfere
        glUseProgram(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // ImGui frame
        if (BeginImGuiFrame(g_screenWidth, g_screenHeight, g_deltaTime)) {
            // Render all FastRenderer GUIs + floating menu
            onImGuiRender();

            EndImGuiFrame();
        }

        // Restore MC GL state
        RestoreGlState(savedState);
    }

    // Call original eglSwapBuffers
    return oEglSwapBuffers(dpy, surface);
}

bool HookEglSwapBuffers() {
    // Find eglSwapBuffers address in loaded libEGL.so
    void* target = (void*)eglSwapBuffers;
    if (!target) return false;

    int ret = pl_hook(
        (PLFuncPtr)target,
        (PLFuncPtr)hkEglSwapBuffers,
        (PLFuncPtr*)&oEglSwapBuffers,
        PL_HOOK_PRIORITY_NORMAL);

    return (ret == 0);
}

void UnhookEglSwapBuffers() {
    if (oEglSwapBuffers) {
        pl_unhook((PLFuncPtr)eglSwapBuffers, (PLFuncPtr)hkEglSwapBuffers);
        oEglSwapBuffers = nullptr;
    }
}