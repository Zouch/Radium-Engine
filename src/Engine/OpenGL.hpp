#pragma once

/// This file provides portable inclusion of OpenGL headers.
/// In addition it defines the following debug macros
/// GL_ASSERT(x) : executes code x and, if in debug, asserts when an openGL error is detected.
/// GL_CHECK_ERROR : queries the last openGL error in debug (no effect in release)
/// glFlushError() : Ignores the previous openGL errors (no effect in release).

#include <glbinding/gl45core/gl.h>
#include <glbinding/gl45ext/gl.h>
namespace Ra {
namespace Engine {
// use glbinding namespace only in Ra::Engine to avoid namespace clash with e.g. Qt.
using namespace gl45core;
using namespace gl45ext;
using namespace gl;
} // namespace Engine
} // namespace Ra

#include <glbinding-aux/types_to_string.h>

// use always float on the OpenGL side, since double have some unresolved func with globjects.
// maybe fix this to allow GL_DOUBLE ? in this change data conversion in color or mesh attrib gpu
// data transfer.
#define GL_SCALAR GL_FLOAT
#define GL_SCALAR_PLAIN float

/// Checks that an openGLContext is available (mostly for debug checks and asserts).
inline bool checkOpenGLContext() {
    return glGetString( gl::GL_VERSION ) != nullptr;
}

/// Gets the openGL error string (emulates gluErrorString())
inline const char* glErrorString( gl::GLenum err ) {
    using namespace gl45core;
    using namespace gl45ext;
    using namespace gl;
    switch ( err ) {
    case GL_INVALID_ENUM:
        return " Invalid enum : An unacceptable value is specified for an enumerated argument. The "
               "offending command is ignored and has no other side effect than to set the error "
               "flag.\n";
    case GL_INVALID_VALUE:
        return " Invalid value : A numeric argument is out of range. The offending command is "
               "ignored and has no other side effect than to set the error flag.\n";
    case GL_INVALID_OPERATION:
        return " Invalid operation : The specified operation is not allowed in the current state. "
               "The offending command is ignored and has no other side effect than to set the "
               "error flag.\n";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return " Invalid framebuffer operation : The framebuffer object is not complete. The "
               "offending command is ignored and has no other side effect than to set the error "
               "flag.\n";
    case GL_OUT_OF_MEMORY:
        return " Out of memory : There is not enough memory left to execute the command. The state "
               "of the GL is undefined, except for the state of the error flags, after this error "
               "is recorded.\n";
    // case GL_STACK_UNDERFLOW:
    //    return " Stack underflow : An attempt has been made to perform an operation that would
    //    cause an internal stack to underflow.\n";
    // case GL_STACK_OVERFLOW:
    //    return " Stack overflow : An attempt has been made to perform an operation that would
    //    cause an internal stack to overflow.\n";
    case GL_NO_ERROR:
        return " No error\n";
    default:
        return " Unknown GL error\n";
    }
}

#ifdef _DEBUG
#    include <Core/Utils/Log.hpp>
#    include <Core/Utils/StackTrace.hpp>
#    define GL_ASSERT( x )                                                                       \
        x;                                                                                       \
        {                                                                                        \
            gl::GLenum err = glGetError();                                                       \
            if ( err != gl::GL_NO_ERROR ) {                                                      \
                const char* errBuf = glErrorString( err );                                       \
                LOG( Ra::Core::Utils::logERROR )                                                 \
                    << "OpenGL error (" << __FILE__ << ":" << __LINE__ << ", " << STRINGIFY( x ) \
                    << ") : " << errBuf << "(" << err << " : 0x" << std::hex << int( err )       \
                    << std::dec << ").";                                                         \
                BREAKPOINT( 0 );                                                                 \
            }                                                                                    \
        }

/// This macro will query the last openGL error.
#    define GL_CHECK_ERROR                                                                   \
        {                                                                                    \
            gl::GLenum err = glGetError();                                                   \
            if ( err != gl::GL_NO_ERROR ) {                                                  \
                const char* errBuf = glErrorString( err );                                   \
                LOG( Ra::Core::Utils::logERROR )                                             \
                    << "OpenGL error (" << __FILE__ << ":" << __LINE__                       \
                    << ", glCheckError()) : " << errBuf << "(" << err << " : 0x" << std::hex \
                    << err << std::dec << ")." << '\n'                                       \
                    << Ra::Core::Utils::StackTrace();                                        \
                BREAKPOINT( 0 );                                                             \
            }                                                                                \
        }

/// Ignore the previous openGL errors.
#    define glFlushError() glGetError()

#else // Release version ignores the checks and errors.
#    define GL_ASSERT( x ) x
#    define GL_CHECK_ERROR \
        {}
#    define glFlushError() \
        {}
#endif // _DEBUG
