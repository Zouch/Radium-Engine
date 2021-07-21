#include "WindowQt.hpp"

#include <QApplication>
#include <QDebug>
#include <QOpenGLContext>
#include <QResizeEvent>
#include <QScreen>
#include <QSurfaceFormat>

#include <Core/Utils/Log.hpp>

using namespace Ra::Core::Utils; // log

namespace Ra {
namespace Gui {

WindowQt* WindowQt::s_getProcAddressHelper = nullptr;

QSurfaceFormat defaultFormat() {
    QSurfaceFormat format;
    format.setProfile( QSurfaceFormat::CoreProfile );
#ifndef NDEBUG
    format.setOption( QSurfaceFormat::DebugContext );
#endif
    return format;
}

WindowQt::WindowQt( QScreen* screen ) :
    QWindow( screen ), m_context( nullptr ), m_updatePending( false ), m_glInitialized( false ) {

    setSurfaceType( QWindow::OpenGLSurface );
    if ( !s_getProcAddressHelper ) { s_getProcAddressHelper = this; }

    // No need to create as this window is used as widget (and actually segfault on Qt 5.12.3)
    // Surface format set in BaseApplication

    m_context = std::make_unique<QOpenGLContext>( this );
    m_context->setFormat( QSurfaceFormat::defaultFormat() );

    if ( !m_context->create() ) {
        LOG( logINFO ) << "Could not create OpenGL context.";
        QApplication::quit();
    }

    connect( this, &QWindow::screenChanged, this, &WindowQt::screenChanged );

    // cleanup connection is set in BaseApplication
}

WindowQt::~WindowQt() {
    // cannot deinitialize OpenGL here as it would require the call of a virtual member function
}

void WindowQt::screenChanged() {
    QSize s { size().width(), size().height() };
    QResizeEvent patchEvent { s, s };
    resizeInternal( &patchEvent );
}

QOpenGLContext* WindowQt::context() {
    return m_context.get();
}

void WindowQt::makeCurrent() {
    if ( QOpenGLContext::currentContext() != m_context.get() ) {
        m_context->makeCurrent( this );
        // reset counter (in case another viewer has broken our context activation counter)
        m_contextActivationCount = 0;
    }
    else {
        ++m_contextActivationCount;
    }
}

void WindowQt::doneCurrent() {
    if ( m_contextActivationCount == 0 ) { m_context->doneCurrent(); }
    else {
        --m_contextActivationCount;
    }
}

void WindowQt::resizeEvent( QResizeEvent* event ) {
    resizeInternal( event );
}

void WindowQt::exposeEvent( QExposeEvent* ) {
    initialize();
}

void WindowQt::initialize() {
    if ( !m_glInitialized.load() ) {
        makeCurrent();
        initializeGL();
        doneCurrent();
    }
}

void WindowQt::showEvent( QShowEvent* /*ev*/ ) {
    initialize();
}

void WindowQt::resizeInternal( QResizeEvent* event ) {
#ifdef OS_MACOS
    // Ugly patch since Qt seems buggy on this point on macos, raise two resize call the first time.
    if ( event->size().width() < minimumSize().width() ||
         event->size().height() < minimumSize().height() ) {
        QSize size { std::max( event->size().width(), minimumSize().width() ),
                     std::max( event->size().height(), minimumSize().height() ) };
        QResizeEvent* patchEvent = new QResizeEvent( size, event->oldSize() );
        event                    = patchEvent;
        QWindow::resize( size );
    }
#endif

    initialize();

    makeCurrent();

    // do not take into account devicePixelRatio(), we work on ldi resolution in our GL world.
    resizeGL( event );

    doneCurrent();
}
/// paint is done by main rendering loop, initialize instead
/*
bool WindowQt::event( QEvent* event ) {
    switch ( event->type() )
    {
    case QEvent::UpdateRequest:
        //        paint();
        return true;

    case QEvent::Enter:
        enterEvent( event );
        return true;

    case QEvent::Leave:
        leaveEvent( event );
        return true;

    default:
        return QWindow::event( event );
    }
}*/

bool WindowQt::initializeGL() {
    // this simple window do not init GL
    m_glInitialized = false;
    return m_glInitialized;
}

void WindowQt::cleanupGL() {
    if ( m_glInitialized.load() ) {
        makeCurrent();

        deinitializeGL();

        doneCurrent();
    }
}

void WindowQt::deinitializeGL() {}

void WindowQt::resizeGL( QResizeEvent* ) {}

/// paintgl is done by main rendering loop

void WindowQt::enterEvent( QEvent* ) {}

void WindowQt::leaveEvent( QEvent* ) {}

glbinding::ProcAddress WindowQt::getProcAddress( const char* name ) {
    if ( !s_getProcAddressHelper || name == nullptr ) { return nullptr; }

    const auto symbol = std::string( name );

#if ( QT_VERSION >= QT_VERSION_CHECK( 5, 4, 0 ) )
    const auto qtSymbol = QByteArray::fromStdString( symbol );
#else
    const auto qtSymbol = QByteArray::fromRawData( symbol.c_str(), symbol.size() );
#endif
    return s_getProcAddressHelper->m_context->getProcAddress( qtSymbol );
}

} // namespace Gui
} // namespace Ra
