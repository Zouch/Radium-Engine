// This file si picked and slightly adapted from
// globjects/source/examples/qtexample/WindowQt.h
// so that update in globjects is easy
// Radium Viewer derive this class

#pragma once
#include <GuiBase/RaGuiBase.hpp>

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
class RA_GUIBASE_API WindowQt : public QWindow
{
  public:
    explicit WindowQt( QWindow* parent );
    virtual ~WindowQt();

    virtual void render();

    virtual void initialize();

    void resizeEvent( QResizeEvent* event ) override;
    void showEvent( QShowEvent* event ) override;
    void exposeEvent( QExposeEvent* event ) override;
    // bool event( QEvent* event ) override;

    virtual void enterEvent( QEvent* event );
    virtual void leaveEvent( QEvent* event );

    /**
     * Make the OpenGL context associated with the viewer the current context.
     */

    void makeCurrent();

    /**
     * Reset the current OpenGL context that is no more the one associated with the viewer.
     */
    void doneCurrent();

    QOpenGLContext* context();

    // note when updating from globjets
    // updateGL done by base app rendering loop
    bool isOpenGLInitialized() const { return m_glInitialized.load(); }

  public slots:
    /// call deinitializeGL if needed, with context activated
    void cleanupGL();
    void screenChanged();

  protected:
    // OpenglContext used with this widget
    std::unique_ptr<QOpenGLContext> m_context;

    bool m_updatePending;
    std::atomic_bool m_glInitialized;

    void resizeInternal( QResizeEvent* event );

    // note when updating from globjets
    // paintGL done by base app rendering loop

    virtual bool initializeGL();
    virtual void deinitializeGL();
    virtual void resizeGL( QResizeEvent* event );

    // note when updating from globjets
    // paintGL done by base app rendering loop

  protected:
    static WindowQt* s_getProcAddressHelper;
    static glbinding::ProcAddress getProcAddress( const char* name );
};

} // namespace Gui
} // namespace Ra
