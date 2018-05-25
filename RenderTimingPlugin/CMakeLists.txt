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

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 11)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CONFIGURATION_TYPES Debug)

set(UNITY_PLUGINS_DIR ${CMAKE_CURRENT_LIST_DIR}/../../Plugins)

set(COMMON_SOURCE 
    RenderTimingPlugin.h RenderTimingPlugin.cpp
    Timers/IDrawcallTimer.h Timers/IDrawcallTimer.cpp
    Timers/DX9DrawcallTimer.h Timers/DX9DrawcallTimer.cpp
    Timers/DX11DrawcallTimer.h Timers/DX11DrawcallTimer.cpp
    Timers/OpenGLDrawcallTimer.h Timers/OpenGLDrawcallTimer.cpp)

message(${COMMON_SOURCE})

add_library(RenderTimingPlugin SHARED ${COMMON_SOURCE})

# Set the output directory based on the platform - for Apple we build a combined 32/64 bit binary, so we put the 
# binary in the plugins directory directly, while for Windows we build separate 32-bit and 64-bit binaries, and they
# have to live in their own directories under Plugins
#
# Additionally, we need to link GLEW... for GL on Windows. GLEW didn't come with a MacOS binary so I'll need to solve 
# that, but for now OpenGL isn't supported on MacOS

if(${CMAKE_SYSTEM_NAME} MATCHES "Android")
    set_output_directories(RenderTimingPlugin ${UNITY_PLUGINS_DIR}/Android)
    add_definitions(-DUNITY_ANDROID)
    target_link_libraries(RenderTimingPlugin GLESv3)

elseif(APPLE)
    set_output_directories(RenderTimingPlugin ${UNITY_PLUGINS_DIR})

else()
    # Windows only. Linux is not supported

    target_include_directories(RenderTimingPlugin SYSTEM PRIVATE ${CMAKE_CURRENT_LIST_DIR}/3rdparty/glew/include/)
    target_link_libraries(RenderTimingPlugin PRIVATE opengl32)

    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4") 
        set_output_directories(RenderTimingPlugin ${UNITY_PLUGINS_DIR}/x86)
        target_link_libraries(RenderTimingPlugin PRIVATE ${CMAKE_CURRENT_LIST_DIR}/3rdparty/glew/lib/Release/Win32/glew32.lib)
    else()
        set_output_directories(RenderTimingPlugin ${UNITY_PLUGINS_DIR}/x86_64)
        target_link_libraries(RenderTimingPlugin PRIVATE ${CMAKE_CURRENT_LIST_DIR}/3rdparty/glew/lib/Release/x64/glew32.lib)
    endif()    
endif()
