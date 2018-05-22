#include "DX11DrawcallTimer.h"
#include <d3d11.h>

DX11DrawcallTimer::DX11DrawcallTimer(IUnityGraphicsD3D11* d3d) {
    _d3dDevice = d3d->GetDevice();
}

void DX11DrawcallTimer::Start(UnityRenderingExtBeforeDrawCallParams drawcallParams) {
    DrawcallQuery drawcallQuery;

    if (_timerPool.empty) {
        ID3D11Query* startQuery;
        D3D11_QUERY_DESC startQueryDecs;
        startQueryDecs.Query = D3D11_QUERY_TIMESTAMP;
        auto queryResult = _d3dDevice->CreateQuery(&startQueryDecs, &startQuery);
        if (queryResult != S_OK) {
            ::printf("Could not create a query object\n");
            return;
        }

        drawcallQuery.StartQuery = startQuery;

        ID3D11Query* endQuery;
        D3D11_QUERY_DESC endQueryDecs;
        endQueryDecs.Query = D3D11_QUERY_TIMESTAMP;
        queryResult = _d3dDevice->CreateQuery(&endQueryDecs, &endQuery);
        if (queryResult != S_OK) {
            ::printf("Could not create a query object\n");
            return;
        }

        drawcallQuery.EndQuery = endQuery;

    } else {

    }
}

void DX11DrawcallTimer::End() {
    
}
