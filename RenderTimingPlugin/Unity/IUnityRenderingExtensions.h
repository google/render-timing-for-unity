#pragma once


#include "IUnityGraphics.h"

/*
    Plugin callback events
    ======================

    These events will be propagated to all plugins that implement void UnityRenderingExtEvent(UnityRenderingExtEventType event, void* data);
    In order to not have conflicts of IDs the plugin should query IUnityRenderingExtConfig::ReserveEventIDRange and use the returned index as
    an offset for the built-in plugin event enums. It should also export it to scripts to be able to offset the script events too.


    // Native code example

    enum PluginCustomCommands
    {
        kPluginCustomCommandDownscale = 0,  // In order to avoid event ID collisions, the custom events must be kept relative to the eventID base returned by ReserveEventIDRange.
        kPluginCustomCommandUpscale,

        // insert your own events here

        kPluginCustomCommandCount
    };

    static IUnityInterfaces* s_UnityInterfaces = NULL;
    int s_MyPluginEventStartIndex;

    extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
    UnityPluginLoad(IUnityInterfaces* unityInterfaces)
    {
        // Called before DX device is created
        s_UnityInterfaces = unityInterfaces;
        IUnityGraphics* unityGfx = s_UnityInterfaces->Get<IUnityGraphics>();
        s_MyPluginEventStartIndex = unityGfx->ReserveEventIDRange(kPluginCustomCommandCount);

        // More initialization code here...

    }

    static UnityRenderingExtEventType RemapToCustomEvent(UnityRenderingExtEventType event)
    {
        return (UnityRenderingExtEventType)((int)event - s_MyPluginEventStartIndex);
    }

    extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
    UnityRenderingExtEvent(UnityRenderingExtEventType event, void* data)
    {
        switch(event)
        {
            case kUnityRenderingExtEventSetStereoTarget:
            case kUnityRenderingExtEventBeforeDrawCall:
            case kUnityRenderingExtEventAfterDrawCall:
            case kUnityRenderingExtEventCustomGrab:
            case kUnityRenderingExtEventCustomBlit:
                ProcessBuiltinEvent(event, data);
                break;
            default:
                ProcessCustomEvent(RemapToCustomEvent(event) , data);
        }
    }

*/
enum UnityRenderingExtEventType
{
    kUnityRenderingExtEventSetStereoTarget,                 // issued during SetStereoTarget and carrying the current 'eye' index as parameter
    kUnityRenderingExtEventSetStereoEye,                    // issued during stereo rendering at the beginning of each eye's rendering loop. It carries the current 'eye' index as parameter
    kUnityRenderingExtEventStereoRenderingDone,             // issued after the rendering has finished
    kUnityRenderingExtEventBeforeDrawCall,                  // issued during BeforeDrawCall and carrying UnityRenderingExtBeforeDrawCallParams as parameter
    kUnityRenderingExtEventAfterDrawCall,                   // issued during AfterDrawCall. This event doesn't carry any parameters
    kUnityRenderingExtEventCustomGrab,                      // issued during GrabIntoRenderTexture since we can't simply copy the resources
                                                            //      when custom rendering is used - we need to let plugin handle this. It carries over
                                                            //      a UnityRenderingExtCustomBlitParams params = { X, source, dest, 0, 0 } ( X means it's irrelevant )
    kUnityRenderingExtEventCustomBlit,                      // issued by plugin to insert custom blits. It carries over UnityRenderingExtCustomBlitParams as param.

    // keep this last
    kUnityRenderingExtEventCount,
    kUnityRenderingExtUserEventsStart = kUnityRenderingExtEventCount
};


enum UnityRenderingExtCustomBlitCommands
{
    kUnityRenderingExtCustomBlitVRFlush,                    // This event is mostly used in multi GPU configurations ( SLI, etc ) in order to allow the plugin to flush all GPU's targets

    // keep this last
    kUnityRenderingExtCustomBlitCount,
    kUnityRenderingExtUserCustomBlitStart = kUnityRenderingExtCustomBlitCount
};

/*
    This will be propagated to all plugins implementing UnityRenderingExtQuery.
*/
enum UnityRenderingExtQueryType
{
    kUnityRenderingExtQueryOverrideViewport             = 1 << 0,
    kUnityRenderingExtQueryOverrideScissor              = 1 << 1,
    kUnityRenderingExtQueryOverrideVROcclussionMesh     = 1 << 2,
    kUnityRenderingExtQueryOverrideVRSinglePass         = 1 << 3,
    kUnityRenderingExtQueryKeepOriginalDoubleWideWidth  = 1 << 4,
    kUnityRenderingExtQueryRequestVRFlushCallback       = 1 << 5,
};


struct UnityRenderingExtBeforeDrawCallParams
{
    void*   vertexShader;                           // bound vertex shader (platform dependent)
    void*   fragmentShader;                         // bound fragment shader (platform dependent)
    void*   geometryShader;                         // bound geometry shader (platform dependent)
    void*   hullShader;                             // bound hull shader (platform dependent)
    void*   domainShader;                           // bound domain shader (platform dependent)
    int     eyeIndex;                               // the index of the current stereo "eye" being currently rendered.
};


struct UnityRenderingExtCustomBlitParams
{
    UnityTextureID source;                          // source texture
    UnityRenderBuffer destination;                  // destination surface
    unsigned int command;                           // command for the custom blit - could be any UnityRenderingExtCustomBlitCommands command or custom ones.
    unsigned int commandParam;                      // custom parameters for the command
    unsigned int commandFlags;                      // custom flags for the command
};


#ifdef __cplusplus
extern "C" {
#endif

// If exported by a plugin, this function will be called for all the events in UnityRenderingExtEventType
void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityRenderingExtEvent(UnityRenderingExtEventType event, void* data);
// If exported by a plugin, this function will be called to query the plugin for the queries in UnityRenderingExtQueryType
bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityRenderingExtQuery(UnityRenderingExtQueryType query);

#ifdef __cplusplus
}
#endif
