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

using UnityEngine;
using UnityEngine.Rendering;
using System.Collections;
using System.Runtime.InteropServices;
using System;

/// Attach me to an active scene object to measure GPU render time
///
/// GPU time is measured from LateUpdate to WaitForEndOfFrame.  Late app rendering
/// done in WaitForEndOfFrame may not be accounted for, depending on script order.
///
/// TODO: support multiple & nested timings per frame
/// TODO: have plugin work with GLES2
/// TODO: support more platforms (iOS, Vulkan, desktop, etc.)
public class RenderTiming : MonoBehaviour {
  public static RenderTiming instance;

  /// True if rendering timing supported in this platform
  /// TODO: test whether GL extention is available
  public bool isSupported {
    get { return GetStartTimingFunc() != IntPtr.Zero; }
  }

  /// Render period in seconds if available, else NaN.  Reflects the value
  /// from several frames prior, depending on the level of GPU buffering.
  /// TODO: smoothDeltaTime
  public float deltaTime { get { return GetTiming(); } }

  /// True to periodically log timing to debug console.  Honored only at init.
  public bool logTiming = true;

  #if UNITY_ANDROID && !UNITY_EDITOR
  [DllImport ("RenderTimingPlugin")]
  private static extern void SetDebugFunction(IntPtr ftp);
  [DllImport ("RenderTimingPlugin")]
  private static extern IntPtr GetStartTimingFunc();
  [DllImport ("RenderTimingPlugin")]
  private static extern IntPtr GetEndTimingFunc();
  [DllImport ("RenderTimingPlugin")]
  private static extern float GetTiming();
  #else
  private void SetDebugFunction(IntPtr ftp) {}
  private IntPtr GetStartTimingFunc() { return IntPtr.Zero; }
  private IntPtr GetEndTimingFunc() { return IntPtr.Zero; }
  private float GetTiming() { return float.NaN; }
  #endif

  [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
  private delegate void MyDelegate(string str);
  private bool isInitialized;
  private CommandBuffer commandBufferStart;
  private CommandBuffer commandBufferEnd;
  private Coroutine loggingCoroutine;

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
    MyDelegate callback_delegate = new MyDelegate(DebugCallback);
    IntPtr intptr_delegate = Marshal.GetFunctionPointerForDelegate(callback_delegate);
    SetDebugFunction(intptr_delegate);

    // prepare timer start & stop commands-- see OnEnable()
    commandBufferStart = new CommandBuffer();
    commandBufferStart.name = "StartGpuTiming";
    commandBufferStart.IssuePluginEvent(GetStartTimingFunc(), 0 /*unused*/);

    commandBufferEnd = new CommandBuffer();
    commandBufferEnd.name = "EndGpuTiming";
    commandBufferEnd.IssuePluginEvent(GetEndTimingFunc(), 0 /*unused*/);

    isInitialized = true;
  }

  void LateUpdate() {
    GL.IssuePluginEvent(GetStartTimingFunc(), 0 /*unused*/);
  }

  IEnumerator FrameEnd() {
    while (true) {
      yield return new WaitForEndOfFrame();
      if (enabled) {
        GL.IssuePluginEvent(GetEndTimingFunc(), 0 /*unused*/);
      } else {
        break;
      }
    }
  }

  void OnEnable() {
    if (!isSupported) {
      enabled = false;
      return;
    }

    if (!isInitialized) {
      Init();
    }

    StartCoroutine(FrameEnd());
    if (logTiming) {
      loggingCoroutine = StartCoroutine(ConsoleDisplay());
    }
  }

  void OnDisable() {
    if (!isSupported) {
      return;
    }

    if (loggingCoroutine != null) {
      StopCoroutine(loggingCoroutine);
      loggingCoroutine = null;
    }
  }

  private IEnumerator ConsoleDisplay() {
    while (true) {
      yield return new WaitForSeconds(1);
      Debug.LogFormat("Render time: {0:F3} ms", deltaTime * 1000);
    }
  }
}
