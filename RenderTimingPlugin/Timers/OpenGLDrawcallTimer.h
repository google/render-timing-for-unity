#pragma once

#include "../RenderTimingPlugin.h"
#if SUPPORT_OPENGL_UNIFIED
#  if UNITY_IPHONE
#    include <OpenGLES/ES2/gl.h>
#  elif UNITY_ANDROID
#    include <GLES3/gl3.h>
#    define GL_TIME_ELAPSED                   0x88BF
#    define GL_GPU_DISJOINT                   0x8FBB
#  else
#    include <GL/glew.h>
#    define GL_TIME_ELAPSED                   0x88BF
#    define GL_GPU_DISJOINT                   0x8FBB
#  endif

#include "IDrawcallTimer.h"

class OpenGLDrawcallTimer : public DrawcallTimer<GLuint> {
public:
    OpenGLDrawcallTimer(DebugFuncPtr debugFunc);

    /*
    * Inherited from DrawcallTimer
    */

    void Start(UnityRenderingExtBeforeDrawCallParams* drawcallParams) override;
    void End() override;

    void ResolveQueries() override;

private:
    uint64_t _timestampMask;
};

#ifndef NDEBUG
void EnableDebugCallback();

void GLAPIENTRY OnOpenGLMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, 
    const GLchar* message, const void* userParam);
#endif
#endif