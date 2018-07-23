#pragma once
#include "IUnityGraphics.h"

// This API is to be able to inject different variants into a shader compiler. A good example would be to
// generate VR instancing via geometry shaders.


enum UnityShaderCompilerExtCompilerPlatform
{
    kUnityShaderCompilerExtCompPlatformUnused0 = 0,
    kUnityShaderCompilerExtCompPlatformD3D9,            // Direct3D 9, compiled with MS D3DCompiler
    kUnityShaderCompilerExtCompPlatformUnused1,
    kUnityShaderCompilerExtCompPlatformUnused2,
    kUnityShaderCompilerExtCompPlatformD3D11,           // Direct3D 11 (FL10.0 and up), compiled with MS D3DCompiler
    kUnityShaderCompilerExtCompPlatformGLES20,          // OpenGL ES 2.0 / WebGL 1.0, compiled with hlsl2glslfork + glsl-optimizer
    kUnityShaderCompilerExtCompPlatformUnused3,
    kUnityShaderCompilerExtCompPlatformUnused4,
    kUnityShaderCompilerExtCompPlatformD3D11_9x,        // Direct3D 11 Feature Levels 9.x, compiled with MS D3DCompiler
    kUnityShaderCompilerExtCompPlatformGLES3Plus,       // OpenGL ES 3.0+ / WebGL 2.0, compiled with MS D3DCompiler + HLSLcc
    kUnityShaderCompilerExtCompPlatformPSP2,            // Sony PSP2/Vita
    kUnityShaderCompilerExtCompPlatformPS4,             // Sony PS4
    kUnityShaderCompilerExtCompPlatformXboxOne,         // MS XboxOne
    kUnityShaderCompilerExtCompPlatformUnused5,
    kUnityShaderCompilerExtCompPlatformMetal,           // Apple Metal, compiled with MS D3DCompiler + HLSLcc
    kUnityShaderCompilerExtCompPlatformOpenGLCore,      // Desktop OpenGL 3+, compiled with MS D3DCompiler + HLSLcc
    kUnityShaderCompilerExtCompPlatformN3DS,            // Nintendo 3DS
    kUnityShaderCompilerExtCompPlatformWiiU,            // Nintendo WiiU
    kUnityShaderCompilerExtCompPlatformVulkan,          // Vulkan SPIR-V, compiled with MS D3DCompiler + HLSLcc
    kUnityShaderCompilerExtCompPlatformSwitch,
    kUnityShaderCompilerExtCompPlatformCount
};


enum UnityShaderCompilerExtShaderType
{
    kUnityShaderCompilerExtShaderNone = 0,
    kUnityShaderCompilerExtShaderVertex = 1,
    kUnityShaderCompilerExtShaderFragment = 2,
    kUnityShaderCompilerExtShaderGeometry = 3,
    kUnityShaderCompilerExtShaderHull = 4,
    kUnityShaderCompilerExtShaderDomain = 5,
    kUnityShaderCompilerExtShaderTypeCount // keep this last!
};


enum UnityShaderCompilerExtGPUProgramType
{
    kUnityShaderCompilerExtGPUProgramTargetUnknown = 0,

    kUnityShaderCompilerExtGPUProgramTargetUnused0 = 1,
    kUnityShaderCompilerExtGPUProgramTargetGLES31AEP = 2,
    kUnityShaderCompilerExtGPUProgramTargetGLES31 = 3,
    kUnityShaderCompilerExtGPUProgramTargetGLES3 = 4,
    kUnityShaderCompilerExtGPUProgramTargetGLES = 5,
    kUnityShaderCompilerExtGPUProgramTargetGLCore32 = 6,
    kUnityShaderCompilerExtGPUProgramTargetGLCore41 = 7,
    kUnityShaderCompilerExtGPUProgramTargetGLCore43 = 8,
    kUnityShaderCompilerExtGPUProgramTargetDX9VertexSM20 = 9,
    kUnityShaderCompilerExtGPUProgramTargetDX9VertexSM30 = 10,
    kUnityShaderCompilerExtGPUProgramTargetDX9PixelSM20 = 11,
    kUnityShaderCompilerExtGPUProgramTargetDX9PixelSM30 = 12,
    kUnityShaderCompilerExtGPUProgramTargetDX10Level9Vertex = 13,
    kUnityShaderCompilerExtGPUProgramTargetDX10Level9Pixel = 14,
    kUnityShaderCompilerExtGPUProgramTargetDX11VertexSM40 = 15,
    kUnityShaderCompilerExtGPUProgramTargetDX11VertexSM50 = 16,
    kUnityShaderCompilerExtGPUProgramTargetDX11PixelSM40 = 17,
    kUnityShaderCompilerExtGPUProgramTargetDX11PixelSM50 = 18,
    kUnityShaderCompilerExtGPUProgramTargetDX11GeometrySM40 = 19,
    kUnityShaderCompilerExtGPUProgramTargetDX11GeometrySM50 = 20,
    kUnityShaderCompilerExtGPUProgramTargetDX11HullSM50 = 21,
    kUnityShaderCompilerExtGPUProgramTargetDX11DomainSM50 = 22,
    kUnityShaderCompilerExtGPUProgramTargetMetalVS = 23,
    kUnityShaderCompilerExtGPUProgramTargetMetalFS = 24,
    kUnityShaderCompilerExtGPUProgramTargetSPIRV = 25,
    kUnityShaderCompilerExtGPUProgramTargetUnused1 = 26,
    kUnityShaderCompilerExtGPUProgramTargetCount,
};


enum UnityShaderCompilerExtGPUProgram
{
    kUnityShaderCompilerExtGPUProgramVS = 1 << kUnityShaderCompilerExtShaderVertex,
    kUnityShaderCompilerExtGPUProgramPS = 1 << kUnityShaderCompilerExtShaderFragment,
    kUnityShaderCompilerExtGPUProgramGS = 1 << kUnityShaderCompilerExtShaderGeometry,
    kUnityShaderCompilerExtGPUProgramHS = 1 << kUnityShaderCompilerExtShaderHull,
    kUnityShaderCompilerExtGPUProgramDS = 1 << kUnityShaderCompilerExtShaderDomain,
    kUnityShaderCompilerExtGPUProgramCustom = 1 << kUnityShaderCompilerExtShaderTypeCount,
};


enum UnityShaderCompilerExtEventType
{
    kUnityShaderCompilerExtEventCreateCustomSourceVariant,          // The plugin can create a new variant based on the initial snippet. The callback will receive UnityShaderCompilerExtCustomSourceVariantParams as data.
    kUnityShaderCompilerExtEventCreateCustomSourceVariantCleanup,   // Typically received after the kUnityShaderCompilerExtEventCreateCustomVariant event so the plugin has a chance to cleanup after itself. (outputSnippet & outputKeyword)

    kUnityShaderCompilerExtEventCreateCustomBinaryVariant,          // The plugin can create a new variant based on the initial snippet. The callback will receive UnityShaderCompilerExtCustomBinaryVariantParams as data.
    kUnityShaderCompilerExtEventCreateCustomBinaryVariantCleanup,   // Typically received after the kUnityShaderCompilerExtEventCreateCustomVariant event so the plugin has a chance to cleanup after itself. (outputSnippet & outputKeyword)
    kUnityShaderCompilerExtEventPluginConfigure,                    // Event sent so the plugin can configure itself. It receives IUnityShaderCompilerExtPluginConfigure* as data

    // keep these last
    kUnityShaderCompilerExtEventCount,
    kUnityShaderCompilerExtUserEventsStart = kUnityShaderCompilerExtEventCount
};


struct UnityShaderCompilerExtCustomSourceVariantParams
{
    char* outputSnippet;                                    // snippet after the plugin has finished processing it.
    char* outputKeywords;                                   // keywords exported by the plugin for this specific variant
    const char* inputSnippet;                               // the source shader snippet
    bool vr;                                                // is VR enabled / supported ?
    UnityShaderCompilerExtCompilerPlatform platform;        // compiler platform
    UnityShaderCompilerExtShaderType shaderType;            // source code type
};


struct UnityShaderCompilerExtCustomBinaryVariantParams
{
    void**  outputBinaryShader;                             // output of the plugin generated binary shader (platform dependent)
    const unsigned char* inputByteCode;                     // the shader byteCode (platform dependent)
    unsigned int inputByteCodeSize;                         // shader bytecode size
    unsigned int programTypeMask;                           // a mask of UnityShaderCompilerExtGPUProgram values
    UnityShaderCompilerExtCompilerPlatform platform;        // compiler platform
};


/*
    This class is queried by unity to get information about the plugin.
    The plugin has to set all the relevant values during the kUnityShaderCompilerExtEventPluginConfigure event.

    extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
    UnityShaderCompilerExtEvent(UnityShaderCompilerExtEventType event, void* data)
    {
        switch (event)
        {

            ...

            case kUnityShaderCompilerExtEventPluginConfigure:
            {
                unsigned int GPUCompilerMask = (1 << kUnityShaderCompilerExtGPUProgramTargetDX11VertexSM40)
                | (1 << kUnityShaderCompilerExtGPUProgramTargetDX11VertexSM50)
                | (1 << kUnityShaderCompilerExtGPUProgramTargetDX11PixelSM40)
                | (1 << kUnityShaderCompilerExtGPUProgramTargetDX11PixelSM50)
                | (1 << kUnityShaderCompilerExtGPUProgramTargetDX11GeometrySM40)
                | (1 << kUnityShaderCompilerExtGPUProgramTargetDX11GeometrySM50)
                | (1 << kUnityShaderCompilerExtGPUProgramTargetDX11HullSM50)
                | (1 << kUnityShaderCompilerExtGPUProgramTargetDX11DomainSM50);

                unsigned int ShaderMask = kUnityShaderCompilerExtGPUProgramVS | kUnityShaderCompilerExtGPUProgramDS;

                IUnityShaderCompilerExtPluginConfigure* pluginConfig = (IUnityShaderCompilerExtPluginConfigure*)data;
                pluginConfig->ReserveKeyword(SHADER_KEYWORDS);
                pluginConfig->SetGPUProgramCompilerMask(GPUCompilerMask);
                pluginConfig->SetShaderProgramMask(ShaderMask);
                break;
            }
        }
    }
*/

class IUnityShaderCompilerExtPluginConfigure
{
public:
    virtual ~IUnityShaderCompilerExtPluginConfigure() {}
    virtual void ReserveKeyword(const char* keyword) = 0;           // Allow the plugin to reserve its keyword so unity can include it in calculating the variants
    virtual void SetGPUProgramCompilerMask(unsigned int mask) = 0;  // Specifies a bit mask of UnityShaderCompilerExtGPUProgramType programs the plugin supports compiling
    virtual void SetShaderProgramMask(unsigned int mask) = 0;       // Specifies a bit mask of UnityShaderCompilerExtGPUProgram programs the plugin supports compiling
};


// Interface to help setup / configure both unity and the plugin.


#ifdef __cplusplus
extern "C" {
#endif

// If exported by a plugin, this function will be called for all the events in UnityShaderCompilerExtEventType
void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityShaderCompilerExtEvent(UnityShaderCompilerExtEventType event, void* data);

#ifdef __cplusplus
}
#endif
