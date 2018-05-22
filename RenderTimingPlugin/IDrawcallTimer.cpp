#include "IDrawcallTimer.h"

void IDrawcallTimer::AdvanceFrame() {
    ResolveQueries();

    _frameCounter++;
    _curFrame = _frameCounter % MAX_QUERY_SETS;
}

uint8_t IDrawcallTimer::GetNextFrameIndex()
{
    if (_curFrame + 1 >= MAX_QUERY_SETS) {
        return 0;
    }
    
    return _curFrame + 1;
}

template <typename TimerType>
inline std::size_t DrawcallTimer<TimerType>::UnityDrawcallParamsHasher::operator()(const UnityRenderingExtBeforeDrawCallParams& k) const {
    return std::hash<void*>(k.vertexShader)
        ^ std::hash<void*>(k.fragmentShader)
        ^ std::hash<void*>(k.geometryShader)
        ^ std::hash<void*>(k.hullShader)
        ^ std::hash<void*>(k.domainShader)
        ^ std::hash<int>(k.eyeIndex);
}