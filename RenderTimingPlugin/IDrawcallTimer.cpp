#include "IDrawcallTimer.h"

void IDrawcallTimer::AdvanceFrame() {
    _curFrame++;
    if (_curFrame >= MAX_QUERY_SETS) {
        _curFrame = 0;
    }

    ResolveQueries();
}