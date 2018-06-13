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
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using AOT;
using Microsoft.Win32.SafeHandles;
using UnityEngine;

/// Attach me to an active scene object to measure GPU render time
///
/// GPU time is measured from LateUpdate to WaitForEndOfFrame.  Late app rendering
/// done in WaitForEndOfFrame may not be accounted for, depending on script order.
///
/// TODO: support multiple & nested timings per frame
public class GpuTimer
{
  
  [StructLayout(LayoutKind.Sequential)]
  public struct ShaderTiming
  {
    public string VertexName;
    public string GeometryName;
    public string HullName;
    public string DomainName;
    public string FragmentName;
    
    public double Time;

    public override string ToString()
    {
      StringBuilder sb = new StringBuilder();
      sb.Append("Shader(");
      if (!VertexName.IsNullOrEmpty())
      {
        sb.Append("Vertex=");
        sb.Append(VertexName);
      }
      if (!GeometryName.IsNullOrEmpty())
      {
        sb.Append(", Geometry=");
        sb.Append(GeometryName);
      }
      if (!HullName.IsNullOrEmpty())
      {
        sb.Append(", Hull=");
        sb.Append(HullName);
      }
      if (!DomainName.IsNullOrEmpty())
      {
        sb.Append(", Domain=");
        sb.Append(DomainName);
      }
      if (!FragmentName.IsNullOrEmpty())
      {
        sb.Append(", Fragment=");
        sb.Append(FragmentName);
      }

      sb.Append(") took ");
      sb.Append(Time);
      sb.Append("ms this frame");

      return sb.ToString();
    }
  }

  public static GpuTimer Instance
  {
    get
    {
      if (_instance == null)
      {
        _instance = new GpuTimer();
      }

      return _instance;
    }
  }

  private static GpuTimer _instance = null;

  /// True to periodically log timing to debug console.  Honored only at init.
  public bool logTiming = true;
  
  [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
  private delegate void MyDelegate(string str);

  [MonoPInvokeCallback(typeof(MyDelegate))]
  static void DebugCallback(string str) {
    Debug.LogWarning("RenderTimingPlugin: " + str);
  }

  public List<ShaderTiming> ShaderTimings { get; private set; }
  public double GpuTime { get; private set; }
  
  private GpuTimer() {
    MyDelegate callback_delegate = DebugCallback;
    IntPtr intptr_delegate = Marshal.GetFunctionPointerForDelegate(callback_delegate);
    SetDebugFunction(intptr_delegate);

    ShaderTimings = new List<ShaderTiming>();
  }

  /// <summary>
  /// Tells the native GPU timer to consider the current frame finished, then caches the last frame's data 
  /// </summary>
  public void Update()
  {
    #if UNITY_ANDROID || UNITY_IOS
    GL.IssuePluginEvent(GetOnFrameEndFunction(), 0 /* unused */);
    #endif

    GetShaderTimings();
    GpuTime = GetLastFrameGpuTime();
  }
  
  #region Native functions
  
  #if UNITY_ANDROID
  [DllImport ("RenderTimingPlugin")]
  private static extern void SetDebugFunction(IntPtr ftp);
  
  [DllImport ("RenderTimingPlugin")]
  private static extern IntPtr GetOnFrameEndFunction();

  [DllImport("RenderTimingPlugin")]
  [return: MarshalAs(UnmanagedType.I1)]
  private static extern bool GetLastFrameShaderTimings(out IntPtr arrayPtr, out int size);

  [DllImport("RenderTimingPlugin")]
  private static extern float GetLastFrameGpuTime();
  
  #elif UNITY_IOS
  [DllImport ("__Internal")]
  private static extern void SetDebugFunction(IntPtr ftp);
  
  [DllImport ("__Internal")]
  private static extern IntPtr GetOnFrameEndFunction();

  [DllImport("__Internal")]
  [return: MarshalAs(UnmanagedType.I1)]
  private static extern bool GetLastFrameShaderTimings(out IntPtr arrayPtr, out int size);

  [DllImport("__Internal")]
  private static extern float GetLastFrameGpuTime();

  #else
  private static void SetDebugFunction(IntPtr fp) {}

  private static IntPtr GetOnFrameEndFunction()
  {
    return IntPtr.Zero;
  }

  private static bool GetLastFrameShaderTimings(out IntPtr arrayPtr, out int size)
  {
    size = 0;
    arrayPtr = IntPtr.Zero;
    return false;
  }

  private static float GetLastFrameGpuTime()
  {
    return 0;
  }
  #endif
  
  #endregion
  
  private void GetShaderTimings()
  {
    IntPtr arrayValue;
    int numShaders;
    ShaderTimings.Clear();

    if (!GetLastFrameShaderTimings(out arrayValue, out numShaders))
    {
      return;
    }

    var shaderTimingSize = Marshal.SizeOf(typeof(ShaderTiming));
    ShaderTimings.Capacity = numShaders;
    for (var i = 0; i < numShaders; i++)
    {
      var cur = (ShaderTiming) Marshal.PtrToStructure(arrayValue, typeof(ShaderTiming));
      ShaderTimings.Add(cur);

      arrayValue = new IntPtr(arrayValue.ToInt32() + shaderTimingSize);
    }
  }
}
