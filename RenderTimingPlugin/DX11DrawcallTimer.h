#include <d3d11.h>

#include "IDrawcallTimer.h"
#include "Unity/IUnityGraphicsD3D11.h"

class DX11DrawcallTimer : public DrawcallTimer<ID3D11Query*> {
public:
    DX11DrawcallTimer(IUnityGraphicsD3D11* d3d);
    ~DX11DrawcallTimer();

    /*
     * Inherited from DrawcallTimer
     */

    void Start(UnityRenderingExtBeforeDrawCallParams* drawcallParams) override;
    void End() override;

    void ResolveQueries() override;

private:
    ID3D11Device * _d3dDevice;
    ID3D11DeviceContext* _d3dContext;
};