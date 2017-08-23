/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "Unity/IUnityInterface.h"


// Which platform we are on?
#if _MSC_VER
#define UNITY_WIN 1
#elif defined(__APPLE__)
  #if defined(__arm__)
    #define UNITY_IPHONE 1
  #else
    #define UNITY_OSX 1
  #endif
#elif defined(UNITY_METRO) || defined(UNITY_ANDROID) || defined(UNITY_LINUX)
// these are defined externally
#else
#error "Unknown platform!"
#endif



// Which graphics device APIs we possibly support?
#if UNITY_METRO
  #define SUPPORT_D3D11 1
  #if WINDOWS_UWP
    #define SUPPORT_D3D12 1
  #endif
#elif UNITY_WIN
  #define SUPPORT_D3D9 1
  #define SUPPORT_D3D11 1 // comment this out if you don't have D3D11 header/library files
  #ifdef _MSC_VER
    #if _MSC_VER >= 1900
      #define SUPPORT_D3D12 1
    #endif
  #endif
  #define SUPPORT_OPENGL_LEGACY 1
  #define SUPPORT_OPENGL_UNIFIED 1
  #define SUPPORT_OPENGL_CORE 1
#elif UNITY_IPHONE || UNITY_ANDROID
  #define SUPPORT_OPENGL_UNIFIED 1
  #define SUPPORT_OPENGL_ES 1
#elif UNITY_OSX || UNITY_LINUX
  #define SUPPORT_OPENGL_LEGACY 1
  #define SUPPORT_OPENGL_UNIFIED 1
  #define SUPPORT_OPENGL_CORE 1
#endif
