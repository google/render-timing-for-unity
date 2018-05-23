#include "DX9DrawcallTimer.h"
#if SUPPORT_D3D9
#include <sstream>
#include <iostream>

#include <comdef.h>

DX9DrawcallTimer::DX9DrawcallTimer(IUnityGraphicsD3D9* d3d, DebugFuncPtr debugFunc) : DrawcallTimer(debugFunc) {
    _d3dDevice = d3d->GetDevice();
    if (_d3dDevice == nullptr) {
        Debug("D3D device is null!\n");
        return;
    }

    auto result = _d3dDevice->CreateQuery(D3DQUERYTYPE_TIMESTAMP, nullptr);
    if (result != S_OK) {
        Debug("Cannot not create timestamp queries");
        return;
    }

    result = _d3dDevice->CreateQuery(D3DQUERYTYPE_TIMESTAMPDISJOINT, nullptr);
    if (result != S_OK) {
        Debug("Cannot not create timestamp disjoint queries");
        return;
    }

    result = _d3dDevice->CreateQuery(D3DQUERYTYPE_TIMESTAMPFREQ, nullptr);
    if (result != S_OK) {
        Debug("Cannot not create timestamp frequency queries");
        return;
    }

    for (int i = 0; i < MAX_QUERY_SETS; i++) {
        _d3dDevice->CreateQuery(D3DQUERYTYPE_TIMESTAMPDISJOINT, &_disjointQueries[i]);
        _d3dDevice->CreateQuery(D3DQUERYTYPE_TIMESTAMPFREQ, &_frequencyQueries[i]);
    }

    _disjointQueries[_curFrame]->Issue(D3DISSUE_BEGIN);

    for (uint32_t i = 0; i < MAX_QUERY_SETS; i++) {
        _d3dDevice->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &(_fullFrameQueries[i].StartQuery));
        _d3dDevice->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &(_fullFrameQueries[i].EndQuery));
    }

    _fullFrameQueries[_curFrame].StartQuery->Issue(D3DISSUE_END);
}

void DX9DrawcallTimer::Start(UnityRenderingExtBeforeDrawCallParams* drawcallParams) {
    DrawcallQuery drawcallQuery;

    if (_timerPool.empty()) {
        IDirect3DQuery9* startQuery;
        auto queryResult = _d3dDevice->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &startQuery);
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

        IDirect3DQuery9* endQuery;
        queryResult = _d3dDevice->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &endQuery);
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

    }
    else {
        drawcallQuery = _timerPool.back();
        _timerPool.pop_back();
    }

    drawcallQuery.StartQuery->Issue(D3DISSUE_END);
    _curQuery = drawcallQuery;
    _timers[_curFrame][*drawcallParams].push_back(drawcallQuery);

    Debug("Began a drawcall timer");
}

void DX9DrawcallTimer::End() {
    _curQuery.EndQuery->Issue(D3DISSUE_END);
    Debug("Ended a drawcall timer");
}

void DX9DrawcallTimer::ResolveQueries()
{
    std::stringstream ss;

    bool record = true;
    const auto& curFullFrameQuery = _fullFrameQueries[_curFrame];

    curFullFrameQuery.EndQuery->Issue(D3DISSUE_END);
    _disjointQueries[_curFrame]->Issue(D3DISSUE_END);
    _frequencyQueries[_curFrame]->Issue(D3DISSUE_END);

    bool isDisjoint = false;
    // Wait for data to become available. This will hurt the frame time a little but it's fine
    _disjointQueries[_curFrame]->GetData(&isDisjoint, sizeof(bool), D3DGETDATA_FLUSH);

    if (isDisjoint) {
        Debug("Disjoint! Throwing away current frame\n");
        record = false;
    }

    uint64_t gpuFrequency = 0;
    _frequencyQueries[_curFrame]->GetData(&gpuFrequency, sizeof(uint64_t), D3DGETDATA_FLUSH);

    if (record) {
        uint64_t frameStart = 0, frameEnd = 0;
        curFullFrameQuery.StartQuery->GetData(&frameStart, sizeof(uint64_t), 0);
        curFullFrameQuery.EndQuery->GetData(&frameEnd, sizeof(uint64_t), 0);

        auto fullFrameTime = 1000 * double(frameEnd - frameStart) / double(gpuFrequency);

        if (_frameCounter % 30 == 0) {
            ss << "The frame took " << fullFrameTime << "ms total\n";
            Debug(ss.str().c_str());
        }
    }

    std::unordered_map<UnityRenderingExtBeforeDrawCallParams, double> shaderTimings;

    // Collect raw GPU time for each shader
    for (const auto& shaderTimers : _timers[_curFrame]) {
        uint64_t shaderTime = 0;
        uint64_t drawcallTime = 0;
        for (const auto& timer : shaderTimers.second) {
            if (record) {
                UINT64 startTime, endTime;
                timer.StartQuery->GetData(&startTime, sizeof(UINT64), 0);
                timer.EndQuery->GetData(&endTime, sizeof(UINT64), 0);

                drawcallTime = endTime - startTime;
                shaderTime += drawcallTime;
            }

            _timerPool.push_back(timer);
        }

        if (record) {
            const auto& shader = shaderTimers.first;

            shaderTimings[shader] = 1000 * double(shaderTime) / double(gpuFrequency);

            if (_frameCounter % 30 == 0 && shaderTimings[shader] > 0) {
                ss.str(std::string());
                ss << "shaderTime: " << shaderTime << " GPU frequency: " << gpuFrequency;
                Debug(ss.str().c_str());

                ss.str(std::string());
                ss << "Shader vertex=" << shader.vertexShader << " geometry=" << shader.geometryShader << " hull="
                    << shader.hullShader << " domain=" << shader.domainShader << " fragment=" << shader.fragmentShader
                    << " took " << shaderTimings[shader] << "ms";
                Debug(ss.str().c_str());
            }
        }
    }

    // TODO: Propagate the timing data to the C# code 

    // Erase the data so there's no garbage for next time 
    _timers[_curFrame].clear();

    _disjointQueries[GetNextFrameIndex()]->Issue(D3DISSUE_BEGIN);
    _fullFrameQueries[GetNextFrameIndex()].StartQuery->Issue(D3DISSUE_END);
}
#endif
