#pragma once

#include "../RenderTimingPlugin.h"

#if UNITY_ANDROID
#if SUPPORT_OPENGL_UNIFIED && !(UNITY_IPHONE || UNITY_IOS)

#if UNITY_IPHONE
#error "iPhone is not supported for OpenGL, why are we here?"
#elif UNITY_IOS
#error "iOS is not supported for OpenGL... And also the defines are setup wrong I think"
#elif UNITY_OSX
#error "Why are we on OSX?"
#endif

#  if UNITY_WIN || UNITY_OSX
#    include <GL/glew.h>
#    define GL_TIME_ELAPSED                   0x88BF
#    define GL_GPU_DISJOINT                   0x8FBB
#  elif UNITY_ANDROID
#    include <GLES3/gl3.h>
#    define GL_TIME_ELAPSED                   0x88BF
#    define GL_GPU_DISJOINT                   0x8FBB
#  endif

#include "IDrawcallTimer.h"

class OpenGLDrawcallTimer : public DrawcallTimer<GLuint> {
public:
    explicit OpenGLDrawcallTimer(DebugFuncPtr debugFunc);

    /*
    * Inherited from DrawcallTimer
    */

    void Start(UnityRenderingExtBeforeDrawCallParams* drawcallParams) override;
    void End() override;

    void ResolveQueries() override;

private:
#if SUPPORT_OPENGL_CORE
    uint64_t _timestampMask;

#else
    uint32_t _timestampMask;
#endif
};

#endif
#endif
