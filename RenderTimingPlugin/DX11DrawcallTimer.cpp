#include "DX11DrawcallTimer.h"

#include <sstream>
#include <iostream>

#include <comdef.h>

DX11DrawcallTimer::DX11DrawcallTimer(IUnityGraphicsD3D11* d3d, DebugFuncPtr debugFunc) : DrawcallTimer(debugFunc) {
    Debug("Creating a DX11 drawcall timer");
    _d3dDevice = d3d->GetDevice();
    Debug("Acquired D3D11 device");
    if (_d3dDevice == nullptr) {
        Debug("D3D device is null!\n");
        return;
    }
    _d3dDevice->GetImmediateContext(&_d3dContext);
    Debug("Acquired an immediate context\n");

    if (_d3dContext == nullptr) {
        Debug("D3D context is null!");
        return;
    }

    D3D11_QUERY_DESC disjointQueryDesc;
    disjointQueryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    for (int i = 0; i < MAX_QUERY_SETS; i++) {
        _d3dDevice->CreateQuery(&disjointQueryDesc, &_disjointQueries[i]);
        Debug("Created a query");
    }

    Debug("DX11DrawcallTimer initialized\n");
}

DX11DrawcallTimer::~DX11DrawcallTimer()
{
    _d3dContext->Release();
}

void DX11DrawcallTimer::Start(UnityRenderingExtBeforeDrawCallParams* drawcallParams) {
    Debug("Starting a profiling block\n");
    DrawcallQuery drawcallQuery;

    if (_timerPool.empty()) {
        ID3D11Query* startQuery;
        D3D11_QUERY_DESC startQueryDesc;
        startQueryDesc.Query = D3D11_QUERY_TIMESTAMP;
        startQueryDesc.MiscFlags = 0;
        auto queryResult = _d3dDevice->CreateQuery(&startQueryDesc, &startQuery);
        if (queryResult != S_OK) {
            Debug("Could not create a query object: ");
            switch (queryResult) {
            case E_OUTOFMEMORY:
                Debug("Out of memory\n");
                break;

            default:
                _com_error err(queryResult);
                Debug(err.ErrorMessage());
                Debug("\n");
            }           

            return;
        }

        drawcallQuery.StartQuery = startQuery;
        Debug("Created start query\n");

        ID3D11Query* endQuery;
        D3D11_QUERY_DESC endQueryDecs;
        endQueryDecs.Query = D3D11_QUERY_TIMESTAMP;
        endQueryDecs.MiscFlags = 0;
        queryResult = _d3dDevice->CreateQuery(&endQueryDecs, &endQuery);
        if (queryResult != S_OK) {
            Debug("Could not create a query object: ");
            switch (queryResult) {
            case E_OUTOFMEMORY:
                Debug("Out of memory");
                break;

            default:
                _com_error err(queryResult);
                Debug(err.ErrorMessage());
            }

            return;
        }

        drawcallQuery.EndQuery = endQuery;
        Debug("Created end query\n");

    } else {
        drawcallQuery = _timerPool.back();
        _timerPool.pop_back();
        Debug("Acquited query from the pool\n");
    }

    _d3dContext->End(drawcallQuery.StartQuery);
    Debug("Got time of the start query\n");
    _curQuery = drawcallQuery;
    std::stringstream ss;
    ss << "Saving the current query in frame " << _curFrame << " out of " << MAX_QUERY_SETS;
    Debug(ss.str().c_str());
    _timers[_curFrame][*drawcallParams].push_back(drawcallQuery);
    Debug("Save complete\n");
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

    std::stringstream ss;

    if (_frameCounter % 30 == 0) {
        ss << "The frame took " << fullFrameTime << "ms total\n";
        Debug(ss.str().data());
    }

    std::unordered_map<UnityRenderingExtBeforeDrawCallParams, double, UnityDrawCallParamsHasher> shaderTimings;

    // Collect raw GPU time for each shader
    for (const auto& shaderTimers : _timers[_curFrame]) {
        uint64_t shaderTime = 0;
        for (const auto& timer : shaderTimers.second) {
            UINT64 startTime, endTime;
            _d3dContext->GetData(timer.StartQuery, &startTime, sizeof(UINT64), 0);
            _d3dContext->GetData(timer.EndQuery, &endTime, sizeof(UINT64), 0);

            shaderTime += endTime - startTime;

            _timerPool.push_back(timer);
        }

        const auto& shader = shaderTimers.first;

        shaderTimings[shader] = double(shaderTime) / double(disjointData.Frequency);

        if (_frameCounter % 30 == 0) {
            ss.clear();
            ss << "Shader vertex=" << shader.vertexShader << " geometry=" << shader.geometryShader << " hull="
                << shader.hullShader << " domain=" << shader.domainShader << " fragment=" << shader.fragmentShader
                << " took " << shaderTimings[shader] << "ms\n";
            Debug(ss.str().data());
        }
    }

    // TODO: Propagate the timing data to the C# code 

    // Erase the data so there's no garbage for next time 
    _timers[_curFrame].clear();
}
