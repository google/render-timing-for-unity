#pragma once

#include "IDrawcallTimer.h"

#if SUPPORT_METAL

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
    static MetalDrawcallTimer* GetInstance();

    MetalDrawcallTimer(IUnityInterfaces* unityInterfaces, DebugFuncPtr debugFunc);

    void Start(UnityRenderingExtBeforeDrawCallParams* drawcallParams) override;
    void End() override;
    void ResolveQueries() override;

private:
    static MetalDrawcallTimer* _instance;

    IUnityGraphicsMetal* _unityMetal;

    std::mutex _queriesLock;
    std::list<MetalQuery> _queries;

    CFTimeInterval GetTotalGpuTimeFromQueries();
};

#endif