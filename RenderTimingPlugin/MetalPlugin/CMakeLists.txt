cmake_minimum_required(VERSION 3.6)
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

set(METAL_SOURCE
        ${CMAKE_CURRENT_LIST_DIR}/../Timers/MetalDrawcallTimer.h ${CMAKE_CURRENT_LIST_DIR}/../Timers/MetalDrawcallTimer.mm
        ${CMAKE_CURRENT_LIST_DIR}/../Timers/MetalDrawcallTimerAdapter.h ${CMAKE_CURRENT_LIST_DIR}/../Timers/MetalDrawcallTimerAdapter.mm)

add_library(RenderTimingPlugin_Metal ${METAL_SOURCE})
set(CMAKE_C_FLAGS "-x objective-c")

target_include_directories(RenderTimingPlugin_Metal PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../)
