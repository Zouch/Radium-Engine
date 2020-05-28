#include <GuiBase/BaseApplication.hpp>
#include <GuiBase/MainWindowInterface.hpp>
#include <GuiBase/Viewer/Viewer.hpp>

#include <Core/CoreMacros.hpp>
#include <Core/Resources/Resources.hpp>
#include <Core/Tasks/Task.hpp>
#include <Core/Tasks/TaskQueue.hpp>
#include <Core/Types.hpp>
#include <Core/Utils/Color.hpp>
#include <Core/Utils/Log.hpp>
#include <Core/Utils/StringUtils.hpp>
#include <Core/Utils/Version.hpp>

#include <Engine/Managers/EntityManager/EntityManager.hpp>
#include <Engine/Managers/SystemDisplay/SystemDisplay.hpp>
#include <Engine/RadiumEngine.hpp>
#include <Engine/Renderer/Camera/Camera.hpp>
#include <Engine/Renderer/RenderObject/RenderObject.hpp>
#include <Engine/Renderer/RenderTechnique/ShaderConfigFactory.hpp>
#include <Engine/System/GeometrySystem.hpp>

#include <PluginBase/RadiumPluginInterface.hpp>

#include <IO/CameraLoader/CameraLoader.hpp>
#ifdef IO_USE_TINYPLY
#    include <IO/TinyPlyLoader/TinyPlyFileLoader.hpp>
#endif
#ifdef IO_USE_ASSIMP
#    include <IO/AssimpLoader/AssimpFileLoader.hpp>
#endif

#include <QCommandLineParser>
#include <QDir>
#include <QMessageBox>
#include <QOpenGLContext>
#include <QPluginLoader>
#include <QTimer>

// Const parameters : TODO : make config / command line options

namespace Ra {
namespace GuiBase {

using namespace Core::Utils; // log
using namespace Core::Asset;

#ifdef GUIBASE_IS_COMPILED_WITH_DEBUG_INFO
static const bool expectPluginsDebug = true;
#else
static const bool expectPluginsDebug = false;
#endif

QCommandLineOption
    fpsOpt( QStringList{"r", "framerate", "fps"},
            "Control the application framerate, 0 to disable it (and run as fast as possible).",
            "number",
            "60" );
QCommandLineOption maxThreadsOpt(
    QStringList{"m", "maxthreads", "max-threads"},
    "Control the maximum number of threads. 0 will set to the number of cores available",
    "number",
    "0" );
QCommandLineOption numFramesOpt( QStringList{"n", "numframes"},
                                 "Run for a fixed number of frames.",
                                 "number",
                                 "0" );
QCommandLineOption pluginOpt( QStringList{"p", "plugins", "pluginsPath"},
                              "Set the path to the plugin dlls.",
                              "folder",
                              "Plugins" );

QCommandLineOption pluginLoadOpt(
    QStringList{"l", "load", "loadPlugin"},
    "Only load plugin with the given name (filename without the extension). If this option is "
    "not used, all plugins in the plugins folder will be loaded. ",
    "name" );

QCommandLineOption pluginIgnoreOpt( QStringList{"K", "ignore", "ignorePlugin"},
                                    "Ignore plugins with the given name. If the name appears "
                                    "within both load and ignore options, it will be ignored.",
                                    "name" );

QCommandLineOption fileOpt( QStringList{"f", "file", "scene"},
                            "Open a scene file at startup.",
                            "file name",
                            "foo.bar" );

QCommandLineOption camOpt( QStringList{"c", "camera", "cam"},
                           "Open a camera file at startup",
                           "file name",
                           "foo.bar" );
QCommandLineOption recordOpt( QStringList{"s", "recordFrames"}, "Enable snapshot recording." );

BaseApplication::BaseApplication( int& argc,
                                  char** argv,
                                  const WindowFactory& factory,
                                  QString applicationName,
                                  QString organizationName ) :
    QApplication( argc, argv ),
    m_mainWindow( nullptr ),
    m_engine( nullptr ),
    m_taskQueue( nullptr ),
    m_viewer( nullptr ),
    m_frameTimer( new QTimer( this ) ),
    m_frameCounter( 0 ),
    m_frameCountBeforeUpdate( 60 ),
    m_numFrames( 0 ),
    m_maxThreads( RA_MAX_THREAD ),
    m_realFrameRate( false ),
    m_recordFrames( false ),
    m_recordTimings( false ),
    m_recordGraph( false ),
    m_isAboutToQuit( false ) {
    // Set application and organization names in order to ensure uniform
    // QSettings configurations.
    // \see http://doc.qt.io/qt-5/qsettings.html#QSettings-4
    QCoreApplication::setOrganizationName( organizationName );
    QCoreApplication::setApplicationName( applicationName );

    m_targetFPS = 60; // Default
    // TODO at startup, only load "standard plugins". This must be extended.
    pluginsPath = std::string{Core::Resources::getRadiumPluginsDir()};


    parser.setApplicationDescription( "Radium Engine RPZ, TMTC" );
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOptions( {fpsOpt,
                        pluginOpt,
                        pluginLoadOpt,
                        pluginIgnoreOpt,
                        fileOpt,
                        camOpt,
                        maxThreadsOpt,
                        numFramesOpt,
                        recordOpt} );

    parser.process( *this );




    parser.values( pluginIgnoreOpt );


    if ( parser.isSet( fpsOpt ) ) m_targetFPS = parser.value( fpsOpt ).toUInt();
    if ( parser.isSet( pluginOpt ) ) pluginsPath = parser.value( pluginOpt ).toStdString();
    if ( parser.isSet( numFramesOpt ) ) m_numFrames = parser.value( numFramesOpt ).toUInt();
    if ( parser.isSet( maxThreadsOpt ) ) m_maxThreads = parser.value( maxThreadsOpt ).toUInt();
    if ( parser.isSet( recordOpt ) )
    {
        m_recordFrames = true;
        setContinuousUpdate( true );
    }

    {
        std::time_t startTime = std::time( nullptr );
        std::tm* startTm      = std::localtime( &startTime );
        std::stringstream ssTp;
        ssTp << std::put_time( startTm, "%Y%m%d-%H%M%S" );
        m_exportFoldername = ssTp.str();
    }

    QDir().mkdir( m_exportFoldername.c_str() );

    // Boilerplate print.
    LOG( logINFO ) << "*** Radium Engine Main App  ***";
    std::stringstream config;
#if defined( CORE_DEBUG )
    config << "Debug Build ";
#else
    config << "Release Build ";
#endif
#if defined( CORE_ENABLE_ASSERT )
    config << "(with asserts) --";
#else
    config << " --";
#endif

#if defined( ARCH_X86 )
    config << " 32 bits x86";
#elif defined( ARCH_X64 )
    config << " 64 bits x64";
#endif
    LOG( logINFO ) << config.str();

    config.str( std::string() );
    config << "Floating point format : ";
#if defined( CORE_USE_DOUBLE )
    config << "double precision";
#else
    config << "single precision";
#endif

    LOG( logINFO ) << config.str();

    config.str( std::string() );
    config << "core build: " << Version::compiler << " - " << Version::compileDate << " "
           << Version::compileTime;
    LOG( logINFO ) << config.str();

    LOG( logINFO ) << "Git changeset: " << Version::gitChangeSet;

    LOG( logINFO ) << "Qt Version: " << qVersion();

    LOG( logINFO ) << "Max Thread: " << m_maxThreads;

    // Create default format for Qt.
    QSurfaceFormat format;
    format.setVersion( 4, 4 );
    format.setProfile( QSurfaceFormat::CoreProfile );
    format.setDepthBufferSize( 24 );
    format.setStencilBufferSize( 8 );
    format.setSamples( 16 );
    format.setSwapBehavior( QSurfaceFormat::DoubleBuffer );
    format.setSwapInterval( 0 );
    QSurfaceFormat::setDefaultFormat( format );

    // Create the instance of the keymapping manager, before creating
    // Qt main windows, which may throw events on Microsoft Windows
    Gui::KeyMappingManager::createInstance();

    // Create engine
    m_engine.reset( Engine::RadiumEngine::createInstance() );
    m_engine->initialize();
    addBasicShaders();

    // Create main window.
    m_mainWindow.reset( factory.createMainWindow() );
    m_mainWindow->show();

    m_viewer = m_mainWindow->getViewer();
    m_viewer->setupKeyMappingCallbacks();

    CORE_ASSERT( m_viewer != nullptr, "GUI was not initialized" );

    // Connect the signals and allow all pending events to be processed
    // (thus the viewer should have initialized the OpenGL context..)
    createConnections();

    // the rest of the initialization has to be done with valid opengl context,
    // so defer after glInitialized.
    QMetaObject::Connection* const connection = new QMetaObject::Connection;

    *connection =
        QObject::connect( m_viewer, &Ra::Gui::Viewer::glInitialized, [this, connection]() {
            deferredInitialization();
            QObject::disconnect( *connection );
            delete connection;
        } );
}

void BaseApplication::deferredInitialization() {

    // Register the GeometrySystem converting loaded assets to meshes
    m_engine->registerSystem( "GeometrySystem", new Ra::Engine::GeometrySystem, 1000 );

    // Initialize plugin context
    m_pluginContext.m_engine           = m_engine.get();
    m_pluginContext.m_selectionManager = m_mainWindow->getSelectionManager();
    m_pluginContext.m_pickingManager   = m_viewer->getPickingManager();
    m_pluginContext.m_viewer           = m_viewer;
    m_pluginContext.m_exportDir        = m_exportFoldername;

    connect( &m_pluginContext,
             &Plugins::Context::setContinuousUpdate,
             this,
             &BaseApplication::setContinuousUpdate );
    connect(
        &m_pluginContext, &Plugins::Context::askForUpdate, this, &BaseApplication::askForUpdate );

    // Load installed plugins plugins
    if ( !loadPlugins(
             pluginsPath, parser.values( pluginLoadOpt ), parser.values( pluginIgnoreOpt ) ) )
    { LOG( logERROR ) << "An error occurred while trying to load plugins."; }
    // load supplemental plugins
    {
        QSettings settings;
        QStringList plunginPaths = settings.value( "plugins/paths" ).value<QStringList>();
        for ( const auto s : plunginPaths )
        {
            loadPlugins(
                s.toStdString(), parser.values( pluginLoadOpt ), parser.values( pluginIgnoreOpt ) );
        }
    }

    // Make builtin loaders the fallback if no plugins can load some file format
#ifdef IO_USE_TINYPLY
    // Register before AssimpFileLoader, in order to ease override of such
    // custom loader (first loader able to load is taking the file)
    m_engine->registerFileLoader(
        std::shared_ptr<FileLoaderInterface>( new IO::TinyPlyFileLoader() ) );
#endif
    m_engine->registerFileLoader(
        std::shared_ptr<FileLoaderInterface>( new IO::CameraFileLoader() ) );
#ifdef IO_USE_ASSIMP
    m_engine->registerFileLoader(
        std::shared_ptr<FileLoaderInterface>( new IO::AssimpFileLoader() ) );
#endif

    // Create task queue with N-1 threads (we keep one for rendering),
    // unless monothread CPU
    uint numThreads =
        std::max( m_maxThreads == 0 ? RA_MAX_THREAD : std::min( m_maxThreads, RA_MAX_THREAD ), 1u );
    m_taskQueue = std::make_unique<Core::TaskQueue>( numThreads );

    setupScene();
    emit starting();

    // Files have been required, load them.
    if ( parser.isSet( fileOpt ) )
    {
        for ( const auto& filename : parser.values( fileOpt ) )
        {
            loadFile( filename );
        }
    }
    // A camera has been required, load it.
    if ( parser.isSet( camOpt ) )
    {
        if ( loadFile( parser.value( camOpt ) ) )
        {
            auto entity = *( m_engine->getEntityManager()->getEntities().rbegin() );
            auto camera = static_cast<Engine::Camera*>( entity->getComponents()[0].get() );
            m_viewer->setCamera( camera );
        }
    }

    m_lastFrameStart = Core::Utils::Clock::now();

    connect( m_frameTimer, &QTimer::timeout, this, &BaseApplication::updateRadiumFrameIfNeeded );
    const int deltaTime( m_targetFPS == 0 ? 1 : 1000 / m_targetFPS );
    m_frameTimer->start( deltaTime );
}

void BaseApplication::createConnections() {
    connect(
        m_mainWindow.get(), &MainWindowInterface::closed, this, &BaseApplication::appNeedsToQuit );
    connect(
        m_viewer, &Gui::Viewer::glInitialized, this, &BaseApplication::initializeOpenGlPlugins );
    connect( this, &QGuiApplication::lastWindowClosed, m_viewer, &Gui::WindowQt::cleanupGL );

    connect( m_viewer, &Gui::Viewer::needUpdate, this, &BaseApplication::askForUpdate );
}

void BaseApplication::setupScene() {

    using namespace Engine::DrawPrimitives;

    auto grid = Primitive( Engine::SystemEntity::uiCmp(),
                           Grid( Core::Vector3::Zero(),
                                 Core::Vector3::UnitX(),
                                 Core::Vector3::UnitZ(),
                                 Core::Utils::Color::Grey( 0.6f ) ) );
    grid->setPickable( false );
    Engine::SystemEntity::uiCmp()->addRenderObject( grid );

    auto frame =
        Primitive( Engine::SystemEntity::uiCmp(), Frame( Ra::Core::Transform::Identity(), 0.05f ) );
    frame->setPickable( false );
    Engine::SystemEntity::uiCmp()->addRenderObject( frame );
}

bool BaseApplication::loadFile( QString path ) {
    std::string filename = path.toLocal8Bit().data();
    LOG( logINFO ) << "Loading file " << filename << "...";
    bool res = m_engine->loadFile( filename );

    if ( !res )
    {
        LOG( logERROR ) << "Aborting file loading !";

        return false;
    }

    m_engine->releaseFile();

    m_mainWindow->postLoadFile( filename );

    emit loadComplete();
    return true;
}

void BaseApplication::framesCountForStatsChanged( uint count ) {
    m_frameCountBeforeUpdate = count;
}

void BaseApplication::addBasicShaders() {
    using namespace Ra::Engine;
    /// For internal resources management in a filesystem
    std::string resourcesRootDir = {Core::Resources::getRadiumResourcesDir()};

    /// @todo Are these shaders used somewhere ?
    /// Why loading them in the applicaiton and not in the engine or in the Plugin ?
    ShaderConfiguration lgConfig( "LinesGeom" );
    lgConfig.addShader( ShaderType_VERTEX, resourcesRootDir + "Shaders/Lines/Lines.vert.glsl" );
    lgConfig.addShader( ShaderType_FRAGMENT, resourcesRootDir + "Shaders/Lines/Lines.frag.glsl" );
    lgConfig.addShader( ShaderType_GEOMETRY, resourcesRootDir + "Shaders/Lines/Lines.geom.glsl" );
    ShaderConfigurationFactory::addConfiguration( lgConfig );

    ShaderConfiguration lagConfig( "LinesAdjacencyGeom" );
    lagConfig.addShader( ShaderType_VERTEX, resourcesRootDir + "Shaders/Lines/Lines.vert.glsl" );
    lagConfig.addShader( ShaderType_FRAGMENT,
                         resourcesRootDir + "Shaders/Lines/LinesAdjacency.frag.glsl" );
    lagConfig.addShader( ShaderType_GEOMETRY, resourcesRootDir + "Shaders/Lines/Lines.geom.glsl" );
    ShaderConfigurationFactory::addConfiguration( lagConfig );
}

void BaseApplication::radiumFrame() {
    if ( !m_viewer->isOpenGLInitialized() ) return;
    FrameTimerData timerData;
    timerData.frameStart = Core::Utils::Clock::now();

    // ----------
    // 0. Compute time since last frame.
    const Scalar dt =
        m_realFrameRate ? Core::Utils::getIntervalSeconds( m_lastFrameStart, timerData.frameStart )
                        : 1.f / Scalar( m_targetFPS );
    m_lastFrameStart = timerData.frameStart;

    // ----------
    // 1. Gather user input and dispatch it.
    // Get picking results from last frame and forward it to the selection.
    m_viewer->processPicking();

    timerData.tasksStart = Core::Utils::Clock::now();

    // ----------
    // 2. Run the engine task queue.
    m_engine->getTasks( m_taskQueue.get(), dt );

    if ( m_recordGraph ) { m_taskQueue->printTaskGraph( std::cout ); }

    // Run one frame of tasks
    m_taskQueue->startTasks();
    m_taskQueue->waitForTasks();
    timerData.taskData = m_taskQueue->getTimerData();
    m_taskQueue->flushTaskQueue();

    timerData.tasksEnd = Core::Utils::Clock::now();

    // also update gizmo manager to deal with annimation playing / reset
    // m_viewer->getGizmoManager()->updateValues();

    // update viewer internal time-dependant state
    m_viewer->update( dt );

    // ----------
    // 3. Kickoff rendering
    m_viewer->startRendering( dt );
    m_viewer->swapBuffers();

    timerData.renderData = m_viewer->getRenderer()->getTimerData();

    // ----------
    // 4. Synchronize whatever needs synchronisation
    m_engine->endFrameSync();

    // ----------
    // 5. Frame end.
    timerData.frameEnd = Core::Utils::Clock::now();
    timerData.numFrame = m_frameCounter;

    if ( m_recordTimings ) { timerData.print( std::cout ); }

    m_timerData.push_back( timerData );

    if ( m_recordFrames ) { recordFrame(); }

    ++m_frameCounter;

    if ( m_numFrames > 0 && m_frameCounter >= m_numFrames ) { appNeedsToQuit(); }

    if ( m_frameCounter % m_frameCountBeforeUpdate == 0 )
    {
        emit( updateFrameStats( m_timerData ) );
        m_timerData.clear();
    }
    m_mainWindow->onFrameComplete();
}

void BaseApplication::appNeedsToQuit() {
    LOG( logDEBUG ) << "About to quit.";
    m_isAboutToQuit = true;
}

void BaseApplication::initializeOpenGlPlugins() {
    // Initialize plugins that depends on Initialized OpenGL (if any)
    if ( !m_openGLPlugins.empty() )
    {
        for ( auto plugin : m_openGLPlugins )
        {
            m_viewer->makeCurrent();
            plugin->openGlInitialize( m_pluginContext );
            m_viewer->doneCurrent();
        }
        m_openGLPlugins.clear();
    }
}

void BaseApplication::setRealFrameRate( bool on ) {
    m_realFrameRate = on;
}

void BaseApplication::setRecordFrames( bool on ) {
    setContinuousUpdate( on );
    if ( on ) askForUpdate();
    m_recordFrames = on;
}

void BaseApplication::recordFrame() {
    std::stringstream filename;
    filename << m_exportFoldername << "/radiumframe_" << std::setw( 6 ) << std::setfill( '0' )
             << m_frameCounter << ".png";
    m_viewer->grabFrame( filename.str() );
}

BaseApplication::~BaseApplication() {
    emit stopping();
    m_mainWindow->cleanup();
    m_engine->cleanup();

    // This will remove the directory if empty.
    QDir().rmdir( m_exportFoldername.c_str() );
}

bool BaseApplication::loadPlugins( const std::string& pluginsPath2,
                                   const QStringList& loadList,
                                   const QStringList& ignoreList ) {
    QDir pluginsDir( qApp->applicationDirPath() );
    LOG( logINFO ) << " *** Loading Plugins ***";
    bool result = pluginsDir.cd( pluginsPath2.c_str() );

    if ( !result )
    {
        LOG( logERROR ) << "Cannot open specified plugins directory " << pluginsPath2;
        return false;
    }

    LOG( logDEBUG ) << "Plugin directory :" << pluginsDir.absolutePath().toStdString();
    bool res       = true;
    uint pluginCpt = 0;

    for ( const auto& filename : pluginsDir.entryList( QDir::Files ) )
    {

        std::string ext = Core::Utils::getFileExt( filename.toStdString() );
#if defined( OS_WINDOWS )
        std::string sysDllExt = "dll";
#elif defined( OS_LINUX )
        std::string sysDllExt = "so";
#elif defined( OS_MACOS )
        std::string sysDllExt = "dylib";
#else
        static_assert( false, "System configuration not handled" );
#endif
        if ( ext == sysDllExt )
        {
            std::string basename = Core::Utils::getBaseName( filename.toStdString(), false );

            auto stringCmp = [basename]( const QString& str ) {
                return str.toStdString() == basename;
            };

            if ( !loadList.empty() &&
                 std::find_if( loadList.begin(), loadList.end(), stringCmp ) == loadList.end() )
            {
                LOG( logDEBUG ) << "Ignoring " << filename.toStdString() << " (not on load list)";
                continue;
            }
            if ( std::find_if( ignoreList.begin(), ignoreList.end(), stringCmp ) !=
                 ignoreList.end() )
            {
                LOG( logDEBUG ) << "Ignoring " << filename.toStdString() << " (on ignore list)";
                continue;
            }

            QPluginLoader pluginLoader( pluginsDir.absoluteFilePath( filename ) );
            // Force symbol resolution at load time.
            pluginLoader.setLoadHints( QLibrary::ResolveAllSymbolsHint );

            // Looking for Radium plugin signature on metadata
            auto metadata = pluginLoader.metaData()["MetaData"].toObject();
            if ( metadata.isEmpty() || !metadata.contains( "com.storm-irit.RadiumEngine" ) ||
                 metadata["com.storm-irit.RadiumEngine"].toString().compare( "plugin" ) != 0 )
            {
                LOG( logDEBUG ) << "File " << filename.toStdString() << " is not a Radium plugin.";
                continue;
            }

            // detect if the plugin meets the minimal requirements
            // if not, triggers a QDialog explaining the error, and abort the application
            // We choose to stop the application to force all the plugins to be updated
            if ( !metadata.contains( "isDebug" ) )
            {
                QMessageBox::critical( m_mainWindow.get(),
                                       "Invalid plugin loaded (see Q_RADIUM_PLUGIN_METADATA)",
                                       QString( "The application tried to load an unsupported "
                                                "plugin. The application will stop.\n" ) +
                                           QString( "Plugin path: " ) +
                                           pluginsDir.absoluteFilePath( filename ) );
                appNeedsToQuit();
                return false;
            }
            bool isPluginDebug = metadata["isDebug"].toString().compare( "true" ) == 0;
            if ( expectPluginsDebug == isPluginDebug )
            {
                LOG( logINFO ) << "Found plugin " << filename.toStdString();
                // load the plugin
                QObject* plugin = pluginLoader.instance();
                if ( plugin )
                {
                    auto loadedPlugin = qobject_cast<Plugins::RadiumPluginInterface*>( plugin );
                    if ( loadedPlugin )
                    {
                        ++pluginCpt;
                        loadedPlugin->registerPlugin( m_pluginContext );
                        m_mainWindow->updateUi( loadedPlugin );

                        if ( loadedPlugin->doAddRenderer() )
                        {
                            std::vector<std::shared_ptr<Engine::Renderer>> tmpR;
                            loadedPlugin->addRenderers( &tmpR );
                            CORE_ASSERT( !tmpR.empty(),
                                         "This plugin is expected to add a renderer" );
                            for ( const auto& ptr : tmpR )
                            {
                                std::string name =
                                    ptr->getRendererName() + "(" + filename.toStdString() + ")";
                                m_mainWindow->addRenderer( name, ptr );
                            }
                        }

                        if ( loadedPlugin->doAddFileLoader() )
                        {
                            std::vector<std::shared_ptr<FileLoaderInterface>> tmpL;
                            loadedPlugin->addFileLoaders( &tmpL );
                            CORE_ASSERT( !tmpL.empty(),
                                         "This plugin is expected to add file loaders" );
                            for ( auto& ptr : tmpL )
                            {
                                m_engine->registerFileLoader( ptr );
                            }
                        }

                        if ( loadedPlugin->doAddROpenGLInitializer() )
                        {
                            if ( m_viewer->isOpenGLInitialized() )
                            {
                                LOG( logDEBUG ) << "Direct OpenGL initialization for plugin "
                                                << filename.toStdString();
                                // OpenGL is ready, initialize openGL part of the plugin
                                m_viewer->makeCurrent();
                                loadedPlugin->openGlInitialize( m_pluginContext );
                                m_viewer->doneCurrent();
                            }
                            else
                            {
                                // Defer OpenGL initialisation
                                LOG( logDEBUG ) << "Defered OpenGL initialization for plugin "
                                                << filename.toStdString();
                                m_openGLPlugins.push_back( loadedPlugin );
                            }
                        }
                    }
                    else
                    {
                        LOG( logERROR ) << "Something went wrong while trying to cast plugin "
                                        << filename.toStdString();
                        res = false;
                    }
                }
                else
                {
                    LOG( logERROR ) << "Something went wrong while trying to load plugin "
                                    << filename.toStdString() << " : "
                                    << pluginLoader.errorString().toStdString();
                    res = false;
                }
            }
            else
            {
                LOG( logERROR ) << "Skipped plugin " << filename.toStdString()
                                << " : invalid build mode. Full path: "
                                << pluginsDir.absoluteFilePath( filename ).toStdString();
                res = false;
            }
        }
    }

    if ( pluginCpt == 0 ) { LOG( logINFO ) << "No plugin found or loaded."; }
    else
    { LOG( logINFO ) << "Loaded " << pluginCpt << " plugins."; }

    return res;
}

void BaseApplication::setRecordTimings( bool on ) {
    m_recordTimings = on;
}

void BaseApplication::setRecordGraph( bool on ) {
    m_recordGraph = on;
}

void BaseApplication::addPluginDirectory( const std::string& pluginDir ) {
    QSettings settings;
    QStringList plunginPaths = settings.value( "plugins/paths" ).value<QStringList>();
    LOG( logINFO ) << "Registered plugin paths are : ";
    for ( const auto s : plunginPaths )
    {
        LOG( logINFO ) << s.toStdString();
    }
    plunginPaths.append( pluginDir.c_str() );
    settings.setValue( "plugins/paths", plunginPaths );
    loadPlugins( pluginDir, QStringList(), QStringList() );
}

void BaseApplication::clearPluginDirectories() {
    QSettings settings;
    settings.setValue( "plugins/paths", QStringList() );
}

} // namespace GuiBase
} // namespace Ra
