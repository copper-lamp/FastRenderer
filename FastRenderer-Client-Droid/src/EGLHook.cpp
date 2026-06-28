#include "EGLHook.h"
#include "ImGuiBackend.h"
#include "FloatOverlay.h"
#include "FileLog.h"
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

// ─── FloatOverlay — ImGui-rendered floating button ───
static FloatOverlay g_floatOverlay;

FloatOverlay& GetFloatOverlay() {
    return g_floatOverlay;
}

int GetScreenWidth()  { return g_screenWidth; }
int GetScreenHeight() { return g_screenHeight; }

// Called by TouchInput to update these values
void UpdateImGuiDisplay(int width, int height) {
    g_screenWidth  = width;
    g_screenHeight = height;
}

bool IsImGuiInitialized() {
    return g_imguiInitialized;
}

// Called by TouchInput to set delta time
void SetImGuiDeltaTime(float dt) {
    g_deltaTime = dt;
}

static EGLBoolean hkEglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    FileLog("EGLHook", "hkEglSwapBuffers enter");

    if (!g_imguiInitialized) {
        FileLog("EGLHook", "InitImGuiFromGLContext...");
        g_imguiInitialized = InitImGuiFromGLContext();
        FileLog("EGLHook", "InitImGuiFromGLContext = %d", (int)g_imguiInitialized);
        if (g_imguiInitialized) {
            EGLint w = 0, h = 0;
            eglQuerySurface(dpy, surface, EGL_WIDTH,  &w);
            eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
            g_screenWidth  = (int)w;
            g_screenHeight = (int)h;
            FileLog("EGLHook", "surface size %dx%d", g_screenWidth, g_screenHeight);
        }
    }

    if (g_imguiInitialized && g_screenWidth > 0 && g_screenHeight > 0) {
        // FloatOverlay: initialize position on first frame
        static bool posLoaded = false;
        if (!posLoaded) {
            g_floatOverlay.restorePosition();
            posLoaded = true;
        }

        // ImGui frame
        FileLog("EGLHook", "BeginImGuiFrame...");
        if (BeginImGuiFrame(g_screenWidth, g_screenHeight, g_deltaTime)) {
            FileLog("EGLHook", "onImGuiRender...");
            onImGuiRender();

            // Render floating button overlay on top of everything
            g_floatOverlay.render(g_screenWidth, g_screenHeight);

            FileLog("EGLHook", "EndImGuiFrame...");
            EndImGuiFrame();
            FileLog("EGLHook", "EndImGuiFrame done");
        }
    }

    FileLog("EGLHook", "call original");
    EGLBoolean ret = oEglSwapBuffers(dpy, surface);
    FileLog("EGLHook", "hkEglSwapBuffers leave ret=%d", (int)ret);
    return ret;
}

bool HookEglSwapBuffers() {
    FileLog("EGLHook", "HookEglSwapBuffers enter");
    void* target = (void*)eglSwapBuffers;
    if (!target) {
        FileLog("EGLHook", "eglSwapBuffers address null");
        return false;
    }
    FileLog("EGLHook", "eglSwapBuffers = %p, hook target = %p", (void*)eglSwapBuffers, target);

    int ret = pl_hook(
        (PLFuncPtr)target,
        (PLFuncPtr)hkEglSwapBuffers,
        (PLFuncPtr*)&oEglSwapBuffers,
        PL_HOOK_PRIORITY_NORMAL);

    FileLog("EGLHook", "pl_hook returned %d, oEglSwapBuffers = %p", ret, (void*)oEglSwapBuffers);
    return (ret == 0);
}

void UnhookEglSwapBuffers() {
    FileLog("EGLHook", "UnhookEglSwapBuffers");
    if (oEglSwapBuffers) {
        pl_unhook((PLFuncPtr)eglSwapBuffers, (PLFuncPtr)hkEglSwapBuffers);
        oEglSwapBuffers = nullptr;
    }
}