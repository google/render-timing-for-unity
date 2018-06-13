#pragma once

#include <cstdint>

#include <unordered_map>
#include <vector> 
#include <string>

#include "../Unity/IUnityRenderingExtensions.h"
#include "../RenderTimingPlugin.h"

#define MAX_QUERY_SETS 2

struct ShaderNames {
    std::string Vertex;
    std::string Geometry;
    std::string Hull;
    std::string Domain;
    std::string Pixel;
};

namespace std {
    template <>
    struct hash<ShaderNames> {
        std::size_t operator()(const ShaderNames& k) const;
    };

    template <>
    struct hash<UnityRenderingExtBeforeDrawCallParams> {
        std::size_t operator()(const UnityRenderingExtBeforeDrawCallParams& k) const;
    };
}

class IDrawcallTimer {
public:
    /*!
     * \brief Start timing for the provided drawcall params
     *
     * The drawcall params are just pointers to the shaders, and an ID for the eye being rendered. This is not 
     * terribly helpful, since I'd like to know like the name of the object being drawn or mesh parameters or like 
     * anything else really, but all well
     *
     * We get pointers to the shaders being used for the current drawcall. We grab two queries from the query pool: one
     * for before the drawcall and one for after. This tells me the time each drawcall takes
     *
     * This method starts the query for before the drawcall
     */
    virtual void Start(UnityRenderingExtBeforeDrawCallParams* drawcallParams) = 0;

    /*!
     * \brief Ends timing for the most recently started drawcall
     *
     * There's a HUUUUUUUUUGE assumption I'm making here: Every call to Start will be DIRECTLY followed by a call to 
     * End. There is no mechanism in my code for this assumption to be false - and no way for Unity to tell me that 
     * assumtion is false. Unity only passes the shaders for the drawcall begin event, and it doesn't give any 
     * parameters to the drawcall end event - leading me to assume that a drawcall begin event is followed immediately 
     * by a drawcall end event. I see no mechanism to confirm/deny that assumption - hopefully I'll get lucky
     *
     * This method starts the query for after the drawcall
     */
    virtual void End() = 0;

    /*!
     * \brief Advances the frame counter so we use a different set of queries (the queries are double-buffered), and 
     * makes the calls necessary to resolve the in-flight queries
     */
    virtual void AdvanceFrame();

    /*!
     * \brief Makes API-specific calls to resolve in-flight queries for the last frame
     */
    virtual void ResolveQueries() = 0;

    /*!
     * \brief Retrieve the timings for the most recently timed frame
     */
    const std::unordered_map<ShaderNames, double>& GetMostRecentShaderExecutionTimes() const;

    virtual double GetLastFrameGpuTime() const;

    void SetDebugFunction(DebugFuncPtr func);

    uint8_t GetNextFrameIndex();

    uint8_t GetLastFrameIndex();
    
protected:
    uint8_t _curFrame = 0;

    // If this thing overflows then you should probably close your application
    uint64_t _frameCounter = 0;

    double _lastFrameTime = 0;

    DebugFuncPtr Debug;

    std::unordered_map<ShaderNames, double> _shaderTimes;
};

/*!
 * \brief An abstract class for timing how long drawcalls take
 */
template <typename TimerType>
class DrawcallTimer : public IDrawcallTimer {
public:
    struct DrawcallQuery {
        TimerType StartQuery;
        TimerType EndQuery;
    };

    DrawcallTimer(DebugFuncPtr DebugFunc) {
        SetDebugFunction(DebugFunc);
    }

protected:
    std::unordered_map<UnityRenderingExtBeforeDrawCallParams, std::vector<DrawcallQuery>> _timers[MAX_QUERY_SETS];
    std::vector<DrawcallQuery> _timerPool;

    TimerType _disjointQueries[MAX_QUERY_SETS];
    DrawcallQuery _fullFrameQueries[MAX_QUERY_SETS];
    DrawcallQuery _curQuery;
};

bool operator==(const UnityRenderingExtBeforeDrawCallParams& lhs, const UnityRenderingExtBeforeDrawCallParams rhs);

bool operator!=(const UnityRenderingExtBeforeDrawCallParams& lhs, const UnityRenderingExtBeforeDrawCallParams rhs);

bool operator==(const ShaderNames& lhs, const ShaderNames rhs);

bool operator!=(const ShaderNames& lhs, const ShaderNames rhs);
