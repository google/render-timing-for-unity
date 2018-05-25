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

#include <memory>
#include <sstream>
#include <iostream>

#include <algorithm>

#include "Timers/IDrawcallTimer.h"

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

#if SUPPORT_D3D9
#include "Timers/DX9DrawcallTimer.h"
#endif

#if SUPPORT_D3D11 
#include "Timers/DX11DrawcallTimer.h"
#endif

static IUnityInterfaces* s_UnityInterfaces = NULL;
static IUnityGraphics* s_Graphics = NULL;
static UnityGfxRenderer s_DeviceType = kUnityGfxRendererNull;

static std::unique_ptr<IDrawcallTimer> s_DrawcallTimer;

static const char * GfxRendererToString(UnityGfxRenderer deviceType);


static void simple_print(const char* c);

static DebugFuncPtr Debug = simple_print;

// --------------------------------------------------------------------------
// GraphicsDeviceEvent

static void CreateProfilerForCurrentGfxApi() {

  if (s_UnityInterfaces == nullptr) {
    Debug("Unity interfaces is null!");
    return;
  }

  switch (s_DeviceType) {
  case kUnityGfxRendererOpenGLES20: {
      Debug("OpenGLES 2.0 device\n");
      break;
    }

  case kUnityGfxRendererOpenGLES30: {
      Debug("OpenGLES 3.0 device\n");
      break;
    }

  #if SUPPORT_D3D9
  case kUnityGfxRendererD3D9: {
      Debug("DirectX 9 device");

      IUnityGraphicsD3D9* d3d9Interface = s_UnityInterfaces->Get<IUnityGraphicsD3D9>();
      s_DrawcallTimer = std::make_unique<DX9DrawcallTimer>(d3d9Interface, Debug);
      break;
    }
  #endif

  #if SUPPORT_D3D11
  case kUnityGfxRendererD3D11: {
      Debug("DirectX 11 device");

      if (s_UnityInterfaces == nullptr) {
          Debug("Unity interfaces is null!");
          return;
      }

      // Load DirectX 11
      IUnityGraphicsD3D11* d3d11Interface = s_UnityInterfaces->Get<IUnityGraphicsD3D11>();
      s_DrawcallTimer = std::make_unique<DX11DrawcallTimer>(d3d11Interface, Debug);
      break;
    }
  #endif

  default: {
      Debug("Graphics api ");
      Debug(GfxRendererToString(s_DeviceType));
      Debug(" not supported\n");
    }
  }
}

// --------------------------------------------------------------------------
// C# Interface

extern "C" typedef struct {
    const char* VertexName;
    const char* GeometryName;
    const char* HullName;
    const char* DomainName;
    const char* FragmentName;

    double Time;
} ShaderTime;

static void UNITY_INTERFACE_API OnFrameEnd(int eventID) {
    if (s_DrawcallTimer) {
        s_DrawcallTimer->AdvanceFrame();
    }
}

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetOnFrameEndFunction() {
    return OnFrameEnd;
}

extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetLastFrameShaderTimings(ShaderTime** times, int32_t* size) {
    static std::vector<ShaderTime> timingsList;

    if (!s_DrawcallTimer) {
        return false;
    }

    const auto& timings = s_DrawcallTimer->GetMostRecentShaderExecutionTimes();
    if (timings.empty()) {
        return false;
    }

    timingsList.clear();
    timingsList.reserve(timings.size());
    for (const auto& timing : timings) {
        ShaderTime time;
        time.VertexName = timing.first.Vertex.c_str();
        time.GeometryName = timing.first.Geometry.c_str();
        time.HullName = timing.first.Hull.c_str();
        time.DomainName = timing.first.Domain.c_str();
        time.FragmentName = timing.first.Pixel.c_str();

        time.Time = timing.second;

        timingsList.push_back(time);
    }

    std::sort(timingsList.begin(), timingsList.end(), [](const ShaderTime& timing1, const ShaderTime& timing2) { return timing1.Time > timing2.Time; });

    *times = &timingsList.front();
    *size = static_cast<int32_t>(timingsList.size());

    return true;
}

extern "C" float UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetLastFrameGpuTime() {
    if (s_DrawcallTimer) {
        return static_cast<float>(s_DrawcallTimer->GetLastFrameGpuTime());
    }
    else {
        return 0;
    }
}

// Register logging function which takes a string
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetDebugFunction(DebugFuncPtr fp) {
    // Debug = fp;
    // if (s_DrawcallTimer) {
    //     s_DrawcallTimer->SetDebugFunction(Debug);
    // }
}

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType) {
  switch (eventType) {
  case kUnityGfxDeviceEventInitialize: {
      s_DeviceType = s_Graphics->GetRenderer();
      CreateProfilerForCurrentGfxApi();
      break;
    }
  
  case kUnityGfxDeviceEventShutdown: {
      s_DeviceType = kUnityGfxRendererNull;
      if (s_DrawcallTimer) {
        s_DrawcallTimer.release();
      }
      break;
    }
  
  case kUnityGfxDeviceEventBeforeReset: {
      break;
    }
  
  case kUnityGfxDeviceEventAfterReset: {
      break;
    }
  };
}

// --------------------------------------------------------------------------
// UnitySetInterfaces

extern "C" void  UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces) {
  freopen("debug.txt", "a", stdout);
  s_UnityInterfaces = unityInterfaces;
  s_Graphics = s_UnityInterfaces->Get<IUnityGraphics>();
  s_Graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);
  
  // Run OnGraphicsDeviceEvent(initialize) manually on plugin load - the plugin might have been loaded after the graphics device was initialized
  OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
  
  Debug("Loaded plugin\n");
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload() {
  // Debug("About to unload plugin\n");
  // s_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityRenderingExtEvent(UnityRenderingExtEventType event, void* data) {
  if (!s_DrawcallTimer) {
    return;
  }
  switch(event) {
  case kUnityRenderingExtEventBeforeDrawCall: {
      // Start rendering time query
      s_DrawcallTimer->Start(reinterpret_cast<UnityRenderingExtBeforeDrawCallParams*>(data));
      break;
    }
    
  case kUnityRenderingExtEventAfterDrawCall: {
      // End the timing query and save it somewhere
      s_DrawcallTimer->End();
      break;
    }
  }
}

// --------------------------------------------------------------------------
// Utilities

static void simple_print(const char* c) {
    std::cout << c << std::endl;
}

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