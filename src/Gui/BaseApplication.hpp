#pragma once
#include <atomic>
#include <chrono>
#include <memory>
#include <vector>

#include <QApplication>

#include <Core/Utils/Timer.hpp>
#include <Gui/TimerData/FrameTimerData.hpp>
#include <PluginBase/RadiumPluginInterface.hpp>

class QTimer;
class QCommandLineParser;
namespace Ra {
namespace Engine {
class RadiumEngine;
namespace Scene {
class GeometrySystem;
struct ItemEntry;
} // namespace Scene
} // namespace Engine

namespace Gui {
class Viewer;
class MainWindowInterface;

/// This class contains the main application logic. It owns the engine and the GUI.
class RA_GUI_API BaseApplication : public QApplication
{
    Q_OBJECT

  public:
    class WindowFactory
    {
      public:
        WindowFactory()                                                = default;
        virtual ~WindowFactory()                                       = default;
        virtual Ra::Gui::MainWindowInterface* createMainWindow() const = 0;
    };

    /** Setup the application, create main window and main connections.
     * \param argc from main()
     * \param argv from main()
     * \param applicationName
     * \param organizationName
     */
    BaseApplication( int& argc,
                     char** argv,
                     QString applicationName  = "RadiumEngine",
                     QString organizationName = "STORM-IRIT" );

    ~BaseApplication() override;

    /** Initialize the application, create the Gui and OpenGL environment.
     * The initialization of an application is made in several steps
     *   1. Create and initialize the engine and its non-OpenGL services
     *     - Once the engine singleton is instanciated, call the BaseApplication's virtual method
     *     engineBaseInitialization to populate the engine with the base systems
     *     (eg. GeometrySystem) and configure the non-openGL engine services.
     *   2. Create and initialize the openGL environment
     *     - When the openGL context is ready and bound, call the BaseApplication's virtual method
     *     engineOpenGLInitialize (this method is a virtual slot connected to the event
     *     Gui::Viewer::requestEngineOpenGLInitialization).
     *     - When the application and its opengl window are ready, call the BaseApplication's
     *     virtual method  initializeGl (this method is a virtual slot connected to the event
     *     Gui::Viewer::glInitialized).
     *   3. Create Plugin context, load plugins, configure base Radium::IO services
     *     - The plugins located int the registered plugin paths (into the Radium Engine or
     *     bundled with the installed application, into paths found in the app configuration file)
     *     are loaded automatically.
     *     - The Radium::IO services compiled into the Radium bundle are configured (file loaders)
     *     - After plugins and default services are configured, call the BaseApplication's virtual
     *     method addApplicationExtension.
     *   4. Manage scene related command-line argument
     *     - loads the given scene, ...
     *
     * \param factory : a functor that instanciate the mainWindow
     * @note The initialize method call virtual methods on the object being initialized to
     * configure the engine and application services.
     * When redefining those methods, it is recommended to call the inherited one to have
     * consistent initialization wrt the BaseApplication ancestor.
     * The initialize method could be also overloaded by derived classes. In this case, it is
     * recommended that the overload explicetely call the ancestor method..
     * \todo allow the user to ask for some "standard" systems to be added to the initialized
     * Engine.
     */
    void initialize( const WindowFactory& factory );

    /**
     * This method configure the base, non opengl dependant, scene services.
     *
     * It is expected that this method add the Engine's systems required by the application.
     * and configure the engine time management properties.
     * The default implementation add the following systems :
     *   - Ra::Engine::GeometrySystem
     *
     * The default implementation also configure the time system according to the user requested
     * frame rate (app command line argument -framerate) or to the default 60fps.
     */
    virtual void engineBaseInitialization();

    /**
     * Allow derived applications to add specific extensions to the engine and the base application.
     * Such extensions are expected to be :
     *  - specific file loaders
     *  - specific plugins
     *  - specific global resources needed by the application (Textures, shaders, ...)
     *  This method could also be used to populate the engine with systems or with OpenGL related
     *  services if this was not already done in the appropriate methods (engineBaseInitialization,
     *  engineOpenGLInitialize, initializeGl)
     *  - specific renderers (could be added instead when configuring OpenGL)
     *  -
     */
    virtual void addApplicationExtension() {};

    /// Advance the engine for one frame.
    void radiumFrame();

    bool isRunning() const { return !m_isAboutToQuit; }

    const Engine::RadiumEngine* getEngine() const { return m_engine.get(); }

    uint getFrameCount() const { return m_frameCounter; }

    const std::string& getExportFolderName() const { return m_exportFoldername; }

    void addPluginDirectory( const std::string& pluginDir );
    void clearPluginDirectories();

  signals:
    /// Fired when the engine has just started, before the frame timer is set.
    void starting();

    /// Fired when the engine is about to stop.
    void stopping();

    /// Fired when the scene has changed.
    void sceneChanged( const Core::Aabb& );

    void updateFrameStats( const std::vector<FrameTimerData>& );

    void loadComplete();

    void selectedItem( const Ra::Engine::Scene::ItemEntry& entry );

  public slots:

    /// slot called when the OpenGL need to be initialized
    virtual void engineOpenGLInitialize();

    /// slot called once the application window and its OpenGL services are ready.
    /// TODO : rename this to be more representative of post opengl & gui operations
    virtual void initializeGl();

    void updateRadiumFrameIfNeeded() {
        // Main loop
        if ( m_isUpdateNeeded.load() ) radiumFrame();
        if ( m_continuousUpdateRequest <= 0 )
        {
            m_continuousUpdateRequest.store( 0 );
            m_isUpdateNeeded.store( false );
        }
        if ( m_isAboutToQuit ) { this->exit(); }
    }

    bool loadFile( QString path );
    void framesCountForStatsChanged( uint count );
    void appNeedsToQuit();

    void setRealFrameRate( bool on );
    void setRecordFrames( bool on );
    void setRecordTimings( bool on );
    void setRecordGraph( bool on );

    void recordFrame();

    void onSelectedItem( const Ra::Engine::Scene::ItemEntry& entry ) { emit selectedItem( entry ); }

    void setContinuousUpdate( bool b ) {
        b ? m_continuousUpdateRequest++ : m_continuousUpdateRequest--;
        // if continuous update is requested, then we need an update, if not,
        // let update needed stay in the same state.
        if ( m_continuousUpdateRequest > 0 ) m_isUpdateNeeded.store( true );
    }
    void askForUpdate() { m_isUpdateNeeded.store( true ); }

  protected:
    /// Create signal / slots connections
    void createConnections();

    /// Load plugins from the specified folder.
    /// If loadList is empty, attempts to load all DLLs in the folder (except those on the ignore
    /// list) If loadList contains names it will only look for DLLs in that folder with the given
    /// name.
    bool loadPlugins( const std::string& pluginsPath,
                      const QStringList& loadList,
                      const QStringList& ignoreList );

    void setupScene();

    /// check wheter someone ask for update
    bool isUpdateNeeded() { return m_isUpdateNeeded.load(); }

    /// if b is true, then update anyway. If b is false, update on request only
    void setIsUpdateNeeded( bool b ) { m_isUpdateNeeded.store( b ); }

    // Public variables, accessible through the mainApp singleton.
  public:
    /// Application main window and GUI root class.
    std::unique_ptr<Gui::MainWindowInterface> m_mainWindow;

    /// Instance of the radium engine.
    std::unique_ptr<Engine::RadiumEngine> m_engine;

    /// Task queue for processing tasks.
    std::unique_ptr<Core::TaskQueue> m_taskQueue;

    /// Number of frames per second to generate.
    uint m_targetFPS;

  protected:
    /// Plugins that need to be initialized once OpenGL is ready
    std::vector<Ra::Plugins::RadiumPluginInterface*> m_openGLPlugins;

    /// Pointer to OpenGL Viewer for render call (belongs to MainWindow).
    Gui::Viewer* m_viewer;

    /// Timer to wake us up at every frame start.
    QTimer* m_frameTimer;

    /// Time since the last frame start.
    Core::Utils::TimePoint m_lastFrameStart;

    uint m_frameCounter;
    uint m_frameCountBeforeUpdate;
    uint m_numFrames;
    uint m_maxThreads;
    std::vector<FrameTimerData> m_timerData;
    std::string m_pluginPath;

    /// If true, use the wall clock to advance the engine. If false, use a fixed time step.
    bool m_realFrameRate;

    // Options to control monitoring and outputs
    /// Name of the folder where exported data goes
    std::string m_exportFoldername;

    /// If true, dump each frame to a PNG file.
    bool m_recordFrames;
    /// If true, print the detailed timings of each frame
    bool m_recordTimings;
    /// If true, print the task graph;
    bool m_recordGraph;

    /// True if the applicatioon is about to quit. prevent to use resources that are being released.
    bool m_isAboutToQuit;

    /// If true update the viewer frame next time
    std::atomic_bool m_isUpdateNeeded {true};

    /// If counter is >= 0, continuously update viewer frame
    std::atomic<int> m_continuousUpdateRequest {1};

    Plugins::Context m_pluginContext;
    QCommandLineParser* m_parser;
};
} // namespace Gui
} // namespace Ra
