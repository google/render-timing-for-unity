#pragma once

#include "OpenGLDrawcallTimer.h"
#if SUPPORT_OPENGL_UNIFIED

#include <sstream>
#include <iostream>

std::string GetShaderName(GLuint* shader);

OpenGLDrawcallTimer::OpenGLDrawcallTimer(DebugFuncPtr debugFunc) : DrawcallTimer(debugFunc) {
#if SUPPORT_OPENGL_CORE
    // Not on mobile? We need to load the OpenGL function pointers
    glewExperimental = GL_TRUE;
    glewInit();
    glGetError();   // Apparently GLEW generates an error that we have to get rid of

#ifndef NDEBUG
    EnableDebugCallback();
#endif
#endif
    
    for (int i = 0; i < MAX_QUERY_SETS; i++) {
        glGenQueries(2, reinterpret_cast<GLuint*>(&_fullFrameQueries[i]));
    }
    glQueryCounter(_fullFrameQueries[_curFrame].StartQuery, GL_TIMESTAMP);

    GLint queryBits;
    glGetQueryiv(GL_TIMESTAMP, GL_QUERY_COUNTER_BITS, &queryBits);

    _timestampMask = 0;
    for (int i = 0; i < queryBits; i++) {
        _timestampMask |= 1 << i;
    }
}

void OpenGLDrawcallTimer::Start(UnityRenderingExtBeforeDrawCallParams * drawcallParams)
{
    DrawcallQuery drawcallQuery;

    if (_timerPool.empty()) {
        GLuint drawcallQueries[2];
        glGenQueries(2, drawcallQueries);

        if (glGetError() != GL_NO_ERROR) {
            Debug("Could not create a query objects");
            return;
        }

        drawcallQuery.StartQuery = drawcallQueries[0];
        drawcallQuery.EndQuery = drawcallQueries[1];

    } else {
        drawcallQuery = _timerPool.back();
        _timerPool.pop_back();
    }

    glQueryCounter(drawcallQuery.StartQuery, GL_TIMESTAMP);
    _curQuery = drawcallQuery;
    _timers[_curFrame][*drawcallParams].push_back(drawcallQuery);
}

void OpenGLDrawcallTimer::End()
{
    glQueryCounter(_curQuery.EndQuery, GL_TIMESTAMP);
    if (glGetError() != GL_NO_ERROR) { 
        Debug("glEndQuery error"); 
    }
}

void OpenGLDrawcallTimer::ResolveQueries()
{
    bool record = true;

    glGetIntegerv(GL_GPU_DISJOINT, reinterpret_cast<GLint*>(&_disjointQueries[_curFrame]));
    if (_disjointQueries[_curFrame] == GL_TRUE) {
        Debug("Disjoint! Throwing away current frame\n");
        record = false;
    }
    std::stringstream ss;

    glQueryCounter(_fullFrameQueries[_curFrame].EndQuery, GL_TIMESTAMP);

    auto& curFullFrameQuery = _fullFrameQueries[GetLastFrameIndex()];

    GLuint available = 0;
    if (record) {
        uint64_t frameStart = 0, frameEnd = 0;
        glGetQueryObjectui64v(curFullFrameQuery.StartQuery, GL_QUERY_RESULT, &frameStart);
        glGetQueryObjectui64v(curFullFrameQuery.EndQuery, GL_QUERY_RESULT, &frameEnd);

        frameStart = frameStart & _timestampMask;
        frameEnd = frameEnd & _timestampMask;

        _lastFrameTime = (frameEnd - frameStart);
        _lastFrameTime /= 1000000.0;

        if (_frameCounter % 30 == 0 && _lastFrameTime > 0) {
            ss << "The frame took " << _lastFrameTime << "ms total";
            Debug(ss.str().c_str());
        }

        glDeleteQueries(2, reinterpret_cast<GLuint*>(&curFullFrameQuery));
        glGenQueries(2, reinterpret_cast<GLuint*>(&curFullFrameQuery));
    }

    _shaderTimes.clear();
    // Collect raw GPU time for each shader
    for (const auto& shaderTimers : _timers[_curFrame]) {

        const auto& shader = shaderTimers.first;
        ShaderNames shaderNames;

        if (shader.vertexShader != nullptr) {
            GLuint* vertexShader = reinterpret_cast<GLuint*>(shader.vertexShader);
            shaderNames.Vertex = GetShaderName(vertexShader);
        }
        if (shader.geometryShader != nullptr) {
            GLuint* geometryShader = reinterpret_cast<GLuint*>(shader.geometryShader);
            shaderNames.Geometry = GetShaderName(geometryShader);
        }
        if (shader.domainShader != nullptr) {
            GLuint* domainShader = reinterpret_cast<GLuint*>(shader.domainShader);
            shaderNames.Domain = GetShaderName(domainShader);
        }
        if (shader.hullShader != nullptr) {
            GLuint* hullShader = reinterpret_cast<GLuint*>(shader.hullShader);
            shaderNames.Hull = GetShaderName(hullShader);
        }
        if (shader.fragmentShader != nullptr) {
            GLuint* pixelShader = reinterpret_cast<GLuint*>(shader.fragmentShader);
            shaderNames.Pixel = GetShaderName(pixelShader);
        }

        uint64_t shaderTime = 0;
        uint64_t drawcallTime = 0;
        for (const auto& timer : shaderTimers.second) {
            if (record) {
                GLuint startTime = 0, endTime = 0; 
                glGetQueryObjectuiv(timer.StartQuery, GL_QUERY_RESULT_AVAILABLE, &available);
                if (available) {
                    glGetQueryObjectuiv(timer.StartQuery, GL_QUERY_RESULT, &startTime);
                    startTime = startTime & _timestampMask;
                }

                glGetQueryObjectuiv(timer.EndQuery, GL_QUERY_RESULT_AVAILABLE, &available);
                if (available) {
                    glGetQueryObjectuiv(timer.EndQuery, GL_QUERY_RESULT, &endTime);
                    endTime = endTime & _timestampMask;
                }

                // If the query result isn't ready, we shouldn't try to use it
                if (startTime != 0 && endTime != 0) {
                    drawcallTime = (endTime - startTime) / 1000000.0;
                    shaderTime += drawcallTime;
                }
            }

            _timerPool.push_back(timer);
        }

        if (record) {
            _shaderTimes[shaderNames] = 1000 * double(shaderTime);

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

    // Erase the data so there's no garbage for next time 
    _timers[_curFrame].clear();

    // Start the next frame
    glQueryCounter(_fullFrameQueries[GetNextFrameIndex()].StartQuery, GL_TIMESTAMP);
}

#ifndef NDEBUG
void EnableDebugCallback() 
{
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    glDebugMessageCallback(OnOpenGLMessage, nullptr);
}

static std::string TranslateDebugSource(GLenum source) 
{
    switch (source) {
    case GL_DEBUG_SOURCE_API:
        return "API";
    
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        return "window system";
    
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        return "shader compiler";
    
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        return "third party";

    case GL_DEBUG_SOURCE_APPLICATION:
        return "application";

    case GL_DEBUG_SOURCE_OTHER:
    default:
        return "other";
    }
}

static std::string TranslateDebugType(GLenum type)
{
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        return "ERROR";

    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        return "DEPRECATED BEHAVIOR";

    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        return "UNDEFINED BEHAVIOUR";

    case GL_DEBUG_TYPE_PORTABILITY:
        return "PORTABILITY";

    case GL_DEBUG_TYPE_PERFORMANCE:
        return "PERFORMANCE";

    case GL_DEBUG_TYPE_MARKER:
        return "MARKER";

    case GL_DEBUG_TYPE_OTHER:
    default:
        return "OTHER";
    }
}

static std::string TranslateDebugSeverity(GLenum severity) 
{
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        return "high";

    case GL_DEBUG_SEVERITY_MEDIUM:
        return "medium";

    case GL_DEBUG_SEVERITY_LOW:
        return "low";

    case GL_DEBUG_SEVERITY_NOTIFICATION:
        return "notification";

    default:
        return "unknown";
    }
}

void GLAPIENTRY OnOpenGLMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
    const GLchar* message, const void* userParam) {
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
        // Don't want to print out notifications
        return;
    }

    std::stringstream ss;
    std::cout << "{\"severity\":\"" << TranslateDebugSeverity(severity) << "\",\"source\":\"" << TranslateDebugSource(source) << "\",\"type\":\"" << TranslateDebugType(type) << "\",\"object\":" << id << ",\"message\":\"" << message << "\"}";
    std::cout << std::endl;
}
#endif

std::string GetShaderName(GLuint* shader) 
{
    GLsizei shaderNameSize = 512;
    char* shaderName = new char[shaderNameSize];
    glGetObjectLabel(GL_SHADER, *shader, shaderNameSize, &shaderNameSize, shaderName);
    shaderName[shaderNameSize] = '\0';

    return { shaderName };
}

#endif