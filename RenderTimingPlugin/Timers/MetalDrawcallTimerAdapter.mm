#include "MetalDrawcallTimerAdapter.h"

#if SUPPORT_METAL

#include "MetalDrawcallTimer.h"

MetalDrawcallTimerAdapter::MetalDrawcallTimerAdapter(IUnityInterfaces* unityInterfaces, DebugFuncPtr debugFunc) : DrawcallTimer(debugFunc)
{
    impl = new MetalDrawcallTimer(unityInterfaces, debugFunc);
}

MetalDrawcallTimerAdapter::~MetalDrawcallTimerAdapter()
{
    delete impl;
}

void MetalDrawcallTimerAdapter::Start(UnityRenderingExtBeforeDrawCallParams* drawcallParams)
{
    impl->Start(drawcallParams);
}

void MetalDrawcallTimerAdapter::End()
{
    impl->End();
}

void MetalDrawcallTimerAdapter::ResolveQueries()
{
    impl->ResolveQueries();
}

double MetalDrawcallTimerAdapter::GetLastFrameGpuTime() const
{
    return impl->GetLastFrameGpuTime();
}

void MetalDrawcallTimerAdapter::AdvanceFrame()
{
    impl->AdvanceFrame();
}

#endif