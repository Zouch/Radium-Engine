#pragma once

#include <Gui/RaGui.hpp>

#include <Core/CoreMacros.hpp>
#include <Core/Types.hpp>
#include <Engine/RadiumEngine.hpp>
#include <Engine/Rendering/Renderer.hpp>
#include <Engine/Scene/CameraComponent.hpp>
#include <Gui/Utils/KeyMappingManager.hpp>
#include <Gui/Viewer/WindowQt.hpp>

#include <atomic>
#include <memory>

#include <QMessageBox>
#include <QThread>
#include <QWindow>

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
class RA_GUI_API Viewer : public WindowQt, public KeyMappingManageable<Viewer>
{
    Q_OBJECT
    friend class KeyMappingManageable<Viewer>;

  public:
    /// Constructor
    explicit Viewer( QScreen* screen = nullptr );

    /// Destructor
    ~Viewer() override;

    /// add observers to keyMappingManager for gizmo, camera and viewer.
    virtual void setupKeyMappingCallbacks();

    /** Prepare the viewer to display a scene.
     * This will ask the renderer to build its specific OenGL related objects
     * and set the camera to its default state
     * @return true if the scene is ready to display.
     */
    virtual bool prepareDisplay();

    //
    // Accessors
    //
    /// Access to the OpenGL context of the Viewer
    QOpenGLContext* getContext() const { return m_context.get(); }

    /// Set the current camera interface.
    void setCameraManipulator( CameraManipulator* ci );

    /// Access to camera interface.
    CameraManipulator* getCameraManipulator();

    void resetToDefaultCamera();

    /// Access to gizmo manager
    GizmoManager* getGizmoManager();

    /// Read-only access to renderer
    const Engine::Rendering::Renderer* getRenderer() const;

    /// Read-write access to renderer
    Engine::Rendering::Renderer* getRenderer();

    /** Add a renderer and return its index. Need to be called when catching
     * \param e : your own renderer
     * \return index of the newly added renderer
     * \code
     * int rendererId = addRenderer(new MyRenderer(width(), height()));
     * changeRenderer(rendererId);
     * getRenderer()->initialize();
     * auto light = Ra::Core::make_shared<Engine::Data::DirectionalLight>();
     * m_camera->attachLight( light );
     * \endcode
     */
    int addRenderer( const std::shared_ptr<Engine::Rendering::Renderer>& e );

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

    /// @name
    /// Misc functions
    /// @{

    /// Emits signals corresponding to picking requests.
    void processPicking();

    /// Moves the camera so that the whole scene is visible.
    void fitCameraToScene( const Core::Aabb& sceneAabb );
    void fitCamera();
    /// Returns the names of the different registred renderers.
    std::vector<std::string> getRenderersName() const;

    /// Write the current frame as an image. Supports either BMP or PNG file names.
    void grabFrame( const std::string& filename );

    void enableDebug();

    /// get the currently used background color
    const Core::Utils::Color& getBackgroundColor() const { return m_backgroundColor; }

    /// Add a custom event handler on keyPressed event
    KeyMappingManager::KeyMappingAction
    addKeyPressEventAction( const std::string& actionName,
                            const std::string& keyString,
                            const std::string& modifiersString,
                            const std::string& buttonsString,
                            const std::string& wheelString,
                            std::function<void( QKeyEvent* )> callback );

    /// Add a custom event handler on keyRelease event
    KeyMappingManager::KeyMappingAction
    addKeyReleaseEventAction( const std::string& actionName,
                              const std::string& keyString,
                              const std::string& modifiersString,
                              const std::string& buttonsString,
                              const std::string& wheelString,
                              std::function<void( QKeyEvent* )> callback );
    ///@}

    Scalar getDepthUnderMouse() const { return m_depthUnderMouse; }
  signals:
    /// Emitted when GL context is ready and the engine OpenGL part must be initialized.
    /// Renderers might be added here using addRenderer.
    void requestEngineOpenGLInitialization();
    bool glInitialized(); //! Emitted when GL context is ready. We except call to addRenderer here
    void rendererReady(); //! Emitted when the rendered is correctly initialized
    void rightClickPicking( const Ra::Engine::Rendering::Renderer::PickingResult& result );
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
    void setBackgroundColor( const Core::Utils::Color& background );

    // Display help dialog about Viewer key-bindings
    void displayHelpDialog();

  private slots:
    /// These slots are connected to the base class signals to properly handle
    /// concurrent access to the renderer.
    void onAboutToCompose();
    void onAboutToResize();
    void onResized();
    void onFrameSwapped();

  protected:
    /// create gizmos
    void createGizmoManager();

    /// Initialize renderer internal state + configure lights.
    void initializeRenderer( Engine::Rendering::Renderer* renderer );

    /// Initialize openGL. Called on by the first "show" call to the main window.
    /// \warning This function is NOT reentrant, and may behave incorrectly
    /// if called at the same time than #intializeRenderer
    /// @note Must be called only when a valid openGLContext is bound (see WindowsQt::initialize)
    bool initializeGL() override;

    /// Resize the view port and the camera. Called by the resize event.
    void resizeGL( QResizeEvent* event ) override;

    Engine::Rendering::Renderer::PickingMode
    getPickingMode( const Ra::Gui::KeyMappingManager::KeyMappingAction& action ) const;

    /// @name
    /// If GL is initialized, do the viewer stuff, and call handle*Event to perform actual event
    /// handling, according to keyMapping. If event is not processed by Viewer, it's forwarded to
    /// parent() (if any).
    ///@{
    void keyPressEvent( QKeyEvent* event ) override;
    void keyReleaseEvent( QKeyEvent* event ) override;

    /// We intercept the mouse events in this widget to get the coordinates of the mouse
    /// in screen space.
    void mousePressEvent( QMouseEvent* event ) override;
    ///\todo propagateToParent
    void mouseReleaseEvent( QMouseEvent* event ) override;
    ///\todo propagateToParent
    void mouseMoveEvent( QMouseEvent* event ) override;
    ///\todo propagateToParent
    void wheelEvent( QWheelEvent* event ) override;
    ///@}

    void showEvent( QShowEvent* ev ) override;

    /// @name
    /// handle the events, called by *Event, do the actual work, should be overriden in
    /// derived classes.
    ///@{
    virtual bool handleKeyPressEvent( QKeyEvent* event );
    virtual bool handleKeyReleaseEvent( QKeyEvent* event );
    virtual void handleMousePressEvent( QMouseEvent* event,
                                        Ra::Engine::Rendering::Renderer::PickingResult& result );
    virtual void handleMouseReleaseEvent( QMouseEvent* event );
    virtual void handleMouseMoveEvent( QMouseEvent* event,
                                       Ra::Engine::Rendering::Renderer::PickingResult& result );
    virtual void handleWheelEvent( QWheelEvent* event );
    ///@}
  private:
    /// update keymapping according to keymapping manager's config, should be
    /// called each time the configuration changes, or added to observer's list
    /// with KeyMappingManager::addListener
    /// Called with KeyManageable::configureKeyMapping
    static void configureKeyMapping_impl();

    Ra::Engine::Rendering::Renderer::PickingResult pickAtPosition( Core::Vector2 position );

    void propagateEventToParent( QEvent* event );

    KeyMappingManager::KeyMappingAction
    addCustomAction( int index,
                     const std::string& actionName,
                     const std::string& keyString,
                     const std::string& modifiersString,
                     const std::string& buttonsString,
                     const std::string& wheelString,
                     std::function<void( QKeyEvent* )> callback );

    std::tuple<KeyMappingManager::KeyMappingAction,
               KeyMappingManager::KeyMappingAction,
               KeyMappingManager::KeyMappingAction>
    getComponentActions( const Qt::MouseButtons& buttons,
                         const Qt::KeyboardModifiers& modifiers,
                         int key,
                         bool wheel );

    Scalar m_depthUnderMouse;
    std::unique_ptr<QMessageBox> m_helpDialog { nullptr };

  protected:
    ///\todo make the following  private:
    /// Owning pointer to the renderers.
    std::vector<std::shared_ptr<Engine::Rendering::Renderer>> m_renderers;
    std::vector<std::shared_ptr<Engine::Rendering::Renderer>> m_pendingRenderers;
    Engine::Rendering::Renderer* m_currentRenderer;

    /// Owning Pointer to the feature picking manager.
    PickingManager* m_pickingManager;

    bool m_isBrushPickingEnabled;
    Scalar m_brushRadius;

    /// Owning pointer to the camera.
    std::unique_ptr<CameraManipulator> m_camera;

    /// Owning (QObject child) pointer to gizmo manager.
    GizmoManager* m_gizmoManager;

    Core::Utils::Color m_backgroundColor { Core::Utils::Color::Grey( 0.0392_ra, 0_ra ) };

    /// Name of the customisable key actions
    enum KeyEventType { KeyPressed = 0, KeyReleased, KeyEventTypeCount };
    /// Array of custom key event handler
    /// Index is KeyEventType
    std::array<std::map<Core::Utils::Index, std::function<void( QKeyEvent* )>>, KeyEventTypeCount>
        m_customKeyActions;

    KeyMappingManager::Context m_activeContext {};
#define KeyMappingViewer                     \
    KMA_VALUE( VIEWER_PICKING )              \
    KMA_VALUE( VIEWER_PICKING_VERTEX )       \
    KMA_VALUE( VIEWER_PICKING_EDGE )         \
    KMA_VALUE( VIEWER_PICKING_TRIANGLE )     \
    KMA_VALUE( VIEWER_PICKING_MULTI_CIRCLE ) \
    KMA_VALUE( VIEWER_RAYCAST )              \
    KMA_VALUE( VIEWER_SCALE_BRUSH )          \
    KMA_VALUE( VIEWER_RELOAD_SHADERS )       \
    KMA_VALUE( VIEWER_TOGGLE_WIREFRAME )     \
    KMA_VALUE( VIEWER_SWITCH_CAMERA )        \
    KMA_VALUE( VIEWER_CAMERA_FIT_SCENE )     \
    KMA_VALUE( VIEWER_HELP )

#define KMA_VALUE( x ) static KeyMappingManager::KeyMappingAction x;
    KeyMappingViewer
#undef KMA_VALUE
};

} // namespace Gui
} // namespace Ra
