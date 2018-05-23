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
    return (_frameCounter + 1) % MAX_QUERY_SETS;
}

bool operator==(const UnityRenderingExtBeforeDrawCallParams& lhs, const UnityRenderingExtBeforeDrawCallParams rhs) {
    return std::hash<UnityRenderingExtBeforeDrawCallParams>()(lhs) == std::hash<UnityRenderingExtBeforeDrawCallParams>()(rhs);
}

bool operator!=(const UnityRenderingExtBeforeDrawCallParams& lhs, const UnityRenderingExtBeforeDrawCallParams rhs) {
    return !(lhs == rhs);
}