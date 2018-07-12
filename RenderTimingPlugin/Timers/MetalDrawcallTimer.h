#pragma once

#include "IDrawcallTimer.h"

#if SUPPORT_METAL && UNITY_IPHONE

#import <Metal/Metal.h>
#import <mutex>
#import <list>

#include "../Unity/IUnityGraphicsMetal.h"

struct MetalQuery {
    CFTimeInterval startTime;
    CFTimeInterval endTime;

    bool overlaps(const MetalQuery& other);
};

class UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API MetalDrawcallTimer : public DrawcallTimer<id<MTLCommandBuffer>> {
public:
    MetalDrawcallTimer(IUnityInterfaces* unityInterfaces, DebugFuncPtr debugFunc);

    void Start(UnityRenderingExtBeforeDrawCallParams* drawcallParams) override;
    void End() override;
    void ResolveQueries() override;

private:
    IUnityGraphicsMetal* _unityMetal;

    std::mutex _queriesLock;
    std::list<MetalQuery> _queries;

    CFTimerInterval GetTotalGpuTimeFromQueries();
};

#endif