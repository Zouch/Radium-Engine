#ifndef RADIUMENGINE_VIEWER_HPP
#define RADIUMENGINE_VIEWER_HPP

#include <GuiBase/RaGuiBase.hpp>

#include <atomic>
#include <memory>

#include <QWindow>

#include <QThread>

#include <Core/CoreMacros.hpp>
#include <Core/Types.hpp>

#include <Engine/RadiumEngine.hpp>
#include <Engine/Renderer/Renderer.hpp>

#include <GuiBase/Utils/KeyMappingManager.hpp>
#include <GuiBase/Viewer/WindowQt.hpp>
// Forward declarations
class QOpenGLContext;
class QSurfaceFormat;

namespace Ra {
namespace Gui {
class CameraManipulator;
class GizmoManager;
class PickingManager;
} // namespace Gui
} // namespace Ra

namespace Ra {
namespace Gui {

/** The Viewer is the main display class. It could be used as an independant window or
 * can be set as a central widget on a more complex gui by using the adapter from QWindow to QWidget
 * To do that, the following code could be used :
 * \code{.cpp}
 *     m_viewer = new Ra::Gui::Viewer();
 *     QWidget * viewerwidget = QWidget::createWindowContainer(m_viewer);
 *     setCentralWidget(viewerwidget);
 * \endcode
 * Whatever its usage (QWindow or QWidget) the Viewer has the same funtionalities.
 * Its acts as a bridge between the interface, the engine and the renderer
 * Among its responsibilities are :
 * * Owning the renderer and camera, and managing their lifetime.
 * * setting up the renderer and camera by keeping it informed of interfaces changes
 *  (e.g. resize).
 * * catching user interaction (mouse clicks) at the lowest level and forward it to
 * the camera and the rest of the application
 * * Expose the asynchronous rendering interface
 */
class RA_GUIBASE_API Viewer : public WindowQt, public KeyMappingManageable<Viewer>
{
    Q_OBJECT
    friend class KeyMappingManageable<Viewer>;

  public:
    /// Constructor
    explicit Viewer( QWindow* parent = nullptr );

    /// Destructor
    ~Viewer() override;

    /// add observers to keyMappingManager for gizmo, camera and viewer.
    void setupKeyMappingCallbacks();

    //
    // Accessors
    //
    /// Access to the OpenGL context of the Viewer
    QOpenGLContext* getContext() const { return m_context.get(); }

    /// Access to camera interface.
    CameraManipulator* getCameraManipulator();

    /// Set the current camera interface.
    void setCameraManipulator( CameraManipulator* ci );

    /// Set the camera managed by the cameraInterface
    void setCamera( Engine::Camera* camera );

    /// Access to gizmo manager
    GizmoManager* getGizmoManager();

    /// Read-only access to renderer
    const Engine::Renderer* getRenderer() const;

    /// Read-write access to renderer
    Engine::Renderer* getRenderer();

    /// Access to the feature picking manager
    PickingManager* getPickingManager();

    //
    //  Time dependent state management
    //
    /// Update the internal viewer state to the (application) time dt
    void update( const Scalar dt );
    //
    // Rendering management
    //

    /// Start rendering (potentially asynchronously in a separate thread)
    void startRendering( const Scalar dt );

    /// Blocks until rendering is finished.
    void swapBuffers();

    //
    // Misc functions
    //

    /// Emits signals corresponding to picking requests.
    void processPicking();

    /// Moves the camera so that the whole scene is visible.
    void fitCameraToScene( const Core::Aabb& sceneAabb );

    /// Returns the names of the different registred renderers.
    std::vector<std::string> getRenderersName() const;

    /// Write the current frame as an image. Supports either BMP or PNG file names.
    void grabFrame( const std::string& filename );

    void enableDebug();

  signals:
    bool glInitialized(); //! Emitted when GL context is ready. We except call to addRenderer here
    void rendererReady(); //! Emitted when the rendered is correctly initialized
    void rightClickPicking( const Ra::Engine::Renderer::PickingResult& result );
    //! Emitted when the resut of a right click picking is known (for selection)

    void toggleBrushPicking(
        bool on ); //! Emitted when the corresponding key is released (see keyReleaseEvent)

    void needUpdate();

  public slots:
    /// Tell the renderer to reload all shaders.
    void reloadShaders();

    /// Set the final display texture
    void displayTexture( const QString& tex );

    /// Set the renderer
    bool changeRenderer( int index );

    /// Toggle the post-process effetcs
    void enablePostProcess( int enabled );

    /// Toggle the debug drawing
    void enableDebugDraw( int enabled );

    /** Add a renderer and return its index. Need to be called when catching
     * \param e : unique_ptr to your own renderer
     * \return index of the newly added renderer
     * \code
     * int rendererId = addRenderer(new MyRenderer(width(), height()));
     * changeRenderer(rendererId);
     * getRenderer()->initialize();
     * auto light = Ra::Core::make_shared<Engine::DirectionalLight>();
     * m_camera->attachLight( light );
     * \endcode
     */
    int addRenderer( const std::shared_ptr<Engine::Renderer>& e );

    void setBackgroundColor( const Core::Utils::Color& background );
    const Core::Utils::Color& getBackgroundColor() const { return m_backgroundColor; }

  private slots:
    /// These slots are connected to the base class signals to properly handle
    /// concurrent access to the renderer.
    void onAboutToCompose();
    void onAboutToResize();
    void onFrameSwapped();
    void onResized();

  protected:
    /// create gizmos
    void createGizmoManager();

    /// Initialize renderer internal state + configure lights.
    void initializeRenderer( Engine::Renderer* renderer );

    //
    // OpenGL primitives
    // Not herited, defined here in the same way QOpenGLWidget define them.
    //

    /// Initialize openGL. Called on by the first "show" call to the main window.
    /// \warning This function is NOT reentrant, and may behave incorrectly
    /// if called at the same time than #intializeRenderer
    bool initializeGL() override;

    /// Resize the view port and the camera. Called by the resize event.
    void resizeGL( QResizeEvent* event ) override;

    void keyPressEvent( QKeyEvent* event ) override;
    void keyReleaseEvent( QKeyEvent* event ) override;

    Engine::Renderer::PickingMode
    getPickingMode( const Ra::Gui::KeyMappingManager::KeyMappingAction& action ) const;
    /// We intercept the mouse events in this widget to get the coordinates of the mouse
    /// in screen space.
    void mousePressEvent( QMouseEvent* event ) override;
    void mouseReleaseEvent( QMouseEvent* event ) override;
    void mouseMoveEvent( QMouseEvent* event ) override;
    void wheelEvent( QWheelEvent* event ) override;
    void showEvent( QShowEvent* ev ) override;

  private:
    /// update keymapping according to keymapping manager's config, should be
    /// called each time the configuration changes, or added to observer's list
    /// with KeyMappingManager::addListener
    /// Called with KeyManageable::configureKeyMapping
    static void configureKeyMapping_impl();

  public:
    Scalar m_dt{0.1_ra};

  protected:
    /// Owning pointer to the renderers.
    std::vector<std::shared_ptr<Engine::Renderer>> m_renderers;
    Engine::Renderer* m_currentRenderer;

    /// Owning Pointer to the feature picking manager.
    PickingManager* m_pickingManager;
    bool m_isBrushPickingEnabled;
    float m_brushRadius;

    /// Owning pointer to the camera.
    std::unique_ptr<CameraManipulator> m_camera;

    /// Owning (QObject child) pointer to gizmo manager.
    GizmoManager* m_gizmoManager;

#ifdef RADIUM_MULTITHREAD_RENDERING
    // TODO are we really use this ? Remove if we do not plan to do multi thread rendering
    /// Thread in which rendering is done.
    [[deprecated]] QThread* m_renderThread = nullptr; // We have to use a QThread for MT rendering
#endif

    Core::Utils::Color m_backgroundColor{Core::Utils::Color::Grey( 0.0392_ra, 0_ra )};

    KeyMappingManager::Context m_activeContext{};
#define KeyMappingViewer                     \
    KMA_VALUE( VIEWER_PICKING )              \
    KMA_VALUE( VIEWER_PICKING_VERTEX )       \
    KMA_VALUE( VIEWER_PICKING_EDGE )         \
    KMA_VALUE( VIEWER_PICKING_TRIANGLE )     \
    KMA_VALUE( VIEWER_PICKING_MULTI_CIRCLE ) \
    KMA_VALUE( VIEWER_RAYCAST )              \
    KMA_VALUE( VIEWER_SCALE_BRUSH )          \
    KMA_VALUE( VIEWER_RELOAD_SHADERS )       \
    KMA_VALUE( VIEWER_TOGGLE_WIREFRAME )

#define KMA_VALUE( x ) static KeyMappingManager::KeyMappingAction x;
    KeyMappingViewer
#undef KMA_VALUE
};

} // namespace Gui
} // namespace Ra

#endif // RADIUMENGINE_VIEWER_HPP
