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

// File modified by KIXEYE in 2018
//
// Significant changes were made:
//  - Added a dummy GPU timer for when the APIs the native plugin supports aren't available
//  - Modified the GPU timer to automatically collect timing information each frame, instead of only timing sections 
//      where it was initially enabled, as in the original

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using AOT;
using UnityEngine;
using UnityEngine.Rendering;

public abstract class GpuTimerBase
{
    [StructLayout(LayoutKind.Sequential)]
    public struct ShaderTiming
    {
        public readonly string VertexName;
        public readonly string GeometryName;
        public readonly string HullName;
        public readonly string DomainName;
        public readonly string FragmentName;

        public readonly double Time;

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("Shader(");
            if (!string.IsNullOrEmpty(VertexName))
            {
                sb.Append("Vertex=");
                sb.Append(VertexName);
            }

            if (!string.IsNullOrEmpty(GeometryName))
            {
                sb.Append(", Geometry=");
                sb.Append(GeometryName);
            }

            if (!string.IsNullOrEmpty(HullName))
            {
                sb.Append(", Hull=");
                sb.Append(HullName);
            }

            if (!string.IsNullOrEmpty(DomainName))
            {
                sb.Append(", Domain=");
                sb.Append(DomainName);
            }

            if (!string.IsNullOrEmpty(FragmentName))
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

    public static GpuTimerBase Instance
    {
        get
        {
            if (_instance == null)
            {
                _instance = MakeGpuTimer();
            }

            return _instance;
        }
    }

    private static GpuTimerBase MakeGpuTimer()
    {
        bool canUseRenderTimer = false;

#if UNITY_ANDROID && !UNITY_EDITOR
        bool sdkAllowed = false;
        using (var version = new AndroidJavaClass("android.os.Build$VERSION")) 
        {
            sdkAllowed = version.GetStatic<int>("SDK_INT") >= 18;
        }
        canUseRenderTimer |= sdkAllowed;
#endif

        canUseRenderTimer |= SystemInfo.graphicsDeviceType == GraphicsDeviceType.OpenGLES3;

        // Desktop APIs so we get data in editor
        canUseRenderTimer |= SystemInfo.graphicsDeviceType == GraphicsDeviceType.Direct3D11;

#if UNITY_EDITOR
        canUseRenderTimer = false;
#endif

        if (canUseRenderTimer)
        {
            return new RealGpuTimer();
        }
        else
        {
            return new DummyGpuTimer();
        }
    }

    private static GpuTimerBase _instance = null;

    public double GpuTime { get; protected set; }

    public abstract void Update();
}

/// <summary>
/// Performs actual GPU timing, rather than 
/// </summary>
public class RealGpuTimer : GpuTimerBase
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void MyDelegate(string str);

    [MonoPInvokeCallback(typeof(MyDelegate))]
    static void DebugCallback(string str)
    {
        Debug.LogWarning("RenderTimingPlugin: " + str);
    }

    public List<ShaderTiming> ShaderTimings { get; private set; }

    protected internal RealGpuTimer()
    {
        MyDelegate callbackDelegate = DebugCallback;
        IntPtr intptrDelegate = Marshal.GetFunctionPointerForDelegate(callbackDelegate);
        SetDebugFunction(intptrDelegate);

        ShaderTimings = new List<ShaderTiming>();
    }

    /// <summary>
    /// Tells the native GPU timer to consider the current frame finished, then caches the last frame's data 
    /// </summary>
    public override void Update()
    {
#if UNITY_ANDROID || UNITY_EDITOR
        GL.IssuePluginEvent(GetOnFrameEndFunction(), 0 /* unused */);
#endif

        GetShaderTimings();
        GpuTime = GetLastFrameGpuTime();
    }

    #region Native functions

    // We don't need to check API versions or anything - this class is only Instantiated if the GPU timer is supported 
    // in the first place
#if UNITY_ANDROID || UNITY_EDITOR
    [DllImport ("RenderTimingPlugin")]
    private static extern void SetDebugFunction(IntPtr ftp);
  
    [DllImport ("RenderTimingPlugin")]
    private static extern IntPtr GetOnFrameEndFunction();

    [DllImport("RenderTimingPlugin")]
    [return: MarshalAs(UnmanagedType.I1)]
    private static extern bool GetLastFrameShaderTimings(out IntPtr arrayPtr, out int size);

    [DllImport("RenderTimingPlugin")]
    private static extern float GetLastFrameGpuTime();
  
#else
    private static void SetDebugFunction(IntPtr fp) { }

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
            var cur = (ShaderTiming)Marshal.PtrToStructure(arrayValue, typeof(ShaderTiming));
            ShaderTimings.Add(cur);

            arrayValue = new IntPtr(arrayValue.ToInt64() + shaderTimingSize);
        }
    }
}

/// <summary>
/// Used when the actual GPU timer isn't available, such as when we're using OpenGL ES 2
/// </summary>
public class DummyGpuTimer : GpuTimerBase
{
    protected internal DummyGpuTimer()
    {
        // This method intentionally left blank
    }

    public override void Update()
    {
        // This method intentionally left blank
    }
}
