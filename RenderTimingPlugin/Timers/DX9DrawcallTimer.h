#pragma once

#include "../RenderTimingPlugin.h"
#if SUPPORT_D3D9
#include <d3d9.h>

#include "IDrawcallTimer.h"
#include "../Unity/IUnityGraphicsD3D9.h"

class DX9DrawcallTimer : public DrawcallTimer<IDirect3DQuery9*> {
public:
    DX9DrawcallTimer(IUnityGraphicsD3D9* d3d, DebugFuncPtr debugFunc);

    /*
    * Inherited from DrawcallTimer
    */

    void Start(UnityRenderingExtBeforeDrawCallParams* drawcallParams) override;
    void End() override;

    void ResolveQueries() override;

private:
    IDirect3DDevice9 * _d3dDevice;
    IDirect3DQuery9* _frequencyQueries[MAX_QUERY_SETS];
};
#endif