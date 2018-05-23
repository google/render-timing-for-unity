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
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
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
  [StructLayout(LayoutKind.Sequential)]
  public struct ShaderTiming
  {
    public string VertexName;
    public string GeometyName;
    public string HullName;
    public string DomainName;
    public string FragmentName;

    public double Time;

    public override string ToString()
    {
      return string.Format("Shader(Vertex={0}, Geometry={1}, Hull={2}, Domain={3}, Fragment={4}) took {5}ms this frame",
        VertexName, GeometyName, HullName, DomainName, FragmentName, Time);
    }
  }
  
  public static RenderTiming instance;

  /// True to periodically log timing to debug console.  Honored only at init.
  public bool logTiming = true;
  

  [DllImport ("RenderTimingPlugin")]
  private static extern void SetDebugFunction(IntPtr ftp);
  [DllImport ("RenderTimingPlugin")]
  private static extern IntPtr GetOnFrameEndFunction();

  [DllImport("RenderTimingPlugin")]
  [return: MarshalAs(UnmanagedType.I1)]
  private static extern bool GetMostRecentShaderTimings(out IntPtr arrayPtr, out int size);

  [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
  private delegate void MyDelegate(string str);
  private bool isInitialized;
  private Coroutine loggingCoroutine;
  private Coroutine updateCoroutine;

  [MonoPInvokeCallback(typeof(MyDelegate))]
  static void DebugCallback(string str) {
    Debug.LogWarning("RenderTimingPlugin: " + str);
  }

  public static List<ShaderTiming> GetShaderTimings()
  {
    var arrayValue = IntPtr.Zero;
    var size = 0;
    var list = new List<ShaderTiming>();

    if (!GetMostRecentShaderTimings(out arrayValue, out size))
    {
      return null;
    }

    var shaderTimingSize = Marshal.SizeOf(typeof(ShaderTiming));
    for (var i = 0; i < size; i++)
    {
      var cur = (ShaderTiming) Marshal.PtrToStructure(arrayValue, typeof(ShaderTiming));
      list.Add(cur);
      arrayValue = new IntPtr(arrayValue.ToInt32() + shaderTimingSize);
    }

    return list;
  }
  
  private void Awake() {
    Debug.Assert(instance == null, "only one instance allowed");
    instance = this;
    // Note: intentionally not accessing any plugin code here so that leaving the component
    // disabled will avoid loading the plugin.
  }

  // called from first OnEnable()
  private void Init() {
    MyDelegate callback_delegate = DebugCallback;
    IntPtr intptr_delegate = Marshal.GetFunctionPointerForDelegate(callback_delegate);
    SetDebugFunction(intptr_delegate);

    isInitialized = true;
  }

  private void OnEnable() {
    if (!isInitialized) {
      Init();
    }

    updateCoroutine = StartCoroutine(UpdateOnFrameEnd());
    
    if (logTiming) {
      loggingCoroutine = StartCoroutine(ConsoleDisplay());
    }
  }

  private static IEnumerator ConsoleDisplay()
  {
    var sb = new StringBuilder();
    while (true)
    {
      yield return new WaitForSeconds(1);
      sb.Remove(0, sb.Length);

      var timings = GetShaderTimings();
      var numTimings = timings.Count;
      ShaderTiming curTiming;

      for (var i = 0; i < numTimings; i++)
      {
        curTiming = timings[i];

        sb.Append(curTiming);
        sb.Append("\n");
      }
      
      DebugCallback(sb.ToString());
    }
  }

  private void OnDisable()
  {
    StopCoroutine(updateCoroutine);

    if (loggingCoroutine != null)
    {
      StopCoroutine(loggingCoroutine);
    }
  }

  private static IEnumerator UpdateOnFrameEnd()
  {
    while (true)
    {
      yield return new WaitForEndOfFrame();
      
      GL.IssuePluginEvent(GetOnFrameEndFunction(), 0 /* unused */);
    }
    // ReSharper disable once IteratorNeverReturns
  }
}
