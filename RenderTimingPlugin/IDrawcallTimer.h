#include <cstdint>

#include <unordered_map>
#include <vector> 

#include "Unity/IUnityRenderingExtensions.h"

#define MAX_QUERY_SETS 2

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
    virtual void Start(UnityRenderingExtBeforeDrawCallParams drawcallParams) = 0;

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
    void AdvanceFrame();

    /*!
     * \brief Makes API-specific calls to resolve in-flight queries for the last frame
     */
    virtual void ResolveQueries() = 0;
    
protected:
    uint8_t _curFrame = 0;
};

/*!
 * \brief An abstract class for timing how long drawcalls take
 */
template <typename TimerType>
class DrawcallTimer : public IDrawcallTimer {
    public struct DrawcallQuery {
        TimerType StartQuery;
        TimerType EndQuery;
    };

protected:
    std::unordered_map<UnityRenderingExtBeforeDrawCallParams, std::vector<DrawcallQuery>> _timers[MAX_QUERY_SETS];
    std::vector<DrawcallQuery> _timerPool;
};