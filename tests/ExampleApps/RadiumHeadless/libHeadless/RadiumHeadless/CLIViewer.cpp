#include <RadiumHeadless/CLIViewer.hpp>

#include <Core/Asset/Camera.hpp>
#include <Core/Asset/FileLoaderInterface.hpp>
#include <Core/Tasks/TaskQueue.hpp>
#include <Core/Utils/Log.hpp>

#include <Engine/Data/ViewingParameters.hpp>
#include <Engine/RadiumEngine.hpp>
#include <Engine/Rendering/Renderer.hpp>
#include <Engine/Scene/DefaultCameraManager.hpp>
#include <Engine/Scene/DirLight.hpp>
#include <Engine/Scene/GeometrySystem.hpp>
#include <Engine/Scene/SkeletonBasedAnimationSystem.hpp>
#include <Engine/Scene/SystemDisplay.hpp>

using namespace Ra::Core::Utils;

constexpr int defaultSystemPriority = 1000;

CLIViewer::CLIViewer() : CLIBaseApplication(), m_glContext {} {
    cmdline_parser.add_option( "-s,--size", m_parameters.m_size, "Size of the computed image." )
        ->delimiter( 'x' );
    cmdline_parser.add_flag(
        "-a,--animation", m_parameters.m_animationEnable, "Enable Radium Animation system." );
}

CLIViewer::~CLIViewer() {
    if ( m_engineInitialized )
    {
        m_glContext.makeCurrent();
        m_renderer.reset();
        m_engine->cleanup();
        m_glContext.doneCurrent();
    }
}

const CLIViewer::ViewerParameters& CLIViewer::getViewerParameters() const {
    return m_parameters;
}

int CLIViewer::init( int argc, const char* argv[] ) {
    try
    { cmdline_parser.parse( argc, argv ); }
    catch ( const CLI::ParseError& e )
    { return cmdline_parser.exit( e ) + 1; }
    // Do the init
    if ( !m_glContext.isValid() )
    {
        LOG( logERROR ) << "Invalid openglContext, the application can't run";
        return 1;
    }

    m_glContext.resize( m_parameters.m_size );
    LOG( logINFO ) << "CLIViewer :\n" << m_glContext.getInfo();

    // Initialize the Radium engine environment

    // Create engine
    m_engine.reset( Ra::Engine::RadiumEngine::createInstance() );
    m_engine->initialize();
    m_engineInitialized = true;

    // Register the GeometrySystem converting loaded assets to meshes
    m_engine->registerSystem(
        "GeometrySystem", new Ra::Engine::Scene::GeometrySystem, defaultSystemPriority );

    if ( m_parameters.m_animationEnable )
    {
        // Register the SkeletonBasedAnimationSystem converting loaded assets to
        // skeletons and skinning data
        m_engine->registerSystem( "SkeletonBasedAnimationSystem",
                                  new Ra::Engine::Scene::SkeletonBasedAnimationSystem,
                                  defaultSystemPriority );
    }

    // initialize OpenGL part of the Engine
    m_glContext.makeCurrent();
    m_engine->initializeGL();
    m_glContext.doneCurrent();
    // Init is OK
    return 0;
}

int CLIViewer::oneFrame( float timeStep ) {
    if ( m_parameters.m_animationEnable )
    {
        auto animationsystem = dynamic_cast<Ra::Engine::Scene::SkeletonBasedAnimationSystem*>(
            m_engine->getSystem( "SkeletonBasedAnimationSystem" ) );
        if ( animationsystem ) { animationsystem->toggleSkeleton( false ); }
        m_engine->setConstantTimeStep( timeStep );
        m_engine->step();
    }

    Ra::Core::TaskQueue tasks( std::thread::hardware_concurrency() - 1 );
    m_engine->getTasks( &tasks, Scalar( timeStep ) );
    tasks.startTasks();
    tasks.waitForTasks();
    tasks.flushTaskQueue();

    Ra::Engine::Data::ViewingParameters data {
        m_camera->getViewMatrix(), m_camera->getProjMatrix(), timeStep};
    m_renderer->render( data );

    return 0;
}

std::unique_ptr<unsigned char[]> CLIViewer::grabFrame( size_t& w, size_t& h ) const {
    return m_renderer->grabFrame( w, h );
}

void CLIViewer::setRenderer( Ra::Engine::Rendering::Renderer* renderer ) {
    m_glContext.makeCurrent();

    m_renderer.reset( renderer );
    m_renderer->initialize( m_parameters.m_size[0], m_parameters.m_size[1] );

    m_glContext.doneCurrent();
}

void CLIViewer::addDataLoader( Ra::Core::Asset::FileLoaderInterface* loader ) {
    m_engine->registerFileLoader( std::shared_ptr<Ra::Core::Asset::FileLoaderInterface>( loader ) );
}

void CLIViewer::compileScene() {
    m_engine->loadFile( m_dataFile );

    auto cameraManager = static_cast<Ra::Engine::Scene::CameraManager*>(
        Ra::Engine::RadiumEngine::getInstance()->getSystem( "DefaultCameraManager" ) );

    if ( cameraManager->count() == 0 )
    {
        setDefaultCamera();
        auto aabb      = Ra::Engine::RadiumEngine::getInstance()->computeSceneAabb();
        Scalar f       = m_camera->getFOV();
        Scalar a       = m_camera->getAspect();
        const Scalar r = ( aabb.max() - aabb.min() ).norm() / 2_ra;
        const Scalar x = r / std::sin( f / 2_ra );
        const Scalar y = r / std::sin( f * a / 2_ra );
        Scalar d       = std::max( std::max( x, y ), 0.001_ra );

        m_camera->setPosition(
            Ra::Core::Vector3( aabb.center().x(), aabb.center().y(), aabb.center().z() + d ) );
        m_camera->setDirection( Ra::Core::Vector3( 0_ra, 0_ra, -1_ra ) );
        Scalar zfar =
            std::max( d + ( aabb.max().z() - aabb.min().z() ) * 2_ra, m_camera->getZFar() );
        m_camera->setZFar( zfar );
    }
    else
    { m_camera = cameraManager->getActiveCamera(); }

    if ( m_renderer )
    {

        m_renderer->buildAllRenderTechniques();

        if ( !m_renderer->hasLight() )
        {
            auto headlight = new Ra::Engine::Scene::DirectionalLight(
                Ra::Engine::Scene::SystemEntity::getInstance(), "headlight" );
            headlight->setColor( Ra::Core::Utils::Color::Grey( 2.0_ra ) );
            headlight->setDirection( m_camera->getDirection() );
            m_renderer->addLight( headlight );
        }
    }
}

void CLIViewer::openGlAddOns( std::function<void()> f ) {
    m_glContext.makeCurrent();
    f();
    m_glContext.doneCurrent();
}

void CLIViewer::bindOpenGLContext( bool on ) {
    if ( on ) { m_glContext.makeCurrent(); }
    else
    { m_glContext.doneCurrent(); }
}

void CLIViewer::setDefaultCamera() {
    // Todo : better management of a camera (from a loaded one, allowing the caller to give its
    // camera ?
    m_camera = new Ra::Core::Asset::Camera( m_parameters.m_size[0], m_parameters.m_size[1] );
    m_camera->setFOV( 60.0_ra * Ra::Core::Math::toRad );
    m_camera->setZNear( 0.1_ra );
    m_camera->setZFar( 100_ra );
}

void CLIViewer::setImageNamePrefix( std::string s ) {
    m_parameters.m_imgPrefix = std::move( s );
}

void CLIViewer::showWindow( bool on ) {
    m_exposedWindow = on;
    if ( m_exposedWindow )
    {
        m_glContext.resize( m_parameters.m_size );
        m_glContext.show();
    }
    else
    { m_glContext.hide(); }
}

void CLIViewer::swapBuffers() {
    m_glContext.swapbuffers();
}

void CLIViewer::waitForClose() {
    m_glContext.waitForClose();
}
