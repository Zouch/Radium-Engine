#include <Engine/RadiumEngine.hpp>

#include <Core/Asset/FileData.hpp>
#include <Core/Asset/FileLoaderInterface.hpp>
#include <Core/Resources/Resources.hpp>
#include <Core/Utils/StringUtils.hpp>

#include <Engine/Data/BlinnPhongMaterial.hpp>
#include <Engine/Data/LambertianMaterial.hpp>
#include <Engine/Data/MaterialConverters.hpp>
#include <Engine/Data/PlainMaterial.hpp>
#include <Engine/Data/ShaderConfigFactory.hpp>
#include <Engine/Data/ShaderProgramManager.hpp>
#include <Engine/Data/TextureManager.hpp>
#include <Engine/Data/VolumetricMaterial.hpp>
#include <Engine/FrameInfo.hpp>
#include <Engine/Rendering/RenderObject.hpp>
#include <Engine/Rendering/RenderObjectManager.hpp>
#include <Engine/Scene/ComponentMessenger.hpp>
#include <Engine/Scene/DefaultCameraManager.hpp>
#include <Engine/Scene/Entity.hpp>
#include <Engine/Scene/EntityManager.hpp>
#include <Engine/Scene/SignalManager.hpp>
#include <Engine/Scene/System.hpp>
#include <Engine/Scene/SystemDisplay.hpp>

#include <cstdio>
#include <iostream>
#include <streambuf>
#include <string>
#include <thread>

namespace Ra {
namespace Engine {

using namespace Core::Utils; // log
using namespace Core::Asset;

RadiumEngine::RadiumEngine() = default;

RadiumEngine::~RadiumEngine() = default;

void RadiumEngine::initialize() {
    LOG( logINFO ) << "*** Radium Engine ***";
    auto resourceDir { Core::Resources::getRadiumResourcesPath() };
    if ( !resourceDir ) {
        LOG( logWARNING )
            << "Default Radium resources dir not found. Setting resources path to \".\"";
        resourceDir = { "." };
    }
    m_resourcesRootDir     = *resourceDir;
    m_signalManager        = std::make_unique<Scene::SignalManager>();
    m_entityManager        = std::make_unique<Scene::EntityManager>();
    m_renderObjectManager  = std::make_unique<Rendering::RenderObjectManager>();
    m_textureManager       = std::make_unique<Data::TextureManager>();
    m_shaderProgramManager = std::make_unique<Data::ShaderProgramManager>();

    m_loadedFile.reset();
    Scene::ComponentMessenger::createInstance();

    auto cameraManager = new Scene::DefaultCameraManager();
    cameraManager->initialize();
    Ra::Engine::RadiumEngine::getInstance()->registerSystem( "DefaultCameraManager",
                                                             cameraManager );

    m_loadingState = false;
}

void RadiumEngine::initializeGL() {
    m_openglState = std::make_unique<globjects::State>( globjects::State::DeferredMode );
    registerDefaultPrograms();
    // needed to upload non multiple of 4 width texture loaded with stbi.
    m_openglState->pixelStore( GL_UNPACK_ALIGNMENT, 1 );
    m_openglState->apply();
}

void RadiumEngine::registerDefaultPrograms() {

    CORE_ASSERT( m_shaderProgramManager != nullptr,
                 "ShaderProgramManager needs to be created first" );

    // Create named strings which correspond to shader files that you want to use in shaders's
    // includes. NOTE: if you want to add a named string to handle a new shader include file, be
    // SURE that the name (first parameter) begin with a "/", otherwise it won't work !
    // Radium V2 : are these initialization required here ? They will be better in
    // Engine::Initialize .... Define a better ressources management and initialization
    // Add named string require opengl context, must be init before (e.g. by viewer)
    /* Default definiton of a transformation matrices struct */
    m_shaderProgramManager->addNamedString(
        "/TransformStructs.glsl", m_resourcesRootDir + "Shaders/Transform/TransformStructs.glsl" );
    m_shaderProgramManager->addNamedString(
        "/DefaultLight.glsl", m_resourcesRootDir + "Shaders/Lights/DefaultLight.glsl" );

    // VertexAttribInterface :add this name string so that each material could include the same code
    m_shaderProgramManager->addNamedString(
        "/VertexAttribInterface.frag.glsl",
        m_resourcesRootDir + "Shaders/Materials/VertexAttribInterface.frag.glsl" );

    // Engine support some built-in materials. Register here
    /// @todo find a way to integrate "Line" material into Radium Material System
    Data::ShaderConfiguration lConfig( "Lines" );
    lConfig.addShader( Data::ShaderType_VERTEX,
                       m_resourcesRootDir + "Shaders/Lines/Lines.vert.glsl" );
    lConfig.addShader( Data::ShaderType_FRAGMENT,
                       m_resourcesRootDir + "Shaders/Lines/Lines.frag.glsl" );
    Data::ShaderConfigurationFactory::addConfiguration( lConfig );

    Data::ShaderConfiguration lgConfig( "LinesGeom" );
    lgConfig.addShader( Data::ShaderType_VERTEX,
                        m_resourcesRootDir + "Shaders/Lines/Lines.vert.glsl" );
    lgConfig.addShader( Data::ShaderType_FRAGMENT,
                        m_resourcesRootDir + "Shaders/Lines/Lines.frag.glsl" );
    lgConfig.addShader( Data::ShaderType_GEOMETRY,
                        m_resourcesRootDir + "Shaders/Lines/Lines.geom.glsl" );
    Data::ShaderConfigurationFactory::addConfiguration( lgConfig );

    Data::ShaderConfiguration lagConfig( "LinesAdjacencyGeom" );
    lagConfig.addShader( Data::ShaderType_VERTEX,
                         m_resourcesRootDir + "Shaders/Lines/Lines.vert.glsl" );
    lagConfig.addShader( Data::ShaderType_FRAGMENT,
                         m_resourcesRootDir + "Shaders/Lines/LinesAdjacency.frag.glsl" );
    lagConfig.addShader( Data::ShaderType_GEOMETRY,
                         m_resourcesRootDir + "Shaders/Lines/Lines.geom.glsl" );
    Data::ShaderConfigurationFactory::addConfiguration( lagConfig );

    // Plain is flat or diffuse
    Data::PlainMaterial::registerMaterial();
    Data::BlinnPhongMaterial::registerMaterial();
    Data::LambertianMaterial::registerMaterial();
    Data::VolumetricMaterial::registerMaterial();
}

void RadiumEngine::cleanup() {
    Data::PlainMaterial::unregisterMaterial();
    Data::BlinnPhongMaterial::unregisterMaterial();
    Data::LambertianMaterial::unregisterMaterial();
    Data::VolumetricMaterial::unregisterMaterial();
    m_signalManager->setOn( false );
    m_entityManager.reset();
    m_renderObjectManager.reset();
    m_textureManager.reset( nullptr );
    m_shaderProgramManager.reset( nullptr );

    m_loadedFile.reset();

    for ( auto& system : m_systems ) {
        system.second.reset();
    }

    Scene::ComponentMessenger::destroyInstance();

    m_loadingState = false;
}

void RadiumEngine::endFrameSync() {
    m_entityManager->swapBuffers();
    m_signalManager->fireFrameEnded();
}

void RadiumEngine::getTasks( Core::TaskQueue* taskQueue, Scalar dt ) {
    static uint frameCounter = 0;

    if ( m_timeData.m_play || m_timeData.m_singleStep ) {
        m_timeData.updateTime( dt );
        m_timeData.m_singleStep = false;
    }

    FrameInfo frameInfo {
        m_timeData.m_time, m_timeData.m_realTime ? dt : m_timeData.m_dt, frameCounter++ };
    for ( auto& syst : m_systems ) {
        syst.second->generateTasks( taskQueue, frameInfo );
    }
}

bool RadiumEngine::registerSystem( const std::string& name, Scene::System* system, int priority ) {
    if ( findSystem( name ) != m_systems.end() ) {
        LOG( logWARNING ) << "Try to add system " << name.c_str()
                          << " multiple time. Keep the already registered one.";
        return false;
    }

    m_systems[std::make_pair( priority, name )] = std::shared_ptr<Scene::System>( system );
    LOG( logINFO ) << "Loaded : " << name;
    return true;
}

Scene::System* RadiumEngine::getSystem( const std::string& system ) const {
    Scene::System* sys = nullptr;
    auto it            = findSystem( system );

    if ( it != m_systems.end() ) { sys = it->second.get(); }

    return sys;
}

Data::Displayable* RadiumEngine::getMesh( const std::string& entityName,
                                          const std::string& componentName,
                                          const std::string& roName ) const {

    // 1) Get entity
    if ( m_entityManager->entityExists( entityName ) ) {
        auto e = m_entityManager->getEntity( entityName );

        // 2) Get component
        const auto c = e->getComponent( componentName );

        if ( c != nullptr && !c->m_renderObjects.empty() ) {
            // 3) Get RO
            if ( roName.empty() ) {
                return m_renderObjectManager->getRenderObject( c->m_renderObjects.front() )
                    ->getMesh()
                    .get();
            }
            else {
                for ( const auto& idx : c->m_renderObjects ) {
                    const auto& ro = m_renderObjectManager->getRenderObject( idx );
                    if ( ro->getName() == roName ) { return ro->getMesh().get(); }
                }
            }
        }
    }
    return nullptr;
}

bool RadiumEngine::loadFile( const std::string& filename ) {
    releaseFile();

    std::string extension = Core::Utils::getFileExt( filename );

    for ( auto& l : m_fileLoaders ) {
        if ( l->handleFileExtension( extension ) ) {
            FileData* data = l->loadFile( filename );
            if ( data != nullptr ) {
                m_loadedFile.reset( data );
                break;
            }
        }
    }

    if ( m_loadedFile == nullptr ) {
        LOG( logERROR ) << "There is no loader to handle \"" << extension
                        << "\" extension ! File can't be loaded.";

        return false;
    }

    std::string entityName = Core::Utils::getBaseName( filename, false );

    Scene::Entity* entity = m_entityManager->createEntity( entityName );

    for ( auto& system : m_systems ) {
        system.second->handleAssetLoading( entity, m_loadedFile.get() );
    }

    if ( !entity->getComponents().empty() ) {
        for ( auto& comp : entity->getComponents() ) {
            comp->initialize();
        }
    }
    else {
        LOG( logWARNING ) << "File \"" << filename << "\" has no usable data. Deleting entity...";
        m_entityManager->removeEntity( entity );
    }
    m_loadingState = true;
    return true;
}

void RadiumEngine::releaseFile() {
    m_loadedFile.reset( nullptr );
    m_loadingState = false;
}

Rendering::RenderObjectManager* RadiumEngine::getRenderObjectManager() const {
    return m_renderObjectManager.get();
}

Scene::EntityManager* RadiumEngine::getEntityManager() const {
    return m_entityManager.get();
}

Scene::SignalManager* RadiumEngine::getSignalManager() const {
    return m_signalManager.get();
}

Data::TextureManager* RadiumEngine::getTextureManager() const {
    return m_textureManager.get();
}

Data::ShaderProgramManager* RadiumEngine::getShaderProgramManager() const {
    return m_shaderProgramManager.get();
}

void RadiumEngine::registerFileLoader( std::shared_ptr<FileLoaderInterface> fileLoader ) {
    m_fileLoaders.push_back( fileLoader );
}

const std::vector<std::shared_ptr<FileLoaderInterface>>& RadiumEngine::getFileLoaders() const {
    return m_fileLoaders;
}

RA_SINGLETON_IMPLEMENTATION( RadiumEngine );

const FileData& RadiumEngine::getFileData() const {
    CORE_ASSERT( m_loadingState, "Access to file content is only available at loading time." );
    return *( m_loadedFile.get() );
}

RadiumEngine::SystemContainer::const_iterator
RadiumEngine::findSystem( const std::string& name ) const {
    return std::find_if( m_systems.cbegin(), m_systems.cend(), [&name]( const auto& el ) {
        return el.first.second == name;
    } );
}

RadiumEngine::SystemContainer::iterator RadiumEngine::findSystem( const std::string& name ) {
    return std::find_if( m_systems.begin(), m_systems.end(), [&name]( const auto& el ) {
        return el.first.second == name;
    } );
}

Core::Aabb RadiumEngine::computeSceneAabb() const {

    Core::Aabb aabb;

    const auto& systemEntity = Scene::SystemEntity::getInstance();
    auto entities            = m_entityManager->getEntities();
    for ( const auto& entity : entities ) {
        if ( entity != systemEntity ) aabb.extend( entity->computeAabb() );
    }
    return aabb;
}

void RadiumEngine::pushFboAndViewport() {
    int activeFbo;
    std::array<int, 4> activeViewport;

    glGetIntegerv( GL_VIEWPORT, activeViewport.data() );
    // save the currently bound FBO
    GL_ASSERT( glGetIntegerv( GL_FRAMEBUFFER_BINDING, &activeFbo ) );
    // Set the internal rendering viewport
    m_fboAndViewportStack.emplace( activeFbo, std::move( activeViewport ) );
}

void RadiumEngine::popFboAndViewport() {
    if ( m_fboAndViewportStack.empty() ) {
        LOG( logERROR ) << "RadiumEngine: try to pop from an empty Fbo and Viewport stack\n";
    }
    else {
        auto b = m_fboAndViewportStack.top();

        glViewport( b.m_viewport[0], b.m_viewport[1], b.m_viewport[2], b.m_viewport[3] );
        GL_ASSERT( glBindFramebuffer( GL_FRAMEBUFFER, b.m_fbo ) );
        m_fboAndViewportStack.pop();
    }
}

void RadiumEngine::setRealTime( bool realTime ) {
    m_timeData.m_realTime = realTime;
}

bool RadiumEngine::isRealTime() const {
    return m_timeData.m_realTime;
}

bool RadiumEngine::isConstantTime() const {
    return !m_timeData.m_realTime;
}

bool RadiumEngine::setConstantTimeStep( Scalar dt, bool forceConstantTime ) {
    m_timeData.m_dt = dt;
    if ( forceConstantTime ) { setRealTime( false ); }
    return !m_timeData.m_realTime;
}

void RadiumEngine::play( bool isPlaying ) {
    m_timeData.m_play = isPlaying;
}

void RadiumEngine::step() {
    m_timeData.m_singleStep = true;
}

void RadiumEngine::resetTime() {
    m_timeData.m_play       = false;
    m_timeData.m_singleStep = false;
    m_timeData.m_time       = m_timeData.m_startTime;
}

void RadiumEngine::setTime( Scalar t ) {
    m_timeData.m_time = t;
}

void RadiumEngine::setStartTime( Scalar t ) {
    m_timeData.m_startTime = std::max( t, 0_ra );
}

Scalar RadiumEngine::getStartTime() const {
    return m_timeData.m_startTime;
}

void RadiumEngine::setEndTime( Scalar t ) {
    m_timeData.m_endTime = t;
}

Scalar RadiumEngine::getEndTime() const {
    return m_timeData.m_endTime;
}

void RadiumEngine::setForwardBackward( bool mode ) {
    m_timeData.m_forwardBackward = mode;
    // if just disabled forward-backward mode, then going forward
    if ( !m_timeData.m_forwardBackward ) { m_timeData.m_isBackward = false; }
}

Scalar RadiumEngine::getTime() const {
    return m_timeData.m_time;
}

uint RadiumEngine::getFrame() const {
    return uint( std::ceil( m_timeData.m_time / m_timeData.m_dt ) );
}

void RadiumEngine::TimeData::updateTime( Scalar dt ) {
    dt += m_realTime ? dt : m_dt;
    // update the time w.r.t. the time flow policy
    if ( m_forwardBackward && m_isBackward ) { m_time -= dt; }
    else {
        m_time += dt;
    }
    // special case: empty time window => forever mode
    if ( m_endTime < 0 || m_startTime >= m_endTime ) {
        // just run forever
        m_isBackward = false;
        return;
    }
    // special case: m_time before time window
    if ( m_time < m_startTime ) {
        // reset whatever the mode
        m_time = m_startTime;
        // and go forward from now on
        m_isBackward = false;
        return;
    }
    // special case: m_time after time window in loop mode
    if ( !m_forwardBackward && m_time > m_endTime ) {
        // compute the overload of time
        dt = Scalar( fmod( double( m_time - m_startTime ), double( m_endTime - m_startTime ) ) );
        // loop around, applying the overload of time
        m_time = m_startTime + dt;
        return;
    }
    // special case: m_time after time window in forward-backward mode
    if ( m_forwardBackward && m_time > m_endTime ) {
        if ( !m_isBackward ) // start backwards
        {
            m_time = 2 * m_endTime - m_time;
        }
        else // restart backward from the end of the time window
        {
            m_time = m_endTime;
        }
        m_isBackward = true;
        return;
    }
}

} // namespace Engine
} // namespace Ra
