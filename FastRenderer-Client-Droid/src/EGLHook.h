#pragma once
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cstdio>

// ─── Hook eglSwapBuffers → insert ImGui rendering ───
// Uses GlossHook (LeviLaunchroid's built-in ARM64 hook library)

bool HookEglSwapBuffers();
void UnhookEglSwapBuffers();

// External forward declaration: called every frame after ImGui NewFrame
extern void onImGuiRender();

// ─── GL state save/restore ───
struct GlState {
    GLint lastProgram = 0;
    GLint lastTexture = 0;
    GLint lastActiveTexture = 0;
    GLint lastArrayBuffer = 0;
    GLint lastElementArrayBuffer = 0;
    GLint lastViewport[4] = {};
    GLint lastScissorBox[4] = {};
    GLboolean lastBlend = GL_FALSE;
    GLboolean lastCullFace = GL_FALSE;
    GLboolean lastDepthTest = GL_FALSE;
    GLboolean lastScissorTest = GL_FALSE;
    GLboolean lastStencilTest = GL_FALSE;
};

inline void SaveGlState(GlState& state) {
    glGetIntegerv(GL_CURRENT_PROGRAM, &state.lastProgram);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &state.lastTexture);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &state.lastActiveTexture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &state.lastArrayBuffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &state.lastElementArrayBuffer);
    glGetIntegerv(GL_VIEWPORT, state.lastViewport);
    glGetIntegerv(GL_SCISSOR_BOX, state.lastScissorBox);
    state.lastBlend       = glIsEnabled(GL_BLEND);
    state.lastCullFace    = glIsEnabled(GL_CULL_FACE);
    state.lastDepthTest   = glIsEnabled(GL_DEPTH_TEST);
    state.lastScissorTest = glIsEnabled(GL_SCISSOR_TEST);
    state.lastStencilTest = glIsEnabled(GL_STENCIL_TEST);
}

inline void RestoreGlState(const GlState& state) {
    glUseProgram(state.lastProgram);
    glActiveTexture(state.lastActiveTexture);
    glBindTexture(GL_TEXTURE_2D, state.lastTexture);
    glBindBuffer(GL_ARRAY_BUFFER, state.lastArrayBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state.lastElementArrayBuffer);
    glViewport(state.lastViewport[0], state.lastViewport[1],
               (GLsizei)state.lastViewport[2], (GLsizei)state.lastViewport[3]);
    glScissor(state.lastScissorBox[0], state.lastScissorBox[1],
              (GLsizei)state.lastScissorBox[2], (GLsizei)state.lastScissorBox[3]);
    if (state.lastBlend)       glEnable(GL_BLEND);       else glDisable(GL_BLEND);
    if (state.lastCullFace)    glEnable(GL_CULL_FACE);    else glDisable(GL_CULL_FACE);
    if (state.lastDepthTest)   glEnable(GL_DEPTH_TEST);   else glDisable(GL_DEPTH_TEST);
    if (state.lastScissorTest) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    if (state.lastStencilTest) glEnable(GL_STENCIL_TEST); else glDisable(GL_STENCIL_TEST);
}