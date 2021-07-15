#include "OpenGLContext.hpp"

#include <glbinding-aux/ContextInfo.h>
#include <glbinding-aux/types_to_string.h>
#include <glbinding/Binding.h>

// Do not import namespace to prevent glbinding/QTOpenGL collision
#include <glbinding/gl/gl.h>

#include <globjects/globjects.h>

#include <GLFW/glfw3.h>

#include <iostream>

static void error( int errnum, const char* errmsg ) {
    globjects::critical() << errnum << ": " << errmsg << std::endl;
}

OpenGLContext::OpenGLContext( const std::array<int, 2>& size ) {
    // initialize openGL
    if ( glfwInit() )
    {
        glfwSetErrorCallback( error );
        glfwDefaultWindowHints();
        glfwWindowHint( GLFW_VISIBLE, false );
        glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
        glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
        glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
        glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
        m_offscreenContext =
            glfwCreateWindow( size[0], size[1], "Radium CommandLine Context", nullptr, nullptr );
    }
    if ( m_offscreenContext == nullptr )
    {
        globjects::critical() << "Context creation failed. Terminate execution.";
        glfwTerminate();
    }
    else
    {
        // Initialize globjects (internally initializes glbinding, and registers the current
        // context)
        glfwMakeContextCurrent( m_offscreenContext );
        globjects::init( []( const char* name ) { return glfwGetProcAddress( name ); } );
    }
}
OpenGLContext::~OpenGLContext() {
    glfwTerminate();
}
void OpenGLContext::makeCurrent() const {
    if ( m_offscreenContext ) { glfwMakeContextCurrent( m_offscreenContext ); }
}

void OpenGLContext::doneCurrent() const {
    if ( m_offscreenContext ) { glfwMakeContextCurrent( nullptr ); }
}

bool OpenGLContext::isValid() const {
    return m_offscreenContext != nullptr;
}

std::string OpenGLContext::getInfo() const {
    std::stringstream infoText;
    makeCurrent();
    infoText << "*** OffScreen OpenGL context ***" << std::endl;
    infoText << "Renderer (glbinding) : " << glbinding::aux::ContextInfo::renderer() << std::endl;
    infoText << "Vendor   (glbinding) : " << glbinding::aux::ContextInfo::vendor() << std::endl;
    infoText << "OpenGL   (glbinding) : " << glbinding::aux::ContextInfo::version().toString()
             << std::endl;
    infoText << "GLSL                 : "
             << gl::glGetString( gl::GLenum( GL_SHADING_LANGUAGE_VERSION ) ) << std::endl;
    doneCurrent();

    return infoText.str();
}

void OpenGLContext::show() {
    glfwShowWindow( m_offscreenContext );
}

void OpenGLContext::hide() {
    glfwHideWindow( m_offscreenContext );
}

void OpenGLContext::resize( const std::array<int, 2>& size ) {
    glfwSetWindowSize( m_offscreenContext, size[0], size[1] );
}

void OpenGLContext::swapbuffers() {
    glfwSwapBuffers( m_offscreenContext );
    glfwPollEvents();
}

void OpenGLContext::waitForClose() {
    while ( !glfwWindowShouldClose( m_offscreenContext ) )
    {
        glfwPollEvents();
    }
}
