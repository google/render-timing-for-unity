// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Plugin to expose GL_EXT_disjoint_timer_query, providing accurate GPU render
// timing in real-time to the app.

#include "RenderTimingPlugin.h"
#include "Unity/IUnityGraphics.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>

// --------------------------------------------------------------------------
// Include headers for the graphics APIs we support

#if SUPPORT_OPENGL_UNIFIED
#  if UNITY_IPHONE
#    include <OpenGLES/ES2/gl.h>
#  elif UNITY_ANDROID
#    include <GLES3/gl3.h>
#    define GL_TIME_ELAPSED                   0x88BF
#    define GL_GPU_DISJOINT                   0x8FBB
#  endif
#endif

#if SUPPORT_D3D11 
#include "DX11DrawcallTimer.h"
#endif

static IUnityInterfaces* s_UnityInterfaces = NULL;
static IUnityGraphics* s_Graphics = NULL;
static UnityGfxRenderer s_DeviceType = kUnityGfxRendererNull;
static void InitRenderTiming();

static IDrawcallTimer* s_drawcallTimer;

static const char * GfxRendererToString(UnityGfxRenderer deviceType);

// --------------------------------------------------------------------------
// GraphicsDeviceEvent

#if SUPPORT_OPENGL_UNIFIED

static void InitializeGfxApi(UnityGfxDeviceEventType eventType) {
  if (eventType == kUnityGfxDeviceEventInitialize) {
    if (s_DeviceType == kUnityGfxRendererOpenGLES20) {
      ::printf("OpenGLES 2.0 device\n");
    }
    else if(s_DeviceType == kUnityGfxRendererOpenGLES30) {
      ::printf("OpenGLES 3.0 device\n");
    }

    #if SUPPORT_D3D11
    else if(s_DeviceType == kUnityGfxRendererD3D11) {
      ::printf("DirectX 11 device\n");

      // Load DirectX 11
      s_drawcallTimer = new DX11DrawcallTimer(s_UnityInterfaces->Get<IUnityGraphicsD3D11>());
    }
    #endif
  }
  else if (eventType == kUnityGfxDeviceEventShutdown) {
  }
}
#endif // SUPPORT_OPENGL_UNIFIED

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType) {
  UnityGfxRenderer currentDeviceType = s_DeviceType;

  switch (eventType) {
  case kUnityGfxDeviceEventInitialize: {
      s_DeviceType = s_Graphics->GetRenderer();
      currentDeviceType = s_DeviceType;
      break;
    }

  case kUnityGfxDeviceEventShutdown: {
      s_DeviceType = kUnityGfxRendererNull;
      break;
    }

  case kUnityGfxDeviceEventBeforeReset: {
      break;
    }

  case kUnityGfxDeviceEventAfterReset: {
      break;
    }
  };

  #if SUPPORT_OPENGL_UNIFIED
  if (currentDeviceType == kUnityGfxRendererOpenGLES20 ||
    currentDeviceType == kUnityGfxRendererOpenGLES30 ||
    currentDeviceType == kUnityGfxRendererOpenGLCore) {
    InitializeGfxApi(eventType);
    InitRenderTiming();
  }
  #endif
}

// --------------------------------------------------------------------------
// UnitySetInterfaces

extern "C" void  UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces) {
  s_UnityInterfaces = unityInterfaces;
  s_Graphics = s_UnityInterfaces->Get<IUnityGraphics>();
  s_Graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);

  // Run OnGraphicsDeviceEvent(initialize) manually on plugin load
  OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload() {
  s_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityRenderingExtEvent(UnityRenderingExtEventType event, void* data) {
    switch(event) {
        case kUnityRenderingExtEventBeforeDrawCall:
            // Start rendering time query
            break;
        
        case kUnityRenderingExtEventAfterDrawCall:
            // End the timing query and save it somewhere
            break;
    }
}

// --------------------------------------------------------------------------
// GPU timer API

typedef void (*DebugFuncPtr)(const char*);

static DebugFuncPtr Debug;
static const int BUFFER_SIZE = 4;
static GLuint queries[BUFFER_SIZE];
static int query_buffer_next_index;
static int query_buffer_n;
static float elapsed_time_seconds = NAN;

static void InitRenderTiming() {
  switch(s_DeviceType) {
    #if SUPPORT_OPENGL_UNIFIED
    case kUnityGfxRendererOpenGLCore:
      glGenQueries(BUFFER_SIZE, queries);
      GLint disjointOccurred;
      glGetIntegerv(GL_GPU_DISJOINT, &disjointOccurred);  // clear
      break;
    #endif
    
    #if SUPPORT_D3D11
    case kUnityGfxRendererD3D11:
      s_drawcallTimer = new DX11DrawcallTimer();
      break;
    #endif 
    
    default:
      ::printf("Graphics api %s not supported\n", GfxRendererToString(s_DeviceType));
  }
}

// Register logging function which takes a string
extern "C" void SetDebugFunction(DebugFuncPtr fp) {
  Debug = fp;
}

// Start GL timer query, and read elapsed time from oldest pending query.
// Use GetTiming() to retrieve elapsed time, which yields NaN if no queries
// pending or timing otherwise can't be accessed.
// Arg eventID is unused.
// TODO: allow multiple (non-nested) timings per frame
// TODO: allow mulitple, nested timings per frame (start & stop timers)
static void UNITY_INTERFACE_API StartTimingEvent(int eventID) {
  // Unknown graphics device type? Do nothing.
  if (s_DeviceType == kUnityGfxRendererNull)
    return;

  // on disjoint exception, clear query buffer
  GLint disjointOccurred = false;
  glGetIntegerv(GL_GPU_DISJOINT, &disjointOccurred);
  if (disjointOccurred) {
    query_buffer_n = 0;
    Debug("disjoint exception");
  }

  // read result of oldest pending query
  // We use buffering to avoid blocking when reading results.
  elapsed_time_seconds = NAN;
  if (query_buffer_n > 0) {
    int index =
      (query_buffer_next_index + (BUFFER_SIZE - query_buffer_n)) % BUFFER_SIZE;
    GLuint available = 0;
    glGetQueryObjectuiv(queries[index], GL_QUERY_RESULT_AVAILABLE, &available);
    if (available) {
      GLuint elapsed_time_ns;
      glGetQueryObjectuiv(queries[index], GL_QUERY_RESULT, &elapsed_time_ns);
      if (glGetError() == GL_NO_ERROR) {
        elapsed_time_seconds = elapsed_time_ns/1e9f;
      } else {
        Debug("error reading query result");
      }
      --query_buffer_n;
    }
  }

  // start new query
  if (query_buffer_n < BUFFER_SIZE) {
    glBeginQuery(GL_TIME_ELAPSED, queries[query_buffer_next_index]);
    if (glGetError() == GL_NO_ERROR) {
      query_buffer_next_index = (query_buffer_next_index + 1) % BUFFER_SIZE;
      ++query_buffer_n;
    } else {
      Debug("glBeginQuery error");
    }
  } else {
    Debug("query buffer overflow");
  }
}

// End timing query.
// Arg eventID is unused.
// Must be called once for every StartTimingEvent.
static void UNITY_INTERFACE_API EndTimingEvent(int eventID) {
  // Unknown graphics device type? Do nothing.
  if (s_DeviceType == kUnityGfxRendererNull)
    return;

  glEndQuery(GL_TIME_ELAPSED);
  if (glGetError() != GL_NO_ERROR) Debug("glEndQuery error");
}

// Returns timing period in seconds if available, else NaN.  Reflects the value
// from several frames prior, depending on the level of GPU buffering.
extern "C" float GetTiming() {
  return elapsed_time_seconds;
}

// Returns plugin event function to be used in the app with IssuePluginEvent()
extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetStartTimingFunc() {
  return StartTimingEvent;
}

// Returns plugin event function to be used in the app with IssuePluginEvent()
extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetEndTimingFunc() {
  return EndTimingEvent;
}

// --------------------------------------------------------------------------
// Utilities

static const char * GfxRendererToString(UnityGfxRenderer deviceType) {
  switch(deviceType) {
    case kUnityGfxRendererD3D9:
      return "Direct3D 9";

    case kUnityGfxRendererD3D11:
      return "Direct3D 11";

    case kUnityGfxRendererGCM:
      return "PlayStation 3";

    case kUnityGfxRendererNull:
      return "null device (used in batch mode)";

    case kUnityGfxRendererOpenGLES20:
      return "OpenGL ES 2.0";

    case kUnityGfxRendererOpenGLES30:
      return "OpenGL ES 3.0";

    case kUnityGfxRendererGXM:
      return "PlayStation Vita";

    case kUnityGfxRendererPS4:
      return "PlayStation 4";

    case kUnityGfxRendererXboxOne:
      return "Xbox One";

    case kUnityGfxRendererMetal:
      return "iOS Metal";

    case kUnityGfxRendererOpenGLCore:
      return "OpenGL core";

    case kUnityGfxRendererD3D12:
      return "Direct3D 12";

    case kUnityGfxRendererVulkan:
      return "Vulkan";

    default:
      return "Unknown Device Type";
  }
}