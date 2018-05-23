cmake_minimum_required(VERSION 3.8)
project(RenderTimingPlugin)

function(set_output_directories target output_dir)
    set_target_properties(${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${output_dir})
    set_target_properties(${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${output_dir})
    set_target_properties(${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${output_dir})
    set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${output_dir})
    set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${output_dir})
    set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${output_dir})
    set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${output_dir})
    set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${output_dir})
    set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${output_dir})
endfunction()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 11)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CONFIGURATION_TYPES Debug)

set(COMMON_SOURCE 
    RenderTimingPlugin.h RenderTimingPlugin.cpp
    IDrawcallTimer.h IDrawcallTimer.cpp)

set(D3D11_SOURCE 
    DX11DrawcallTimer.h DX11DrawcallTimer.cpp)

add_library(RenderTimingPlugin SHARED ${COMMON_SOURCE} ${D3D11_SOURCE})

message(sizeof void p: ${CMAKE_SIZEOF_VOID_P})

if(APPLE)
    set_output_directories(RenderTimingPlugin ${CMAKE_CURRENT_LIST_DIR}/../../Plugins)
else()
    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4") 
        set_output_directories(RenderTimingPlugin ${CMAKE_CURRENT_LIST_DIR}/../../Plugins/x86)
    else()
        set_output_directories(RenderTimingPlugin ${CMAKE_CURRENT_LIST_DIR}/../../Plugins/x86_64)
    endif()
endif()