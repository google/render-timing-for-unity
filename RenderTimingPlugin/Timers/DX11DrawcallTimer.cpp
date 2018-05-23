#include "DX11DrawcallTimer.h"
#if SUPPORT_D3D11
#include <sstream>
#include <iostream>

#include <comdef.h>

#pragma comment(lib, "dxguid.lib")

std::string GetShaderName(ID3D11DeviceChild* shader);

DX11DrawcallTimer::DX11DrawcallTimer(IUnityGraphicsD3D11* d3d, DebugFuncPtr debugFunc) : DrawcallTimer(debugFunc) {
    _d3dDevice = d3d->GetDevice();
    if (_d3dDevice == nullptr) {
        Debug("D3D device is null!\n");
        return;
    }
    _d3dDevice->GetImmediateContext(&_d3dContext);

    if (_d3dContext == nullptr) {
        Debug("D3D context is null!");
        return;
    }

    D3D11_QUERY_DESC disjointQueryDesc;
    disjointQueryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    disjointQueryDesc.MiscFlags = 0;
    for (int i = 0; i < MAX_QUERY_SETS; i++) {
        _d3dDevice->CreateQuery(&disjointQueryDesc, &_disjointQueries[i]);
    }

    _d3dContext->Begin(_disjointQueries[_curFrame]);

    D3D11_QUERY_DESC fullFrameQueryDesc;
    fullFrameQueryDesc.Query = D3D11_QUERY_TIMESTAMP;
    fullFrameQueryDesc.MiscFlags = 0;
    for (uint32_t i = 0; i < MAX_QUERY_SETS; i++) {
        _d3dDevice->CreateQuery(&fullFrameQueryDesc, &(_fullFrameQueries[i].StartQuery));
        _d3dDevice->CreateQuery(&fullFrameQueryDesc, &(_fullFrameQueries[i].EndQuery));
    }

    _d3dContext->End(_fullFrameQueries[_curFrame].StartQuery);
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

    } else {
        drawcallQuery = _timerPool.back();
        _timerPool.pop_back();
    }

    _d3dContext->End(drawcallQuery.StartQuery);
    _curQuery = drawcallQuery;
    _timers[_curFrame][*drawcallParams].push_back(drawcallQuery);
}

void DX11DrawcallTimer::End() {
    _d3dContext->End(_curQuery.EndQuery);
}

void DX11DrawcallTimer::ResolveQueries()
{
    std::stringstream ss;

    bool record = true;
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
        Debug("Disjoint! Throwing away current frame\n");
        record = false;
    }

    if (record) {
        uint64_t frameStart, frameEnd;
        _d3dContext->GetData(curFullFrameQuery.StartQuery, &frameStart, sizeof(uint64_t), 0);
        _d3dContext->GetData(curFullFrameQuery.EndQuery, &frameEnd, sizeof(uint64_t), 0);

        auto fullFrameTime = 1000 * double(frameEnd - frameStart) / double(disjointData.Frequency);
        
        if (_frameCounter % 30 == 0) {
            ss << "The frame took " << fullFrameTime << "ms total\n";
            Debug(ss.str().c_str());
        }
    }

    // Collect raw GPU time for each shader
    for (const auto& shaderTimers : _timers[_curFrame]) {

        const auto& shader = shaderTimers.first;
        ShaderNames shaderNames;
        
        if (shader.vertexShader != nullptr) {
            ID3D11DeviceChild* vertexShader = reinterpret_cast<ID3D11VertexShader*>(shader.vertexShader);
            shaderNames.Vertex = GetShaderName(vertexShader);
        }
        if (shader.geometryShader != nullptr) {
            ID3D11DeviceChild* geometryShader = reinterpret_cast<ID3D11GeometryShader*>(shader.geometryShader);
            shaderNames.Geometry = GetShaderName(geometryShader);
        }
        if (shader.domainShader != nullptr) {
            ID3D11DeviceChild* domainShader = reinterpret_cast<ID3D11DomainShader*>(shader.domainShader);
            shaderNames.Domain = GetShaderName(domainShader);
        }
        if (shader.hullShader != nullptr) {
            ID3D11DeviceChild* hullShader = reinterpret_cast<ID3D11HullShader*>(shader.hullShader);
            shaderNames.Hull = GetShaderName(hullShader);
        }
        if (shader.fragmentShader != nullptr) {
            ID3D11DeviceChild* pixelShader = reinterpret_cast<ID3D11PixelShader*>(shader.fragmentShader);
            shaderNames.Pixel = GetShaderName(pixelShader);
        }

        uint64_t shaderTime = 0;
        uint64_t drawcallTime = 0;
        for (const auto& timer : shaderTimers.second) {
            if (record) {
                UINT64 startTime, endTime;
                _d3dContext->GetData(timer.StartQuery, &startTime, sizeof(UINT64), 0);
                _d3dContext->GetData(timer.EndQuery, &endTime, sizeof(UINT64), 0);

                drawcallTime = endTime - startTime;
                shaderTime += drawcallTime;
            }

            _timerPool.push_back(timer);
        }

        if (record) {

            _shaderTimes[shaderNames] = 1000 * double(shaderTime) / double(disjointData.Frequency);

            if (_frameCounter % 30 == 0 && _shaderTimes[shaderNames] > 0) {

                ss.str(std::string());
                ss << "Shader vertex=" << shaderNames.Vertex;

                if (!shaderNames.Geometry.empty()) {
                    ss << " geometry=" << shaderNames.Geometry;
                }
                if (!shaderNames.Hull.empty()) {
                    ss << " hull=" << shaderNames.Hull;
                } 
                if (!shaderNames.Domain.empty()) {
                    ss << " domain=" << shaderNames.Domain;
                }
                ss << " fragment=" << shaderNames.Pixel << " took " << _shaderTimes[shaderNames] << "ms";

                Debug(ss.str().c_str());
            }
        }
    }

    // TODO: Propagate the timing data to the C# code 

    // Erase the data so there's no garbage for next time 
    _timers[_curFrame].clear();
}

std::string GetShaderName(ID3D11DeviceChild* shader) {

    uint32_t shaderNameSize = 512;
    char* shaderName = new char[shaderNameSize];
    shader->GetPrivateData(WKPDID_D3DDebugObjectName, &shaderNameSize, shaderName);
    shaderName[shaderNameSize] = '\0';

    return { shaderName };
}

#endif
