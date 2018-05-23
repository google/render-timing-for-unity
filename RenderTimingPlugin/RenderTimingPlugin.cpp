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

static std::unique_ptr<IDrawcallTimer> s_DrawcallTimer;

static const char * GfxRendererToString(UnityGfxRenderer deviceType);


void simple_print(const char* c);

static DebugFuncPtr Debug = simple_print;

// --------------------------------------------------------------------------
// GraphicsDeviceEvent

static void InitializeGfxApi() {
  switch (s_DeviceType) {
  case kUnityGfxRendererOpenGLES20: {
      Debug("OpenGLES 2.0 device\n");
      break;
    }

  case kUnityGfxRendererOpenGLES30: {
      Debug("OpenGLES 3.0 device\n");
      break;
    }

  #if SUPPORT_D3D11
  case kUnityGfxRendererD3D11: {
      Debug("DirectX 11 device\n");

      // Load DirectX 11
      s_DrawcallTimer = std::make_unique<DX11DrawcallTimer>(s_UnityInterfaces->Get<IUnityGraphicsD3D11>());
      s_DrawcallTimer->SetDebugFunction(Debug);
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

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType) {
  switch (eventType) {
  case kUnityGfxDeviceEventInitialize: {
      s_DeviceType = s_Graphics->GetRenderer();
      InitializeGfxApi();
      break;
    }

  case kUnityGfxDeviceEventShutdown: {
      s_DeviceType = kUnityGfxRendererNull;
      s_DrawcallTimer.release();
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
  Debug("About to unload plugin\n");
  s_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityRenderingExtEvent(UnityRenderingExtEventType event, void* data) {
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

static void UNITY_INTERFACE_API OnFrameEnd(int eventID) {
  s_DrawcallTimer->AdvanceFrame();
}

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetOnFrameEndFunction() {
    return OnFrameEnd;
}

// --------------------------------------------------------------------------
// Utilities

// Register logging function which takes a string
extern "C" void UNITY_INTERFACE_EXPORT SetDebugFunction(DebugFuncPtr fp) {
  //Debug = fp;
  //if (s_DrawcallTimer) {
  //    s_DrawcallTimer->SetDebugFunction(Debug);
  //}
}

static void simple_print(const char* c) {
    printf(c);
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