#include "IDrawcallTimer.h"

#include <iostream>

void IDrawcallTimer::AdvanceFrame() {
    ResolveQueries();

    _frameCounter++;
    _curFrame = _frameCounter % MAX_QUERY_SETS;
}

void IDrawcallTimer::SetDebugFunction(DebugFuncPtr func)
{
    std::cout << "Setting debug function" << std::endl;
    Debug = func;
}

uint8_t IDrawcallTimer::GetNextFrameIndex()
{
    if (_curFrame + 1 >= MAX_QUERY_SETS) {
        return 0;
    }
    
    return _curFrame + 1;
}

std::size_t UnityDrawCallParamsHasher::operator()(const UnityRenderingExtBeforeDrawCallParams & k) const
{
    return reinterpret_cast<intptr_t>(k.vertexShader)
        ^ reinterpret_cast<intptr_t>(k.fragmentShader)
        ^ reinterpret_cast<intptr_t>(k.geometryShader)
        ^ reinterpret_cast<intptr_t>(k.hullShader)
        ^ reinterpret_cast<intptr_t>(k.domainShader)
        ^ k.eyeIndex;
}

bool operator==(const UnityRenderingExtBeforeDrawCallParams& lhs, const UnityRenderingExtBeforeDrawCallParams rhs) {
    auto hasher = UnityDrawCallParamsHasher();
    return hasher(lhs) == hasher(rhs);
}

bool operator!=(const UnityRenderingExtBeforeDrawCallParams& lhs, const UnityRenderingExtBeforeDrawCallParams rhs) {
    return !(lhs == rhs);
}