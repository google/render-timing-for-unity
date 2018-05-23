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

using System;
using System.Runtime.InteropServices;
using AOT;
using UnityEngine;
using UnityEngine.Rendering;

/// Attach me to an active scene object to measure GPU render time
///
/// GPU time is measured from LateUpdate to WaitForEndOfFrame.  Late app rendering
/// done in WaitForEndOfFrame may not be accounted for, depending on script order.
///
/// TODO: support multiple & nested timings per frame
public class RenderTiming : MonoBehaviour {
  public static RenderTiming instance;

  /// True to periodically log timing to debug console.  Honored only at init.
  public bool logTiming = true;

  [DllImport ("RenderTimingPlugin")]
  private static extern void SetDebugFunction(IntPtr ftp);
  [DllImport ("RenderTimingPlugin")]
  private static extern IntPtr GetOnFrameEndFunction();

  [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
  private delegate void MyDelegate(string str);
  private bool isInitialized;
  private CommandBuffer commandBufferStart;
  private CommandBuffer commandBufferEnd;
  private Coroutine loggingCoroutine;

  [MonoPInvokeCallback(typeof(MyDelegate))]
  static void DebugCallback(string str) {
    Debug.LogWarning("RenderTimingPlugin: " + str);
  }

  void Awake() {
    Debug.Assert(instance == null, "only one instance allowed");
    instance = this;
    // Note: intentionally not accessing any plugin code here so that leaving the component
    // disabled will avoid loading the plugin.
  }

  // called from first OnEnable()
  void Init() {
    MyDelegate callback_delegate = DebugCallback;
    IntPtr intptr_delegate = Marshal.GetFunctionPointerForDelegate(callback_delegate);
    SetDebugFunction(intptr_delegate);

    isInitialized = true;
  }

  private void OnPostRender()
  {
    GL.IssuePluginEvent(GetOnFrameEndFunction(), 0 /* unused */);
  }

  void OnEnable() {
    if (!isInitialized) {
      Init();
    }
  }
}
