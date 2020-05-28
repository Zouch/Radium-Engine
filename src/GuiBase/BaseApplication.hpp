#ifndef RADIUMENGINE_BASEAPPLICATION_HPP_
#define RADIUMENGINE_BASEAPPLICATION_HPP_

#include <GuiBase/RaGuiBase.hpp>

#include <Core/Utils/Timer.hpp>
#include <GuiBase/TimerData/FrameTimerData.hpp>
#include <PluginBase/RadiumPluginInterface.hpp>

#include <QApplication>
#include <QCommandLineParser>

#include <atomic>
#include <chrono>
#include <memory>
#include <vector>

class QTimer;

namespace Ra {
namespace Engine {
class RadiumEngine;
class GeometrySystem;
struct ItemEntry;
} // namespace Engine
} // namespace Ra

namespace Ra {
namespace Gui {
class Viewer;
}

namespace GuiBase {
class MainWindowInterface;
}
} // namespace Ra

namespace Ra {
namespace GuiBase {

/// virtual class to handle main window creation
class WindowFactory
{
  public:
    WindowFactory()                                                    = default;
    virtual ~WindowFactory()                                           = default;
    virtual Ra::GuiBase::MainWindowInterface* createMainWindow() const = 0;
};

/// This class contains the main application logic. It owns the engine and the GUI.
class RA_GUIBASE_API BaseApplication : public QApplication
{
    Q_OBJECT

  public:
    /** Setup the application, create main window and main connections.
     * \param argc from main()
     * \param argv from main()
     * \param factory : a functor that instanciate the mainWindow
     * \param applicationName
     * \param organizationName
     */
    BaseApplication( int& argc,
                     char** argv,
                     const WindowFactory& factory,
                     QString applicationName  = "RadiumEngine",
                     QString organizationName = "STORM-IRIT" );
    ~BaseApplication();

    /// This function is attached to viewer glInitialized signals so that
    /// initialization that need opengl context is done correctly.
    void deferredInitialization();

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

    void selectedItem( const Ra::Engine::ItemEntry& entry );

  public slots:

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
    void initializeOpenGlPlugins();

    void setRealFrameRate( bool on );
    void setRecordFrames( bool on );
    void setRecordTimings( bool on );
    void setRecordGraph( bool on );

    void recordFrame();

    void onSelectedItem( const Ra::Engine::ItemEntry& entry ) { emit selectedItem( entry ); }

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
    void addBasicShaders();

    /// check wheter someone ask for update
    bool isUpdateNeeded() { return m_isUpdateNeeded.load(); }

    /// if b is true, then update anyway. If b is false, update on request only
    void setIsUpdateNeeded( bool b ) { m_isUpdateNeeded.store( b ); }

    // Public variables, accessible through the mainApp singleton.
  public:
    /// Application main window and GUI root class.
    std::unique_ptr<GuiBase::MainWindowInterface> m_mainWindow;

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
    std::atomic_bool m_isUpdateNeeded{true};

    /// If counter is >= 0, continuously update viewer frame
    std::atomic<int> m_continuousUpdateRequest{1};

    Plugins::Context m_pluginContext;

    QCommandLineParser parser;
    std::string pluginsPath;
};

} // namespace GuiBase
} // namespace Ra
#endif // RADIUMENGINE_BASEAPPLICATION_HPP_
