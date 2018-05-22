cmake_minimum_required(VERSION 3.8)
project(render-timing-plugin)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 11)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CONFIGURATION_TYPES Debug)

set(COMMON_SOURCE 
    RenderTimingPlugin.h RenderTimingPlugin.cpp
    IDrawcallTimer.h IDrawcallTimer.cpp)

set(D3D11_SOURCE 
    DX11DrawcallTimer.h DX11DrawcallTimer.cpp)

add_library(render-timing-plugin SHARED ${COMMON_SOURCE} ${D3D11_SOURCE})