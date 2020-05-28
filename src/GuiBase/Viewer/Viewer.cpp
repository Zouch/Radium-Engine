#include <glbinding-aux/ContextInfo.h>
#include <glbinding-aux/debug.h>
#include <glbinding-aux/types_to_string.h>
#include <glbinding/Binding.h>
#include <glbinding/glbinding.h>

#include <glbinding/Version.h>
// Do not import namespace to prevent glbinding/QTOpenGL collision
#include <glbinding/gl/gl.h>

#include <globjects/globjects.h>

#include <Engine/RadiumEngine.hpp>

#include <Core/Asset/FileData.hpp>
#include <Core/Utils/StringUtils.hpp>
#include <GuiBase/Viewer/Viewer.hpp>

#include <iostream>

#include <QOpenGLContext>

#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include <Core/Containers/MakeShared.hpp>
#include <Core/Math/Math.hpp>
#include <Core/Utils/Color.hpp>
#include <Core/Utils/Log.hpp>
#include <Core/Utils/StringUtils.hpp>

#include <Engine/Component/Component.hpp>
#include <Engine/Renderer/Camera/Camera.hpp>
#include <Engine/Renderer/Light/DirLight.hpp>
#include <Engine/Renderer/Renderer.hpp>

#include <Engine/Managers/EntityManager/EntityManager.hpp>
#include <Engine/Managers/SystemDisplay/SystemDisplay.hpp>
#include <Engine/Renderer/RenderTechnique/ShaderProgramManager.hpp>
#include <Engine/Renderer/Renderers/ForwardRenderer.hpp>

#include <GuiBase/Utils/KeyMappingManager.hpp>
#include <GuiBase/Utils/Keyboard.hpp>
#include <GuiBase/Utils/PickingManager.hpp>

#include <GuiBase/Viewer/Gizmo/GizmoManager.hpp>
#include <GuiBase/Viewer/TrackballCameraManipulator.hpp>

namespace Ra {

using namespace Core::Utils; // log
using namespace glbinding;

#define KMA_VALUE( x ) Gui::KeyMappingManager::KeyMappingAction Gui::Viewer::x;
KeyMappingViewer
#undef KMA_VALUE

    // Register all keymapings related to the viewer and its managed functionalities (Trackball
    // camera, Gizmo, ..)
    void
    Gui::Viewer::setupKeyMappingCallbacks() {
    auto keyMappingManager = Gui::KeyMappingManager::getInstance();

    // Add default manipulator listener
    keyMappingManager->addListener( Gui::TrackballCameraManipulator::configureKeyMapping );
    // add viewer related listener
    keyMappingManager->addListener( Gui::GizmoManager::configureKeyMapping );
    keyMappingManager->addListener( configureKeyMapping );
}

void Gui::Viewer::configureKeyMapping_impl() {
    auto keyMappingManager = Gui::KeyMappingManager::getInstance();
    m_keyMappingContext    = keyMappingManager->getContext( "ViewerContext" );
    if ( m_keyMappingContext.isInvalid() )
    {
        LOG( logINFO )
            << "ViewerContext not defined (maybe the configuration file do not contains it)";
        LOG( Ra::Core::Utils::logERROR ) << "ViewerContext all keymapping invalide !";
        return;
    }

#define KMA_VALUE( XX ) XX = keyMappingManager->getActionIndex( m_keyMappingContext, #XX );
    KeyMappingViewer
#undef KMA_VALUE
}

Gui::Viewer::Viewer( QWindow* parent ) :
    WindowQt( parent ),
    m_currentRenderer( nullptr ),
    m_pickingManager( new PickingManager() ),
    m_isBrushPickingEnabled( false ),
    m_brushRadius( 10 ),
    m_camera( nullptr ),
    m_gizmoManager( nullptr )
#ifdef RADIUM_MULTITHREAD_RENDERING
    ,
    m_renderThread( nullptr )
#endif
{
}

Gui::Viewer::~Viewer() {
    if ( m_glInitialized.load() )
    {
        makeCurrent();
        m_renderers.clear();

        delete m_gizmoManager;
        doneCurrent();
    }
}

void Gui::Viewer::createGizmoManager() {
    if ( m_gizmoManager == nullptr ) { m_gizmoManager = new GizmoManager( this ); }
}

int Gui::Viewer::addRenderer( const std::shared_ptr<Engine::Renderer>& e ) {
    // initial state and lighting (deferred if GL is not ready yet)
    if ( m_glInitialized.load() )
    {
        makeCurrent();
        initializeRenderer( e.get() );
        doneCurrent();
    }
    else
    {
        LOG( logINFO ) << "[Viewer] New Renderer (" << e->getRendererName()
                       << ") added before GL being Ready: deferring initialization...";
    }

    m_renderers.push_back( e );

    return m_renderers.size() - 1;
}

void Gui::Viewer::setBackgroundColor( const Core::Utils::Color& background ) {
    m_backgroundColor = background;
    for ( const auto& renderer : m_renderers )
        renderer->setBackgroundColor( m_backgroundColor );

    emit needUpdate();
}

void Gui::Viewer::enableDebug() {
    glbinding::setCallbackMask( glbinding::CallbackMask::After |
                                glbinding::CallbackMask::ParametersAndReturnValue );
    glbinding::setAfterCallback( []( const glbinding::FunctionCall& call ) {
        std::cerr << call.function->name() << "(";
        for ( unsigned i = 0; i < call.parameters.size(); ++i )
        {
            std::cerr << call.parameters[i].get();
            if ( i < call.parameters.size() - 1 ) { std::cerr << ", "; }
        }
        std::cerr << ")";
        if ( call.returnValue ) { std::cerr << " -> " << call.returnValue.get(); }
        std::cerr << std::endl;
    } );
}

bool Gui::Viewer::initializeGL() {
    globjects::init( getProcAddress );

    LOG( logINFO ) << "*** Radium Engine OpenGL context ***";
    LOG( logINFO ) << "Renderer (glbinding) : " << glbinding::aux::ContextInfo::renderer();
    LOG( logINFO ) << "Vendor   (glbinding) : " << glbinding::aux::ContextInfo::vendor();
    LOG( logINFO ) << "OpenGL   (glbinding) : "
                   << glbinding::aux::ContextInfo::version().toString();
    LOG( logINFO ) << "GLSL                 : "
                   << gl::glGetString( gl::GLenum( GL_SHADING_LANGUAGE_VERSION ) );

    Engine::ShaderProgramManager::createInstance();
    Engine::RadiumEngine::getInstance()->registerDefaultPrograms();

    createGizmoManager();
    // create default camera interface : trackball
    m_camera = std::make_unique<Gui::TrackballCameraManipulator>( width(), height() );
    auto headlight =
        new Engine::DirectionalLight( Ra::Engine::SystemEntity::getInstance(), "headlight" );
    headlight->setColor( Ra::Core::Utils::Color::Grey( 1.0_ra ) );
    m_camera->attachLight( headlight );

    m_glInitialized = true;
    makeCurrent();

    LOG( logINFO ) << "*** Radium Engine Viewer ***";

    // initialize renderers added before GL was ready
    if ( !m_renderers.empty() )
    {
        for ( auto& rptr : m_renderers )
        {
            initializeRenderer( rptr.get() );
            LOG( logINFO ) << "[Viewer] Deferred initialization of " << rptr->getRendererName();
        }
    }

    emit glInitialized();
    doneCurrent();

    // this code is usefull only if glInitialized() connected slot does not add a renderer
    // On Windows, actually, the signal seems to be not fired (DLL_IMPORT/EXPORT problem ?
    if ( m_renderers.empty() )
    {
        LOG( logINFO )
            << "Renderers fallback: no renderer added, enabling default (Forward Renderer)";

        makeCurrent();
        std::shared_ptr<Ra::Engine::Renderer> e( new Ra::Engine::ForwardRenderer() );
        doneCurrent();

        addRenderer( e );
    }

    if ( m_currentRenderer == nullptr ) { changeRenderer( 0 ); }

    return true;
}

void Gui::Viewer::setCameraManipulator( CameraManipulator* ci ) {
    m_camera.reset( ci );
}

Gui::CameraManipulator* Gui::Viewer::getCameraManipulator() {
    return m_camera.get();
}

Gui::GizmoManager* Gui::Viewer::getGizmoManager() {
    return m_gizmoManager;
}

const Engine::Renderer* Gui::Viewer::getRenderer() const {
    return m_currentRenderer;
}

Engine::Renderer* Gui::Viewer::getRenderer() {
    return m_currentRenderer;
}

Gui::PickingManager* Gui::Viewer::getPickingManager() {
    return m_pickingManager;
}

void Gui::Viewer::onAboutToCompose() {
    // This slot function is called from the main thread as part of the event loop
    // when the GUI is about to update. We have to wait for the rendering to finish.
    m_currentRenderer->lockRendering();
}

void Gui::Viewer::onFrameSwapped() {
    // This slot is called from the main thread as part of the event loop when the
    // GUI has finished displaying the rendered image, so we unlock the renderer.
    m_currentRenderer->unlockRendering();
}

void Gui::Viewer::onAboutToResize() {
    // Like swap buffers, resizing is a blocking operation and we have to wait for the rendering
    // to finish before resizing.
    m_currentRenderer->lockRendering();
}

void Gui::Viewer::onResized() {
    m_currentRenderer->unlockRendering();
    emit needUpdate();
}

void Gui::Viewer::initializeRenderer( Engine::Renderer* renderer ) {
    // see issue #261 Qt Event order and default viewport management (Viewer.cpp)
    // https://github.com/STORM-IRIT/Radium-Engine/issues/261
#ifndef OS_MACOS
    gl::glViewport( 0, 0, width(), height() );
#endif
    renderer->initialize( width(), height() );
    renderer->setBackgroundColor( m_backgroundColor );
    // resize camera viewport since it might be 0x0
    m_camera->resizeViewport( width(), height() );
    // do this only when the renderer has something to render and that there is no lights
    /*
    if ( m_camera->hasLightAttached() )
    {
        renderer->addLight( m_camera->getLight() );
    }
    */
    renderer->lockRendering();
}

void Gui::Viewer::resizeGL( QResizeEvent* event ) {
    int width  = event->size().width();
    int height = event->size().height();
    // Renderer should have been locked by previous events.
    makeCurrent();
#ifndef OS_MACOS
    gl::glViewport( 0, 0, width, height );
#endif
    m_camera->resizeViewport( width, height );
    m_currentRenderer->resize( width, height );
    doneCurrent();
    emit needUpdate();
}

Engine::Renderer::PickingMode
Gui::Viewer::getPickingMode( const KeyMappingManager::KeyMappingAction& action ) const {
    if ( action == VIEWER_PICKING_VERTEX )
    { return m_isBrushPickingEnabled ? Engine::Renderer::C_VERTEX : Engine::Renderer::VERTEX; }
    if ( action == VIEWER_PICKING_EDGE )
    { return m_isBrushPickingEnabled ? Engine::Renderer::C_EDGE : Engine::Renderer::EDGE; }
    if ( action == VIEWER_PICKING_TRIANGLE )
    {
        return m_isBrushPickingEnabled ? Engine::Renderer::C_TRIANGLE : Engine::Renderer::TRIANGLE;
    }
    if ( action == VIEWER_PICKING ) { return Engine::Renderer::RO; }
    return Engine::Renderer::NONE;
}

enum InteractionMode { INTERACTIVE, PICKING };
InteractionMode m_interactionMode;

void Gui::Viewer::mousePressEvent( QMouseEvent* event ) {
    using Core::Utils::Color;

    if ( !m_glInitialized.load() )
    {
        event->ignore();
        return;
    }

    // get what's under the mouse

    makeCurrent();
    auto result = m_currentRenderer->doPickingNow(
        {Core::Vector2( event->x(), height() - event->y() ),
         Engine::Renderer::PickingPurpose::SELECTION,
         Engine::Renderer::RO},
        {m_camera->getViewMatrix(), m_camera->getProjMatrix(), 0.} );

    doneCurrent();

    ///\todo something like explained here
    // if under mouse objects grabs the action, just send it to the object
    // so we need to have something like
    // grabber  = renderobject(ro id)->getMouseManipulator
    // if(grabber && grabber->handleEvent(event, buttons, modifiers, key){
    // context = grabber->getContext
    // currentGrabber = grabber (we need to store it for mouse move)

    // for now just handle one active context
    m_activeContext = -1;

    m_currentRenderer->setMousePosition( Ra::Core::Vector2( event->x(), event->y() ) );

    auto keyMap    = Gui::KeyMappingManager::getInstance();
    auto buttons   = event->buttons();
    auto modifiers = event->modifiers();
    auto key       = activeKey();

    // nothing under mouse ? juste move the camera ...
    if ( result.m_roIdx.isInvalid() )
    {
        if ( m_camera->handleMousePressEvent( event, buttons, modifiers, key ) )
        { m_activeContext = m_camera->mappingContext(); }
        else
        {
            // should not pass here, since viewerContext is only for valid picking ...
            m_activeContext = KeyMappingManageable::getContext();
        }
    }
    else
    {
        // something under the mouse, let's check if it's a gizmo ro
        getGizmoManager()->handlePickingResult( result.m_roIdx );
        if ( getGizmoManager()->handleMousePressEvent( event, buttons, modifiers, key ) )
        { m_activeContext = GizmoManager::getContext(); } // if not, try to do camera stuff
        else if ( m_camera->handleMousePressEvent( event, buttons, modifiers, key ) )
        { m_activeContext = m_camera->mappingContext(); }
        else
        {
            m_activeContext  = KeyMappingManageable::getContext();
            auto action      = keyMap->getAction( m_activeContext, buttons, modifiers, key );
            auto pickingMode = getPickingMode( action );

            if ( pickingMode != Ra::Engine::Renderer::NONE )
            {
                // Push query, we may also do it here ...
                Engine::Renderer::PickingQuery query = {
                    Core::Vector2( event->x(), ( height() - event->y() ) ),
                    Engine::Renderer::PickingPurpose::MANIPULATION,
                    pickingMode};
                m_currentRenderer->addPickingRequest( query );
            }
        }
    }
    /*
     * // action == Gui::KeyMappingManager::VIEWER_RAYCAST
    LOG( logINFO ) << "Raycast query are disabled";
    auto r = m_camera->getCamera()->getRayFromScreen( Core::Vector2( event->x(), event->y() ));
    RA_DISPLAY_POINT( r.origin(), Color::Cyan(), 0.1f ); RA_DISPLAY_RAY( r, Color::Yellow() );
    }*/

    emit needUpdate();
}

void Gui::Viewer::mouseReleaseEvent( QMouseEvent* event ) {
    if ( m_activeContext == m_camera->mappingContext() )
    { m_camera->handleMouseReleaseEvent( event ); }
    if ( m_activeContext == GizmoManager::getContext() )
    { m_gizmoManager->handleMouseReleaseEvent( event ); }
    m_activeContext = -1;
    emit needUpdate();
}

void Gui::Viewer::mouseMoveEvent( QMouseEvent* event ) {
    if ( !m_glInitialized.load() )
    {
        event->ignore();
        return;
    }

    m_currentRenderer->setMousePosition( Ra::Core::Vector2( event->x(), event->y() ) );

    makeCurrent();

    auto result = m_currentRenderer->doPickingNow(
        {Core::Vector2( event->x(), height() - event->y() ),
         Engine::Renderer::PickingPurpose::SELECTION,
         Engine::Renderer::RO},
        {m_camera->getViewMatrix(), m_camera->getProjMatrix(), 0.} );
    doneCurrent();

    auto keyMap    = Gui::KeyMappingManager::getInstance();
    auto buttons   = event->buttons();
    auto modifiers = event->modifiers();
    auto key       = activeKey();
    // if needed can use
    //    auto action = keyMap->getAction( m_activeContext, buttons, modifiers, key );

    if ( m_activeContext == m_camera->mappingContext() )
    { m_camera->handleMouseMoveEvent( event, buttons, modifiers, key ); }
    else if ( m_activeContext == GizmoManager::getContext() )
    { m_gizmoManager->handleMouseMoveEvent( event, buttons, modifiers, key ); }
    else if ( m_activeContext == KeyMappingManageable::getContext() )
    {
        auto action      = keyMap->getAction( m_activeContext, buttons, modifiers, key );
        auto pickingMode = getPickingMode( action );
        if ( pickingMode != Ra::Engine::Renderer::NONE )
        {
            Engine::Renderer::PickingQuery query = {
                Core::Vector2( event->x(), ( height() - event->y() ) ),
                Engine::Renderer::PickingPurpose::MANIPULATION,
                pickingMode};
            m_currentRenderer->addPickingRequest( query );
        }
    }
    else
    { getGizmoManager()->handlePickingResult( result.m_roIdx ); }

    emit needUpdate();
}

void Gui::Viewer::wheelEvent( QWheelEvent* event ) {

    if ( !m_glInitialized.load() )
    {
        event->ignore();
        return;
    }

    auto keyMap    = Gui::KeyMappingManager::getInstance();
    auto buttons   = event->buttons();
    auto modifiers = event->modifiers();
    auto key       = activeKey();
    auto action =
        keyMap->getAction( KeyMappingManageable::getContext(), buttons, modifiers, key, true );

    if ( action == VIEWER_SCALE_BRUSH && m_isBrushPickingEnabled )
    {
        m_brushRadius +=
            ( event->angleDelta().y() * 0.01 + event->angleDelta().x() * 0.01 ) > 0 ? 5 : -5;
        m_brushRadius = std::max( m_brushRadius, Scalar( 5 ) );
        m_currentRenderer->setBrushRadius( m_brushRadius );
    }
    else
    { m_camera->handleWheelEvent( event ); }
    emit needUpdate();
}

void Gui::Viewer::keyPressEvent( QKeyEvent* event ) {

    if ( !m_glInitialized.load() )
    {
        event->ignore();
        return;
    }

    keyPressed( event->key() );
    if ( event->isAutoRepeat() ) return;

    auto keyMap    = Gui::KeyMappingManager::getInstance();
    auto buttons   = Qt::NoButton;
    auto modifiers = event->modifiers();
    auto key       = activeKey();

    // Is keymapping something of the viewer only ?
    // or should be dispatched to all receivers ?
    auto actionCamera =
        keyMap->getAction( keyMap->getContext( "CameraContext" ), buttons, modifiers, key );
    auto actionGizmo =
        keyMap->getAction( keyMap->getContext( "GizmoContext" ), buttons, modifiers, key );
    auto actionViewer =
        keyMap->getAction( keyMap->getContext( "ViewerContext" ), buttons, modifiers, key );

    if ( actionCamera.isValid() ) { m_camera->handleKeyPressEvent( event, actionCamera ); }

    if ( actionGizmo.isValid() )
    {
        // m_gizmoManager->handleKeyPressEvent( event, action );
    }

    if ( actionViewer.isValid() )
    {

        if ( actionViewer == VIEWER_TOGGLE_WIREFRAME ) { m_currentRenderer->toggleWireframe(); }
        else if ( actionViewer == VIEWER_RELOAD_SHADERS )
        { reloadShaders(); }
        else if ( actionViewer == VIEWER_PICKING_MULTI_CIRCLE )
        {
            m_isBrushPickingEnabled = !m_isBrushPickingEnabled;
            m_currentRenderer->setBrushRadius( m_isBrushPickingEnabled ? m_brushRadius : 0 );
            emit toggleBrushPicking( m_isBrushPickingEnabled );
        }
    }

    emit needUpdate();
}

void Gui::Viewer::keyReleaseEvent( QKeyEvent* event ) {
    keyReleased( event->key() );
    // auto keyMap    = Gui::KeyMappingManager::getInstance();
    // auto modifiers = event->modifiers();
    // auto key       = activeKey();
    // auto action    = keyMap->getAction( m_keyMappingContext, Qt::NoButton, modifiers, key);

    m_camera->handleKeyReleaseEvent( event );

    emit needUpdate();
}

void Gui::Viewer::showEvent( QShowEvent* ev ) {
    WindowQt::showEvent( ev );
    /// todo remove this commented code when camera init in ctr is tested on other arch.
    if ( isOpenGLInitialized() ) m_camera->resizeViewport( width(), height() );

    emit needUpdate();
}

void Gui::Viewer::reloadShaders() {
    CORE_ASSERT( m_glInitialized.load(), "OpenGL needs to be initialized reload shaders." );

    // FIXME : check thread-saefty of this.
    m_currentRenderer->lockRendering();

    makeCurrent();
    m_currentRenderer->reloadShaders();
    doneCurrent();

    m_currentRenderer->unlockRendering();

    emit needUpdate();
}

void Gui::Viewer::displayTexture( const QString& tex ) {
    CORE_ASSERT( m_glInitialized.load(), "OpenGL needs to be initialized to display textures." );

    makeCurrent();
    m_currentRenderer->lockRendering();
    m_currentRenderer->displayTexture( tex.toStdString() );
    m_currentRenderer->unlockRendering();
    doneCurrent();

    emit needUpdate();
}

bool Gui::Viewer::changeRenderer( int index ) {
    if ( m_glInitialized.load() && m_renderers[index] )
    {
        makeCurrent();

        if ( m_currentRenderer != nullptr ) { m_currentRenderer->lockRendering(); }

        m_currentRenderer = m_renderers[index].get();
        // renderers in m_renderers are supposed to be locked
        m_currentRenderer->resize( width(), height() );
        // Configure the renderObjects for this renderer
        m_currentRenderer->buildAllRenderTechniques();
        m_currentRenderer->unlockRendering();

        LOG( logINFO ) << "[Viewer] Set active renderer: " << m_currentRenderer->getRendererName();

        // resize camera viewport since the one in show event might have 0x0
        m_camera->resizeViewport( width(), height() );

        doneCurrent();
        emit rendererReady();

        emit needUpdate();
        return true;
    }
    return false;
}

// Asynchronous rendering implementation

void Gui::Viewer::startRendering( const Scalar dt ) {
    makeCurrent();
    CORE_ASSERT( m_glInitialized.load(), "OpenGL needs to be initialized before rendering." );

    CORE_ASSERT( m_currentRenderer != nullptr, "No renderer found." );

    m_pickingManager->clear();

    // TODO : as soon as everything could be computed efficiently, activate z-bounds fitting.
    // For the moment (sept 2019), request of the scene bounding box is really inefficient (all is
    // recomputed, even if no change since last computation)
    // TODO : implement better management of UI and debug render objects so that the are drawn with
    // an adequate
    //  znear/zfar values
#if 0
    // update znear/zfar to fit the scene ...
    auto roManager = Engine::RadiumEngine::getInstance()->getRenderObjectManager();
    if (roManager) {
        // TODO : make the aabb only recomputed when needed. For now, getSceneAabb loop over scene vertices to (re)compute the aabb at each call.
        auto aabb = roManager->getSceneAabb();
        if ( !aabb.isEmpty() ) {
            // tight depth bounds
            m_camera->getCamera()->fitZRange(aabb);
        } else {
            // scene is empty, reset to defaults bounds ?
            m_camera->setCameraZNear(0.1);
            m_camera->setCameraZFar(100);
        }
    }
#endif

    Engine::ViewingParameters data{m_camera->getViewMatrix(), m_camera->getProjMatrix(), dt};

    // FIXME : move this outside of the rendering loop. must be done once per renderer ...
    // if there is no light on the renderer, add the head light attached to the camera ...
    if ( !m_currentRenderer->hasLight() )
    {
        if ( m_camera->hasLightAttached() )
            m_currentRenderer->addLight( m_camera->getLight() );
        else
            LOG( logDEBUG ) << "Unable to attach the head light!";
    }
    m_currentRenderer->render( data );
}

void Gui::Viewer::swapBuffers() {
    if ( isOpenGLInitialized() && isExposed() )
    {
        m_context->swapBuffers( this );
        doneCurrent();
    }
}

void Gui::Viewer::processPicking() {
    CORE_ASSERT( m_glInitialized.load(), "OpenGL needs to be initialized before rendering." );

    CORE_ASSERT( m_currentRenderer != nullptr, "No renderer found." );

    CORE_ASSERT( m_currentRenderer->getPickingQueries().size() ==
                     m_currentRenderer->getPickingResults().size(),
                 "There should be one result per query." );

    for ( uint i = 0; i < m_currentRenderer->getPickingQueries().size(); ++i )
    {
        const Engine::Renderer::PickingQuery& query = m_currentRenderer->getPickingQueries()[i];

        if ( query.m_purpose == Engine::Renderer::PickingPurpose::MANIPULATION )
        {
            const auto& result = m_currentRenderer->getPickingResults()[i];
            m_pickingManager->setCurrent( result );
            emit rightClickPicking( result );
        }
    }
}

void Gui::Viewer::fitCameraToScene( const Core::Aabb& aabb ) {
    if ( !aabb.isEmpty() )
    {
        CORE_ASSERT( m_camera != nullptr, "No camera found." );
        m_camera->fitScene( aabb );
        emit needUpdate();
    }
    else
    { LOG( logINFO ) << "Unable to fit the camera to the scene : empty Bbox."; }
}

std::vector<std::string> Gui::Viewer::getRenderersName() const {
    std::vector<std::string> ret;

    for ( const auto& renderer : m_renderers )
    {
        if ( renderer ) { ret.push_back( renderer->getRendererName() ); }
    }

    return ret;
}

void Gui::Viewer::grabFrame( const std::string& filename ) {
    makeCurrent();

    size_t w, h;
    auto writtenPixels = m_currentRenderer->grabFrame( w, h );

    std::string ext = Core::Utils::getFileExt( filename );

    if ( ext == "bmp" ) { stbi_write_bmp( filename.c_str(), w, h, 4, writtenPixels.get() ); }
    else if ( ext == "png" )
    { stbi_write_png( filename.c_str(), w, h, 4, writtenPixels.get(), w * 4 * sizeof( uchar ) ); }
    else
    { LOG( logWARNING ) << "Cannot write frame to " << filename << " : unsupported extension"; }

    doneCurrent();
}

void Gui::Viewer::enablePostProcess( int enabled ) {
    m_currentRenderer->enablePostProcess( enabled );
}

void Gui::Viewer::enableDebugDraw( int enabled ) {
    m_currentRenderer->enableDebugDraw( enabled );
}

void Gui::Viewer::update( const Scalar dt ) {
    CORE_UNUSED( dt );
    if ( m_gizmoManager != nullptr ) { m_gizmoManager->updateValues(); }
}

void Gui::Viewer::setCamera( Engine::Camera* camera ) {
    m_camera->setCamera( camera );
}
} // namespace Ra
