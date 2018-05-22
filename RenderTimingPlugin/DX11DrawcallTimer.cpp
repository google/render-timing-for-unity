#include "DX11DrawcallTimer.h"

DX11DrawcallTimer::DX11DrawcallTimer(IUnityGraphicsD3D11* d3d) {
    _d3dDevice = d3d->GetDevice();
    _d3dDevice->GetImmediateContext(&_d3dContext);

    D3D11_QUERY_DESC disjointQueryDesc;
    disjointQueryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    for (int i = 0; i < MAX_QUERY_SETS; i++) {
        _d3dDevice->CreateQuery(&disjointQueryDesc, &_disjointQueries[i]);
    }
}

DX11DrawcallTimer::~DX11DrawcallTimer()
{
    _d3dContext->Release();
}

void DX11DrawcallTimer::Start(UnityRenderingExtBeforeDrawCallParams* drawcallParams) {
    DrawcallQuery drawcallQuery;

    if (_timerPool.empty()) {
        ID3D11Query* startQuery;
        D3D11_QUERY_DESC startQueryDesc;
        startQueryDesc.Query = D3D11_QUERY_TIMESTAMP;
        auto queryResult = _d3dDevice->CreateQuery(&startQueryDesc, &startQuery);
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
        drawcallQuery = _timerPool.back();
        _timerPool.pop_back();
    }

    _d3dContext->End(drawcallQuery.StartQuery);
    _curQuery = drawcallQuery;
    _timers[_curFrame][drawcallParams].push_back(drawcallQuery);
}

void DX11DrawcallTimer::End() {
    _d3dContext->End(_curQuery.EndQuery);
}

void DX11DrawcallTimer::ResolveQueries()
{
    const auto& curFullFrameQuery = _fullFrameQueries[_curFrame];

    _d3dContext->End(curFullFrameQuery.EndQuery);
    _d3dContext->End(_disjointQueries[_curFrame]);
    _d3dContext->Begin(_disjointQueries[GetNextFrameIndex()]);
    _d3dContext->End(_fullFrameQueries[GetNextFrameIndex()].StartQuery);

    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;

    // Wait for data to become available. This will huts the frame time a little but it's fine
    while (_d3dContext->GetData(_disjointQueries[_curFrame], nullptr, 0, 0) == S_FALSE) {
        Sleep(1);
    }
    _d3dContext->GetData(_disjointQueries[_curFrame], &disjointData, sizeof(disjointData), 0);
    if (disjointData.Disjoint) {
        return;
    }

    uint64_t frameStart, frameEnd;
    _d3dContext->GetData(curFullFrameQuery.StartQuery, &frameStart, sizeof(uint64_t), 0);
    _d3dContext->GetData(curFullFrameQuery.EndQuery, &frameEnd, sizeof(uint64_t), 0);

    auto fullFrameTime = double(frameEnd - frameStart) / double(disjointData.Frequency);
    if (_frameCounter % 30 == 0) {
        ::printf("The frame took %d ms total\n", fullFrameTime);
    }

    std::unordered_map<UnityRenderingExtBeforeDrawCallParams, double, UnityDrawCallParamsHasher> shaderTimings;

    // Collect raw GPU time for each shader
    for (const auto& shaderTimers : _timers[_curFrame]) {
        uint64_t shaderTime;
        for (const auto& timer : shaderTimers.second) {
            UINT64 startTime, endTime;
            _d3dContext->GetData(timer.StartQuery, &startTime, sizeof(UINT64), 0);
            _d3dContext->GetData(timer.EndQuery, &endTime, sizeof(UINT64), 0);

            shaderTime += endTime - startTime;
        }

        const auto& shader = shaderTimers.first;

        shaderTimings[shader] = double(shaderTime) / double(disjointData.Frequency);

        if (_frameCounter % 30 == 0) {
            ::printf("Shader vertex=%02x geometry=%02x hull=%02x domain=%02x fragment=%02x took %d ms\n",
                shader.vertexShader, shader.geometryShader, shader.hullShader, shader.domainShader,
                shader.fragmentShader, shaderTime);
        }
    }

    // TODO: Propagate the timing data to the C# code 

    // Erase the data so there's no garbage for next time 
    _timers[_curFrame].clear();
}
