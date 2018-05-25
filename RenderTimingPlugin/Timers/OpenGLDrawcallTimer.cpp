#include "OpenGLDrawcallTimer.h"
#if SUPPORT_OPENGL_UNIFIED

#include <sstream>
#include <iostream>

std::string GetShaderName(const GLuint* shader);

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

    for (auto &fullFrameQuery : _fullFrameQueries) {
        glGenQueries(2, reinterpret_cast<GLuint*>(&fullFrameQuery));
    }
    glBeginQuery(GL_TIME_ELAPSED, _fullFrameQueries[_curFrame].StartQuery);

    GLint queryBits = 32;
#if SUPPORT_OPENGL_CORE
    glGetQueryiv(GL_TIMESTAMP, GL_QUERY_COUNTER_BITS, &queryBits);
#endif

    _timestampMask = 0;
    for (uint32_t i = 0; i < (uint32_t)queryBits; i++) {
        _timestampMask |= 1U << i;
    }
}

void OpenGLDrawcallTimer::Start(UnityRenderingExtBeforeDrawCallParams * drawcallParams)
{
#if SUPPORT_OPENGL_CORE
    DrawcallQuery drawcallQuery = {};

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
#endif
}

void OpenGLDrawcallTimer::End()
{
#if SUPPORT_OPENGL_CORE
    glQueryCounter(_curQuery.EndQuery, GL_TIMESTAMP);
    if (glGetError() != GL_NO_ERROR) { 
        Debug("glEndQuery error"); 
    }
#endif
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

#if SUPPORT_OPENGL_CORE
    glQueryCounter(_fullFrameQueries[_curFrame].EndQuery, GL_TIMESTAMP);
#else
    glEndQuery(GL_TIME_ELAPSED);
#endif

    auto& curFullFrameQuery = _fullFrameQueries[GetLastFrameIndex()];

    GLuint available = 0;
    if (record) {
#if SUPPORT_OPENGL_CORE
        uint64_t frameStart = 0, frameEnd = 0;
        glGetQueryObjectui64v(curFullFrameQuery.StartQuery, GL_QUERY_RESULT, &frameStart);
        glGetQueryObjectui64v(curFullFrameQuery.EndQuery, GL_QUERY_RESULT, &frameEnd);

        frameStart = frameStart & _timestampMask;
        frameEnd = frameEnd & _timestampMask;

        _lastFrameTime = static_cast<double>(frameEnd - frameStart);

#else
        uint32_t frameStart = 0;
        glGetQueryObjectuiv(curFullFrameQuery.StartQuery, GL_QUERY_RESULT, &frameStart);
        _lastFrameTime = static_cast<double>(frameStart);

        glDeleteQueries(1, &curFullFrameQuery.StartQuery);
        glGenQueries(1, &curFullFrameQuery.StartQuery);
#endif

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
            auto * vertexShader = reinterpret_cast<GLuint*>(shader.vertexShader);
            shaderNames.Vertex = GetShaderName(vertexShader);
        }
        if (shader.geometryShader != nullptr) {
            auto* geometryShader = reinterpret_cast<GLuint*>(shader.geometryShader);
            shaderNames.Geometry = GetShaderName(geometryShader);
        }
        if (shader.domainShader != nullptr) {
            auto* domainShader = reinterpret_cast<GLuint*>(shader.domainShader);
            shaderNames.Domain = GetShaderName(domainShader);
        }
        if (shader.hullShader != nullptr) {
            auto* hullShader = reinterpret_cast<GLuint*>(shader.hullShader);
            shaderNames.Hull = GetShaderName(hullShader);
        }
        if (shader.fragmentShader != nullptr) {
            auto* pixelShader = reinterpret_cast<GLuint*>(shader.fragmentShader);
            shaderNames.Pixel = GetShaderName(pixelShader);
        }

        double shaderTime = 0;
        double drawcallTime = 0;
        for (const auto& timer : shaderTimers.second) {
            if (record) {
                GLuint startTime = 0, endTime = 0; 
                glGetQueryObjectuiv(timer.StartQuery, GL_QUERY_RESULT_AVAILABLE, &available);
                if (available) {
                    glGetQueryObjectuiv(timer.StartQuery, GL_QUERY_RESULT, &startTime);
                    startTime = static_cast<GLuint>(startTime & _timestampMask);
                }

                glGetQueryObjectuiv(timer.EndQuery, GL_QUERY_RESULT_AVAILABLE, &available);
                if (available) {
                    glGetQueryObjectuiv(timer.EndQuery, GL_QUERY_RESULT, &endTime);
                    endTime = static_cast<GLuint>(endTime & _timestampMask);
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
            _shaderTimes[shaderNames] = 1000 * shaderTime;

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
    glBeginQuery(GL_TIME_ELAPSED, _fullFrameQueries[GetNextFrameIndex()].StartQuery);
}

#ifndef NDEBUG
#ifdef SUPPORT_OPENGL_CORE
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
    std::cout << R"({"severity":")" << TranslateDebugSeverity(severity) << R"(","source":")" << TranslateDebugSource(source)
              << R"(","type":")" << TranslateDebugType(type) << R"(","object":)" << id << R"(,"message":")" << message << R"("})";
    std::cout << std::endl;
}
#endif
#endif

std::string GetShaderName(const GLuint* shader)
{
#if SUPPORT_OPENGL_CORE
    GLsizei shaderNameSize = 512;
    auto * shaderName = new char[shaderNameSize];
    glGetObjectLabel(GL_SHADER, *shader, shaderNameSize, &shaderNameSize, shaderName);
    shaderName[shaderNameSize] = '\0';

    return { shaderName };
#else
    std::stringstream ss;
    ss << *shader;
    return ss.str();
#endif
}

#endif