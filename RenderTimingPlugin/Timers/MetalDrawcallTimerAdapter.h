#pragma once

#include "IDrawcallTimer.h"

#if SUPPORT_METAL && UNITY_IPHONE

class MetalDrawcallTimer;

class UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API MetalDrawcallTimerAdapter : public DrawcallTimer<uint32_t> {
public:
    MetalDrawcallTimerAdapter(IUnityInterfaces* unityInterfaces, DebugFuncPtr debugFunc);
    ~MetalDrawcallTimerAdapter();

    void Start(UnityRenderingExtBeforeDrawCallParams* drawcallParams) override;
    void End() override;
    void ResolveQueries() override;

    double GetLastFrameGpuTime() const override;
    void AdvanceFrame() override;

private:
    MetalDrawcallTimer* impl;
};

#endif