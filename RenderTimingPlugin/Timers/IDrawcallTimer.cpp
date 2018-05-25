#include "IDrawcallTimer.h"

#include <iostream>

void IDrawcallTimer::AdvanceFrame() {
    ResolveQueries();

    _frameCounter++;
    _curFrame = static_cast<uint8_t>(_frameCounter % MAX_QUERY_SETS);
}

void IDrawcallTimer::SetDebugFunction(DebugFuncPtr func)
{
    std::cout << "Setting debug function" << std::endl;
    Debug = func;
}

uint8_t IDrawcallTimer::GetNextFrameIndex()
{
    return static_cast<uint8_t>((_frameCounter + 1) % MAX_QUERY_SETS);
}

uint8_t IDrawcallTimer::GetLastFrameIndex()
{
    if (_frameCounter == 0) {
        return MAX_QUERY_SETS - 1;
    }
    return static_cast<uint8_t>((_frameCounter - 1) % MAX_QUERY_SETS);
}

const std::unordered_map<ShaderNames, double>& IDrawcallTimer::GetMostRecentShaderExecutionTimes() const {
    return _shaderTimes;
}

double IDrawcallTimer::GetLastFrameGpuTime() const {
    return _lastFrameTime;
}

bool operator==(const UnityRenderingExtBeforeDrawCallParams& lhs, const UnityRenderingExtBeforeDrawCallParams rhs) {
    return std::hash<UnityRenderingExtBeforeDrawCallParams>()(lhs) == std::hash<UnityRenderingExtBeforeDrawCallParams>()(rhs);
}

bool operator!=(const UnityRenderingExtBeforeDrawCallParams& lhs, const UnityRenderingExtBeforeDrawCallParams rhs) {
    return !(lhs == rhs);
}

bool operator==(const ShaderNames& lhs, const ShaderNames rhs) {
    return std::hash<ShaderNames>()(lhs) == std::hash<ShaderNames>()(rhs);
}

bool operator!=(const ShaderNames& lhs, const ShaderNames rhs) {
    return !(lhs == rhs);
}

namespace std {
    std::size_t hash<UnityRenderingExtBeforeDrawCallParams>::operator()(const UnityRenderingExtBeforeDrawCallParams & k) const
    {
        return reinterpret_cast<intptr_t>(k.vertexShader)
            ^ (reinterpret_cast<intptr_t>(k.fragmentShader) << 1)
            ^ (reinterpret_cast<intptr_t>(k.geometryShader) << 2)
            ^ (reinterpret_cast<intptr_t>(k.hullShader) << 3)
            ^ (reinterpret_cast<intptr_t>(k.domainShader) << 4)
            ^ (k.eyeIndex << 5);
    }

    std::size_t hash<ShaderNames>::operator()(const ShaderNames & k) const {

        return std::hash<std::string>()(k.Vertex)
            ^ (std::hash<std::string>()(k.Geometry) << 1)
            ^ (std::hash<std::string>()(k.Domain) << 2)
            ^ (std::hash<std::string>()(k.Hull) << 3)
            ^ (std::hash<std::string>()(k.Pixel) << 4);
    }
}