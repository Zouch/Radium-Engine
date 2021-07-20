// This file si picked and slightly adapted from
// globjects/source/examples/qtexample/WindowQt.h
// so that update in globjects is easy
// Radium Viewer derive this class

#pragma once
#include <Gui/RaGui.hpp>

#include <QScopedPointer>
#include <QWindow>

#include <glbinding/ProcAddress.h>

#include <atomic>

class QSurfaceFormat;
class QOpenGLContext;

namespace Ra {
namespace Gui {

/*!
 * \brief Base class for OpenGL widgets, compatble with Qt and globjects/glbindings.
 * \note This class has been extracted from globjects example
 * https://github.com/cginternals/globjects/blob/master/source/examples/qtexample/WindowQt.h
 */
class RA_GUI_API WindowQt : public QWindow
{
  public:
    explicit WindowQt( QScreen* screen );
    virtual ~WindowQt();

    void resizeEvent( QResizeEvent* event ) override;
    void showEvent( QShowEvent* event ) override;
    void exposeEvent( QExposeEvent* event ) override;
    // bool event( QEvent* event ) override;

    virtual void enterEvent( QEvent* event );
    virtual void leaveEvent( QEvent* event );

    /**
     * Make this->context() the current context.
     * This function is made reentrant thanks to an internal counter that count how many time the
     * context has been asked to be activated.
     * If the context have already been made current with a previous call to makeCurrent, this
     * function increase an internal counter by one so that doneCurrent do not release the context.
     */
    void makeCurrent();

    /**
     * Release this->context().
     * This results in no context being current in the current thread.
     * In case a context has been made current multiple times, this function just deacrease the
     * internal counter by one. \see makeCurrent()
     */
    void doneCurrent();

    QOpenGLContext* context();

    // note when updating from globjets
    // updateGL done by base app rendering loop

    bool isOpenGlInitialized() const { return m_glInitialized.load(); }

  public slots:
    /// call deinitializeGL if needed, with context activated
    void cleanupGL();
    void screenChanged();

  protected:
    // OpenglContext used with this widget
    std::unique_ptr<QOpenGLContext> m_context;

    bool m_updatePending;
    std::atomic_bool m_glInitialized;

    void initialize();
    void resizeInternal( QResizeEvent* event );

    //
    // OpenGL related methods
    // Not inherited, defined here in the same way QOpenGLWidget define them.
    //
    /// Initialize the openGL related part of the Window
    virtual bool initializeGL();
    /// DeInitialize the OpenGL
    virtual void deinitializeGL();
    /// Resize the OpenGL related part of the Window
    virtual void resizeGL( QResizeEvent* event );

    // note when updating from globjets
    // paintGL done by base app rendering loop

  protected:
    static WindowQt* s_getProcAddressHelper;
    static glbinding::ProcAddress getProcAddress( const char* name );

  private:
    int m_contextActivationCount { 0 };
};

} // namespace Gui
} // namespace Ra
