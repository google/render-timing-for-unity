#import "MetalDrawcallTimer.h"

#if SUPPORT_METAL && UNITY_IPHONE

bool MetalQuery::overlaps(const MetalQuery &other)
{
    if(startTime < other.startTime)
    {
        return endTime >= other.startTime;
    }

    if(endTime > other.endTime)
    {
        return startTime <= other.endTime;
    }

    return false;
}

bool compare_queries(const MetalQuery& first, const MetalQuery& second)
{
    return first.startTime < second.endTime;
}

MetalDrawcallTimer::MetalDrawcallTimer(IUnityInterfaces* unityInterfaces, DebugFuncPtr debugFunc) :
        DrawcallTimer(debugFunc)
{
      _unityMetal = unityInterfaces->Get<IUnityGraphicsMetal>();
}

void MetalDrawcallTimer::Start(UnityRenderingExtBeforeDrawCallParams *drawcallParams)
{
    MTLCommandBufferHandler getTimingInfo = [this](id<MTLCommandBuffer> buf)
    {
        MetalQuery query = {};
        query.startTime = buf.GPUStartTime;
        query.endTime = buf.GPUEndTime;

        _queriesLock.lock();
        _queries.push_back(query);
        _queriesLock.unlock();
    };

    auto cmdBuf = _unityMetal->CurrentCommandBuffer();
    [cmdBuf addCompletedHandler:getTimingInfo];

    _curQuery = DrawcallQuery();
    _timers[_curFrame][*drawcallParams] = _curQuery;
}

void MetalDrawcallTimer::End()
{ }

void MetalDrawcallTimer::ResolveQueries()
{
    CFTimeInterval timeFromCallbacks = GetTotalGpuTimeFromQueries();

    _queriesLock.lock()
    _queries.clear();

    for(const auto& bufWrapper : _timers[_curFrame]) {
        id<MTLCommandBuffer> buf = bufWrapper.second.StartQuery;
        [buf waitUntilCompleted];

        MetalQuery query = {};
        query.startTime = buf.GPUStartTime;
        query.endTime = buf.GPUEndTime;

        _queries.push_back(query);
    }
    _queriesLock.unlock();

    CFTimeInterval timeFromCompletedBuffers = GetTotalGpuTimeFromQueries();

    _lastFrameTime = std::max(timeFromCallbacks, timeFromCompletedBuffers);
}

CFTimeInterval MetalDrawcallTimer::GetTotalGpuTimeFromQueries() {
    // Go through each query, getting the start and end time. Keep track of the blocks of time available, expand blocks
    // when we see an overlapping block

    std::list<MetalQuery> gpuBusyBlocks;

    _queriesLock.lock();
    _queries.sort(compare_queries);
    for(const MetalQuery& query : _queries)
    {
        bool needNewBlock = true;

        // Is there already a block that overlaps this one?
        for(MetalQuery& timeblock : gpuBusyBlocks)
        {
            if(timeblock.overlaps(query))
            {
                timeblock.startTime = std::min(query.startTime, timeblock.endTime);
                timeblock.endTime = std::max(query.endTime, timeblock.endTime);
                needNewBlock = false;
            }
        }

        if(needNewBlock)
        {
            gpuBusyBlocks.push_back(query);
        }

        gpuBusyBlocks.sort(compare_queries);
    }
    _queriesLock.unlock();

    // At this point, gpuBusyBlocks is sorted by startTime, and it shouldn't have any overlapping blocks. We can sum up
    // all the durations and report back the final number
    CFTimeInterval totalGpuTime = 0;
    for(const MetalQuery& block : gpuBusyBlocks)
    {
        totalGpuTime += block.endTime - block.startTime;
    }

    return totalGpuTime;
}

#endif
