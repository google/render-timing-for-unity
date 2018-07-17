#import <sstream>
#import "MetalDrawcallTimer.h"

#if SUPPORT_METAL

#import "UnityAppController.h"

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

MetalDrawcallTimer* MetalDrawcallTimer::_instance;

MetalDrawcallTimer::MetalDrawcallTimer(IUnityInterfaces* unityInterfaces, DebugFuncPtr debugFunc) :
        DrawcallTimer(debugFunc)
{
      _unityMetal = unityInterfaces->Get<IUnityGraphicsMetal>();

      _instance = this;
}

void MetalDrawcallTimer::Start(UnityRenderingExtBeforeDrawCallParams *drawcallParams)
{
    auto cmdBuf = _unityMetal->CurrentCommandBuffer();
    [cmdBuf addCompletedHandler:^(id<MTLCommandBuffer> buf)
    {
        MetalQuery query = {};
        query.startTime = buf.GPUStartTime;
        query.endTime = buf.GPUEndTime;

        _queriesLock.lock();
        _queries.push_back(query);
        _queriesLock.unlock();
    }];

    _curQuery = DrawcallQuery();
    _timers[_curFrame][*drawcallParams].push_back(_curQuery);
}

void MetalDrawcallTimer::End()
{ }

void MetalDrawcallTimer::ResolveQueries()
{
    // Get time from the current command buffer - which should work at least a little bit, right?
    auto cmdBuf = _unityMetal->CurrentCommandBuffer();
    [cmdBuf addCompletedHandler:^(id<MTLCommandBuffer>) {
        MetalQuery query = {};
        query.startTime = cmdBuf.GPUStartTime;
        query.endTime = cmdBuf.GPUEndTime;

        _queriesLock.lock();
        _queries.push_back(query);
        _queriesLock.unlock();
    }];

    CFTimeInterval timeFromCallbacks = GetTotalGpuTimeFromQueries();

    _queriesLock.lock();
    _queries.clear();

    for(const auto& bufWrapper : _timers[_curFrame]) {
        for(const DrawcallQuery& timer : bufWrapper.second) {
            id <MTLCommandBuffer> buf = timer.StartQuery;
            [buf waitUntilCompleted];

            MetalQuery query = {};
            query.startTime = buf.GPUStartTime;
            query.endTime = buf.GPUEndTime;

            _queries.push_back(query);
        }
    }
    _queriesLock.unlock();

    CFTimeInterval timeFromCompletedBuffers = GetTotalGpuTimeFromQueries();

    std::stringstream ss;
    ss << "Time from callbacks: " << timeFromCallbacks << ", time from completed buffers: " << timeFromCompletedBuffers << "\n";
    Debug(ss.str().c_str());

    _lastFrameTime = std::max(timeFromCallbacks, timeFromCompletedBuffers);

    // Apple's things report the time in seconds, but we want it in ms
    _lastFrameTime *= 1000;
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

MetalDrawcallTimer *MetalDrawcallTimer::GetInstance() {
    return _instance;
}

#endif
