#include <Engine/Rendering/Renderer.hpp>

#include <Core/Asset/FileData.hpp>
#include <Core/Geometry/MeshPrimitives.hpp>
#include <Core/Utils/Log.hpp>
#include <Engine/Data/Material.hpp>
#include <Engine/Data/Mesh.hpp>
#include <Engine/Data/ShaderConfigFactory.hpp>
#include <Engine/Data/ShaderProgram.hpp>
#include <Engine/Data/ShaderProgramManager.hpp>
#include <Engine/Data/Texture.hpp>
#include <Engine/Data/TextureManager.hpp>
#include <Engine/Data/ViewingParameters.hpp>
#include <Engine/OpenGL.hpp>
#include <Engine/RadiumEngine.hpp>
#include <Engine/Rendering/RenderObject.hpp>
#include <Engine/Rendering/RenderObjectManager.hpp>
#include <Engine/Scene/LightManager.hpp>

#include <globjects/Framebuffer.h>

#include <globjects/Texture.h>

#include <algorithm>
#include <iostream>

namespace Ra {
namespace Engine {
namespace Rendering {

using namespace Core::Utils; // log

namespace {
const GLenum buffers[] = { GL_COLOR_ATTACHMENT0,
                           GL_COLOR_ATTACHMENT1,
                           GL_COLOR_ATTACHMENT2,
                           GL_COLOR_ATTACHMENT3,
                           GL_COLOR_ATTACHMENT4,
                           GL_COLOR_ATTACHMENT5,
                           GL_COLOR_ATTACHMENT6,
                           GL_COLOR_ATTACHMENT7 };
}

Renderer::Renderer() :
    m_quadMesh { nullptr },
    m_depthTexture { nullptr },
    m_fancyTexture { nullptr },
    m_pickingFbo { nullptr },
    m_pickingTexture { nullptr } {}

Renderer::~Renderer() = default;

void Renderer::initialize( uint width, uint height ) {
    /// For internal resources management in a filesystem
    auto resourcesRootDir { RadiumEngine::getInstance()->getResourcesDir() };

    // Get aliases on frequently used managers from the Engine
    // Not that ownership is never transfered when using raw pointers
    // See :
    // https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Ri-raw
    // https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rr-ptr
    m_renderObjectManager  = RadiumEngine::getInstance()->getRenderObjectManager();
    m_shaderProgramManager = RadiumEngine::getInstance()->getShaderProgramManager();

    m_width  = width;
    m_height = height;

    m_shaderProgramManager->addShaderProgram(
        { { "DrawScreen" },
          resourcesRootDir + "Shaders/2DShaders/Basic2D.vert.glsl",
          resourcesRootDir + "Shaders/2DShaders/DrawScreen.frag.glsl" } );
    m_shaderProgramManager->addShaderProgram(
        { { "DrawScreenI" },
          resourcesRootDir + "Shaders/2DShaders/Basic2D.vert.glsl",
          resourcesRootDir + "Shaders/2DShaders/DrawScreenI.frag.glsl" } );
    m_shaderProgramManager->addShaderProgram(
        { { "CircleBrush" },
          resourcesRootDir + "Shaders/2DShaders/Basic2D.vert.glsl",
          resourcesRootDir + "Shaders/2DShaders/CircleBrush.frag.glsl" } );
    m_shaderProgramManager->addShaderProgram(
        { { "DisplayDepthBuffer" },
          resourcesRootDir + "Shaders/2DShaders/Basic2D.vert.glsl",
          resourcesRootDir + "Shaders/2DShaders//DepthDisplay.frag.glsl" } );

    Data::ShaderConfiguration pickingPointsConfig( "PickingPoints" );
    pickingPointsConfig.addShader( Data::ShaderType_VERTEX,
                                   resourcesRootDir + "Shaders/Picking/Picking.vert.glsl" );
    pickingPointsConfig.addShader( Data::ShaderType_GEOMETRY,
                                   resourcesRootDir + "Shaders/Picking/PickingPoints.geom.glsl" );
    pickingPointsConfig.addShader( Data::ShaderType_FRAGMENT,
                                   resourcesRootDir + "Shaders/Picking/Picking.frag.glsl" );
    Data::ShaderConfigurationFactory::addConfiguration( pickingPointsConfig );
    {
        auto pickingShader = m_shaderProgramManager->addShaderProgram( pickingPointsConfig );
        CORE_ASSERT( pickingShader, "Picking Shader is required for points" );
        m_pickingShaders[Data::Displayable::PKM_POINTS] = *pickingShader;
    }

    Data::ShaderConfiguration pickingLinesConfig( "PickingLines" );
    pickingLinesConfig.addShader( Data::ShaderType_VERTEX,
                                  resourcesRootDir + "Shaders/Picking/Picking.vert.glsl" );
    pickingLinesConfig.addShader( Data::ShaderType_GEOMETRY,
                                  resourcesRootDir + "Shaders/Picking/PickingLines.geom.glsl" );
    pickingLinesConfig.addShader( Data::ShaderType_FRAGMENT,
                                  resourcesRootDir + "Shaders/Picking/Picking.frag.glsl" );
    Data::ShaderConfigurationFactory::addConfiguration( pickingLinesConfig );
    {
        auto pickingShader = m_shaderProgramManager->addShaderProgram( pickingLinesConfig );
        CORE_ASSERT( pickingShader, "Picking Shader is required for lines" );
        m_pickingShaders[Data::Displayable::PKM_LINES] = *pickingShader;
    }

    Data::ShaderConfiguration pickingLinesAdjacencyConfig( "PickingLinesAdjacency" );
    pickingLinesAdjacencyConfig.addShader( Data::ShaderType_VERTEX,
                                           resourcesRootDir + "Shaders/Picking/Picking.vert.glsl" );
    pickingLinesAdjacencyConfig.addShader( Data::ShaderType_GEOMETRY,
                                           resourcesRootDir +
                                               "Shaders/Picking/PickingLinesAdjacency.geom.glsl" );
    pickingLinesAdjacencyConfig.addShader( Data::ShaderType_FRAGMENT,
                                           resourcesRootDir + "Shaders/Picking/Picking.frag.glsl" );
    Data::ShaderConfigurationFactory::addConfiguration( pickingLinesAdjacencyConfig );
    {
        auto pickingShader =
            m_shaderProgramManager->addShaderProgram( pickingLinesAdjacencyConfig );
        CORE_ASSERT( pickingShader, "Picking Shader is required for lines adjacency" );
        m_pickingShaders[Data::Displayable::PKM_LINE_ADJ] = *pickingShader;
    }

    Data::ShaderConfiguration pickingTrianglesConfig( "PickingTriangles" );
    pickingTrianglesConfig.addShader( Data::ShaderType_VERTEX,
                                      resourcesRootDir + "Shaders/Picking/Picking.vert.glsl" );
    pickingTrianglesConfig.addShader( Data::ShaderType_GEOMETRY,
                                      resourcesRootDir +
                                          "Shaders/Picking/PickingTriangles.geom.glsl" );
    pickingTrianglesConfig.addShader( Data::ShaderType_FRAGMENT,
                                      resourcesRootDir + "Shaders/Picking/Picking.frag.glsl" );
    Data::ShaderConfigurationFactory::addConfiguration( pickingTrianglesConfig );
    {
        auto pickingShader = m_shaderProgramManager->addShaderProgram( pickingTrianglesConfig );
        CORE_ASSERT( pickingShader, "Picking Shader is required for triangles" );
        m_pickingShaders[Data::Displayable::PKM_TRI] = *pickingShader;
    }

    Data::TextureParameters texparams;
    texparams.width     = m_width;
    texparams.height    = m_height;
    texparams.target    = GL_TEXTURE_2D;
    texparams.minFilter = GL_NEAREST;
    texparams.magFilter = GL_NEAREST;

    texparams.name           = "Depth";
    texparams.internalFormat = GL_DEPTH_COMPONENT24;
    texparams.format         = GL_DEPTH_COMPONENT;
    texparams.type           = GL_UNSIGNED_INT;
    m_depthTexture           = std::make_unique<Data::Texture>( texparams );

    m_pickingFbo             = std::make_unique<globjects::Framebuffer>();
    texparams.name           = "Picking";
    texparams.internalFormat = GL_RGBA32I;
    texparams.format         = GL_RGBA_INTEGER;
    texparams.type           = GL_INT;
    m_pickingTexture         = std::make_unique<Data::Texture>( texparams );

    // Final texture
    texparams.name           = "Final image";
    texparams.internalFormat = GL_RGBA32F;
    texparams.format         = GL_RGBA;
    texparams.type           = GL_SCALAR;
    m_fancyTexture           = std::make_unique<Data::Texture>( texparams );

    m_displayedTexture                     = m_fancyTexture.get();
    m_secondaryTextures["Picking Texture"] = m_pickingTexture.get();

    // Quad mesh
    Core::Geometry::TriangleMesh mesh =
        Core::Geometry::makeZNormalQuad( Core::Vector2( -1.f, 1.f ) );

    auto qm = std::make_unique<Data::Mesh>( "quad" );
    qm->loadGeometry( std::move( mesh ) );
    m_quadMesh = std::move( qm ); // we need to move, as loadGeometry is not a member of Displayable
    m_quadMesh->updateGL();

    initializeInternal();

    resize( m_width, m_height );
}

Renderer::PickingResult Renderer::doPickingNow( const PickingQuery& query,
                                                const Data::ViewingParameters& renderData ) {
    CORE_ASSERT( RadiumEngine::getInstance() != nullptr, "Engine is not initialized." );
    // skip query if out of window (can occur when picking while moving outside)
    if ( query.m_screenCoords.x() < 0 || query.m_screenCoords.x() > m_width - 1 ||
         query.m_screenCoords.y() < 0 || query.m_screenCoords.y() > m_height - 1 )
    {
        // return empty result
        return {};
    }

    PickingResult result;

    std::lock_guard<std::mutex> renderLock( m_renderMutex );
    CORE_UNUSED( renderLock );

    // 0. Save eventual already bound FBO (e.g. QtOpenGLWidget) and viewport
    saveExternalFBOInternal();

    // 1. Gather render objects if needed
    feedRenderQueuesInternal( renderData );
    updateRenderObjectsInternal( renderData );
    // 3. Do picking if needed

    m_pickingFbo->bind();
    /// \todo Fixup prepare picking to take pixel position and rendering window, and read depth in
    /// it.
    // preparePicking( renderData );
    // here is preparePicking code + save depth
    GL_ASSERT( glDepthMask( GL_TRUE ) );
    GL_ASSERT( glColorMask( 1, 1, 1, 1 ) );
    GL_ASSERT( glDrawBuffers( 1, buffers ) );

    float clearDepth = 1.0;
    int clearColor[] = { -1, -1, -1, -1 };

    GL_ASSERT( glClearBufferiv( GL_COLOR, 0, clearColor ) );
    GL_ASSERT( glClearBufferfv( GL_DEPTH, 0, &clearDepth ) );

    splitRenderQueuesForPicking( renderData );

    // First draw Geometry Objects
    GL_ASSERT( glEnable( GL_DEPTH_TEST ) );
    GL_ASSERT( glDepthFunc( GL_LESS ) );

    renderForPicking( renderData, m_pickingShaders, m_fancyRenderObjectsPicking );
    ////////////////// added save depth //////////////////////////////////////
    float depth;
    m_pickingFbo->readPixels( { static_cast<int>( query.m_screenCoords.x() ),
                                static_cast<int>( query.m_screenCoords.y() ),
                                1,
                                1 },
                              GL_DEPTH_COMPONENT,
                              GL_FLOAT,
                              &depth );
    result.setDepth( depth );

    //////////////////////////////////////////////////////////////////////////

    if ( m_drawDebug )
    {
        // Then draw debug objects
        GL_ASSERT( glClearBufferfv( GL_DEPTH, 0, &clearDepth ) );
        renderForPicking( renderData, m_pickingShaders, m_debugRenderObjectsPicking );
        // Then draw xrayed objects on top of normal objects
        GL_ASSERT( glClearBufferfv( GL_DEPTH, 0, &clearDepth ) );
        renderForPicking( renderData, m_pickingShaders, m_xrayRenderObjectsPicking );
    }

    // Finally draw ui stuff on top of everything
    // these have a different way to compute the transform matrices
    // FIXME (florian): find a way to use renderForPicking()!
    GL_ASSERT( glClearBufferfv( GL_DEPTH, 0, &clearDepth ) );
    for ( uint i = 0; i < m_pickingShaders.size(); ++i )
    {
        m_pickingShaders[i]->bind();
        m_pickingShaders[i]->setUniform( "transform.proj", renderData.projMatrix );
        m_pickingShaders[i]->setUniform( "transform.view", renderData.viewMatrix );

        for ( const auto& ro : m_uiRenderObjectsPicking[i] )
        {
            if ( ro->isVisible() && ro->isPickable() )
            {
                m_pickingShaders[i]->setUniform( "objectId", ro->getIndex().getValue() );

                Core::Matrix4 M  = ro->getTransformAsMatrix();
                Core::Matrix4 MV = renderData.viewMatrix * M;
                Scalar d         = MV.block<3, 1>( 0, 3 ).norm();

                Core::Matrix4 S = Core::Matrix4::Identity();
                S( 0, 0 ) = S( 1, 1 ) = S( 2, 2 ) = d;

                M               = M * S;
                Core::Matrix4 N = M.inverse().transpose();

                m_pickingShaders[i]->setUniform( "transform.model", M );
                m_pickingShaders[i]->setUniform( "transform.worldNormal", N );

                // render
                ro->getMesh()->render( m_pickingShaders[i] );
            }
        }
    }

    ///////////////////////

    int pick[4];

    // Now read the Picking Texture to address the Picking Requests.
    m_pickingFbo->readPixels( GL_COLOR_ATTACHMENT0,
                              { static_cast<int>( query.m_screenCoords.x() ),
                                static_cast<int>( query.m_screenCoords.y() ),
                                1,
                                1 },
                              GL_RGBA_INTEGER,
                              GL_INT,
                              pick );
    result.setRoIdx( pick[0] ); // RO idx
    result.addIndex( { pick[2], pick[1], pick[3] } );
    result.setMode( query.m_mode );
    m_pickingFbo->unbind();

    ///////

    restoreExternalFBOInternal();

    return result;
} // namespace Engine

void Renderer::render( const Data::ViewingParameters& data ) {
    if ( !m_initialized ) return;

    CORE_ASSERT( RadiumEngine::getInstance() != nullptr, "Engine is not initialized." );

    std::lock_guard<std::mutex> renderLock( m_renderMutex );
    CORE_UNUSED( renderLock );

    m_timerData.renderStart = Core::Utils::Clock::now();

    // 0. Save eventual already bound FBO (e.g. QtOpenGLWidget) and viewport
    saveExternalFBOInternal();

    // 1. Gather render objects if needed
    // TODO : make this only once and only update modified objects at each frame
    //  Actually, this correspond to 3 loops over all ROs. Could be done without loops, just by
    //  using observers
    feedRenderQueuesInternal( data );

    m_timerData.feedRenderQueuesEnd = Core::Utils::Clock::now();

    // 2. Update them (from an opengl point of view)
    // TODO : This naively updates the OpenGL State of objects at each frame.
    //  Do it only for modified objects (With an observer ?)
    updateRenderObjectsInternal( data );
    m_timerData.updateEnd = Core::Utils::Clock::now();

    // 3. Do picking if needed
    // TODO : Make picking much more effient.
    //  Do not need to loop twice on objects to implement picking.
    m_pickingResults.clear();
    if ( !m_pickingQueries.empty() ) { doPicking( data ); }
    m_lastFramePickingQueries = m_pickingQueries;
    m_pickingQueries.clear();

    updateStepInternal( data );

    // 4. Do the rendering.
    renderInternal( data );
    m_timerData.mainRenderEnd = Core::Utils::Clock::now();

    // 5. Post processing
    postProcessInternal( data );
    m_timerData.postProcessEnd = Core::Utils::Clock::now();

    // 6. Debug
    debugInternal( data );

    // 7. Draw UI
    uiInternal( data );

    // 8. Write image to Qt framebuffer.
    drawScreenInternal();
    m_timerData.renderEnd = Core::Utils::Clock::now();

    // 9. Tell renderobjects they have been drawn (to decreaase the counter)
    // TODO : this must be done when rendering the object, not after.
    // doing this here make looping on Ros twice (at least) and even much more due to
    // implementations of indirectly called methods.
    notifyRenderObjectsRenderingInternal();
}

void Renderer::saveExternalFBOInternal() {
    RadiumEngine::getInstance()->pushFboAndViewport();
    // Set the internal rendering viewport
    glViewport( 0, 0, int( m_width ), int( m_height ) );
}

void Renderer::updateRenderObjectsInternal( const Data::ViewingParameters& /*renderData*/ ) {
    for ( auto& ro : m_fancyRenderObjects )
    { ro->updateGL(); }
    for ( auto& ro : m_xrayRenderObjects )
    { ro->updateGL(); }
    for ( auto& ro : m_debugRenderObjects )
    { ro->updateGL(); }
    for ( auto& ro : m_uiRenderObjects )
    { ro->updateGL(); }
}

void Renderer::feedRenderQueuesInternal( const Data::ViewingParameters& /*renderData*/ ) {
    m_fancyRenderObjects.clear();
    m_debugRenderObjects.clear();
    m_uiRenderObjects.clear();
    m_xrayRenderObjects.clear();

    m_renderObjectManager->getRenderObjectsByType( m_fancyRenderObjects,
                                                   RenderObjectType::Geometry );
    m_renderObjectManager->getRenderObjectsByType( m_debugRenderObjects, RenderObjectType::Debug );
    m_renderObjectManager->getRenderObjectsByType( m_uiRenderObjects, RenderObjectType::UI );

    for ( auto it = m_fancyRenderObjects.begin(); it != m_fancyRenderObjects.end(); )
    {
        if ( ( *it )->isXRay() )
        {
            m_xrayRenderObjects.push_back( *it );
            it = m_fancyRenderObjects.erase( it );
        }
        else
        { ++it; }
    }

    for ( auto it = m_debugRenderObjects.begin(); it != m_debugRenderObjects.end(); )
    {
        if ( ( *it )->isXRay() )
        {
            m_xrayRenderObjects.push_back( *it );
            it = m_debugRenderObjects.erase( it );
        }
        else
        { ++it; }
    }

    for ( auto it = m_uiRenderObjects.begin(); it != m_uiRenderObjects.end(); )
    {
        if ( ( *it )->isXRay() )
        {
            m_xrayRenderObjects.push_back( *it );
            it = m_uiRenderObjects.erase( it );
        }
        else
        { ++it; }
    }
}

// subroutine to Renderer::splitRenderQueuesForPicking()
void Renderer::splitRQ( const std::vector<RenderObjectPtr>& renderQueue,
                        std::array<std::vector<RenderObjectPtr>, 4>& renderQueuePicking ) {
    // clean renderQueuePicking
    for ( auto& q : renderQueuePicking )
    { q.clear(); }

    // fill renderQueuePicking from renderQueue
    for ( auto& roPtr : renderQueue )
    {
        auto mode = roPtr->getMesh()->pickingRenderMode();
        if ( mode != Data::Displayable::NO_PICKING )
            renderQueuePicking[size_t( mode )].push_back( roPtr );
    }
}

void Renderer::splitRenderQueuesForPicking( const Data::ViewingParameters& /*renderData*/ ) {
    splitRQ( m_fancyRenderObjects, m_fancyRenderObjectsPicking );
    splitRQ( m_debugRenderObjects, m_debugRenderObjectsPicking );
    splitRQ( m_uiRenderObjects, m_uiRenderObjectsPicking );
    splitRQ( m_xrayRenderObjects, m_xrayRenderObjectsPicking );
}

// subroutine to Renderer::doPicking()
void Renderer::renderForPicking(
    const Data::ViewingParameters& renderData,
    const std::array<const Data::ShaderProgram*, 4>& pickingShaders,
    const std::array<std::vector<RenderObjectPtr>, 4>& renderQueuePicking ) {
    for ( uint i = 0; i < pickingShaders.size(); ++i )
    {
        pickingShaders[i]->bind();
        pickingShaders[i]->setUniform( "transform.proj", renderData.projMatrix );
        pickingShaders[i]->setUniform( "transform.view", renderData.viewMatrix );
        for ( const auto& ro : renderQueuePicking[i] )
        {
            if ( ro->isVisible() && ro->isPickable() )
            {
                pickingShaders[i]->setUniform( "objectId", ro->getIndex().getValue() );
                Core::Matrix4 M = ro->getTransformAsMatrix();
                Core::Matrix4 N = M.inverse().transpose();
                pickingShaders[i]->setUniform( "transform.model", M );
                pickingShaders[i]->setUniform( "transform.worldNormal", N );

                // render
                ro->getMesh()->render( pickingShaders[i] );
            }
        }
    }
}

void Renderer::doPicking( const Data::ViewingParameters& renderData ) {
    m_pickingResults.reserve( m_pickingQueries.size() );

    m_pickingFbo->bind();
    preparePicking( renderData );

    // Now read the Picking Texture to address the Picking Requests.
    GL_ASSERT( glReadBuffer( GL_COLOR_ATTACHMENT0 ) );

    int pick[4];
    for ( const PickingQuery& query : m_pickingQueries )
    {
        PickingResult result;
        // fill picking result according to picking mode
        if ( query.m_mode < C_VERTEX )
        {
            // skip query if out of window (can occur when picking while moving outside)
            if ( query.m_screenCoords.x() < 0 || query.m_screenCoords.x() > m_width - 1 ||
                 query.m_screenCoords.y() < 0 || query.m_screenCoords.y() > m_height - 1 )
            {
                // this qurey has an empty result
                m_pickingResults.push_back( {} );
                continue;
            }
            GL_ASSERT( glReadPixels( query.m_screenCoords.x(),
                                     query.m_screenCoords.y(),
                                     1,
                                     1,
                                     GL_RGBA_INTEGER,
                                     GL_INT,
                                     pick ) );
            result.setRoIdx( pick[0] ); // RO idx
            result.addIndex( { pick[2], pick[1], pick[3] } );
        }
        else
        {
            // select the results for the RO with the most representatives
            // (or first to come if same amount)
            std::map<int, PickingResult> resultPerRO;
            for ( auto i = -m_brushRadius; i <= m_brushRadius; i += 3 )
            {
                auto h = std::round( std::sqrt( m_brushRadius * m_brushRadius - i * i ) );
                for ( auto j = -h; j <= +h; j += 3 )
                {
                    const int x = query.m_screenCoords.x() + i;
                    const int y = query.m_screenCoords.y() - j;
                    // skip query if out of window (can occur when picking while moving outside)
                    if ( x < 0 || x > int( m_width ) - 1 || y < 0 || y > int( m_height ) - 1 )
                    { continue; }
                    GL_ASSERT( glReadPixels( x, y, 1, 1, GL_RGBA_INTEGER, GL_INT, pick ) );
                    resultPerRO[pick[0]].setRoIdx( pick[0] );
                    resultPerRO[pick[0]].addIndex( { pick[2], pick[1], pick[3] } );
                }
            }

            auto itr = std::max_element(
                resultPerRO.begin(),
                resultPerRO.end(),
                []( const std::map<int, PickingResult>::value_type& a,
                    const std::map<int, PickingResult>::value_type& b ) -> bool {
                    return a.second.getIndices().size() < b.second.getIndices().size();
                } );
            result = itr->second;
        }
        result.setMode( query.m_mode );
        m_pickingResults.push_back( result );
    }

    m_pickingFbo->unbind();
}
void Renderer::preparePicking( const Data::ViewingParameters& renderData ) {

    GL_ASSERT( glDepthMask( GL_TRUE ) );
    GL_ASSERT( glColorMask( 1, 1, 1, 1 ) );
    GL_ASSERT( glDrawBuffers( 1, buffers ) );

    float clearDepth = 1.0;
    int clearColor[] = { -1, -1, -1, -1 };

    GL_ASSERT( glClearBufferiv( GL_COLOR, 0, clearColor ) );
    GL_ASSERT( glClearBufferfv( GL_DEPTH, 0, &clearDepth ) );

    splitRenderQueuesForPicking( renderData );

    // First draw Geometry Objects
    GL_ASSERT( glEnable( GL_DEPTH_TEST ) );
    GL_ASSERT( glDepthFunc( GL_LESS ) );

    renderForPicking( renderData, m_pickingShaders, m_fancyRenderObjectsPicking );

    if ( m_drawDebug )
    {
        // Then draw debug objects
        GL_ASSERT( glClearBufferfv( GL_DEPTH, 0, &clearDepth ) );
        renderForPicking( renderData, m_pickingShaders, m_debugRenderObjectsPicking );

        // Then draw xrayed objects on top of normal objects
        GL_ASSERT( glClearBufferfv( GL_DEPTH, 0, &clearDepth ) );
        renderForPicking( renderData, m_pickingShaders, m_xrayRenderObjectsPicking );
    }

    // Finally draw ui stuff on top of everything
    // these have a different way to compute the transform matrices
    // FIXME (florian): find a way to use renderForPicking()!
    GL_ASSERT( glClearBufferfv( GL_DEPTH, 0, &clearDepth ) );
    for ( uint i = 0; i < m_pickingShaders.size(); ++i )
    {
        m_pickingShaders[i]->bind();
        m_pickingShaders[i]->setUniform( "transform.proj", renderData.projMatrix );
        m_pickingShaders[i]->setUniform( "transform.view", renderData.viewMatrix );

        for ( const auto& ro : m_uiRenderObjectsPicking[i] )
        {
            if ( ro->isVisible() && ro->isPickable() )
            {
                m_pickingShaders[i]->setUniform( "objectId", ro->getIndex().getValue() );

                Core::Matrix4 M  = ro->getTransformAsMatrix();
                Core::Matrix4 MV = renderData.viewMatrix * M;
                Scalar d         = MV.block<3, 1>( 0, 3 ).norm();

                Core::Matrix4 S = Core::Matrix4::Identity();
                S( 0, 0 ) = S( 1, 1 ) = S( 2, 2 ) = d;

                M               = M * S;
                Core::Matrix4 N = M.inverse().transpose();

                m_pickingShaders[i]->setUniform( "transform.model", M );
                m_pickingShaders[i]->setUniform( "transform.worldNormal", N );

                // render
                ro->getMesh()->render( m_pickingShaders[i] );
            }
        }
    }
}

void Renderer::restoreExternalFBOInternal() {
    RadiumEngine::getInstance()->popFboAndViewport();
}

void Renderer::drawScreenInternal() {

    restoreExternalFBOInternal();
    // Display the final screen
    {
        GL_ASSERT( glDepthFunc( GL_ALWAYS ) );

        auto shader = ( m_displayedTexture->getParameters().type == GL_INT ||
                        m_displayedTexture->getParameters().type == GL_UNSIGNED_INT )
                          ? ( m_displayedTexture->getParameters().format == GL_DEPTH_COMPONENT
                                  ? m_shaderProgramManager->getShaderProgram( "DisplayDepthBuffer" )
                                  : m_shaderProgramManager->getShaderProgram( "DrawScreenI" ) )
                          : m_shaderProgramManager->getShaderProgram( "DrawScreen" );
        shader->bind();
        shader->setUniform( "screenTexture", m_displayedTexture, 0 );
        m_quadMesh->render( shader );

        GL_ASSERT( glDepthFunc( GL_LESS ) );
    }
    // draw brush circle if enabled
    if ( m_brushRadius > 0 )
    {
        GL_ASSERT( glDisable( GL_BLEND ) );
        GL_ASSERT( glDisable( GL_DEPTH_TEST ) );
        auto shader = m_shaderProgramManager->getShaderProgram( "CircleBrush" );
        shader->bind();
        shader->setUniform( "mousePosition", m_mousePosition );
        shader->setUniform( "brushRadius", m_brushRadius );
        shader->setUniform( "dim", Core::Vector2( m_width, m_height ) );
        m_quadMesh->render( shader );
        GL_ASSERT( glEnable( GL_DEPTH_TEST ) );
        GL_ASSERT( glEnable( GL_BLEND ) );
    }
}

void Renderer::notifyRenderObjectsRenderingInternal() {
    for ( auto& ro : m_fancyRenderObjects )
    { ro->hasBeenRenderedOnce(); }

    for ( auto& ro : m_debugRenderObjects )
    { ro->hasBeenRenderedOnce(); }

    for ( auto& ro : m_xrayRenderObjects )
    { ro->hasBeenRenderedOnce(); }

    for ( auto& ro : m_uiRenderObjects )
    { ro->hasBeenRenderedOnce(); }
}

void Renderer::resize( uint w, uint h ) {
    // never init zero sized texture/fbo since fbo do not consider them as complete
    // not initilialized ? init texture
    // then only init if size is different than current
    if ( ( w != 0 && h != 0 ) && ( !m_initialized || ( w != m_width || h != m_height ) ) )
    {
        m_width  = w;
        m_height = h;
        m_depthTexture->resize( m_width, m_height );
        m_pickingTexture->resize( m_width, m_height );
        m_fancyTexture->resize( m_width, m_height );

        m_pickingFbo->bind();
        m_pickingFbo->attachTexture( GL_DEPTH_ATTACHMENT, m_depthTexture->texture() );
        m_pickingFbo->attachTexture( GL_COLOR_ATTACHMENT0, m_pickingTexture->texture() );
        if ( m_pickingFbo->checkStatus() != GL_FRAMEBUFFER_COMPLETE )
        {
            LOG( logERROR ) << "File " << __FILE__ << "(" << __LINE__ << ") Picking FBO Error "
                            << m_pickingFbo->checkStatus();
        }
        m_pickingFbo->unbind();
        resizeInternal();
        GL_CHECK_ERROR;
        m_initialized = true;
    }
}

void Renderer::displayTexture( const std::string& texName ) {
    if ( m_secondaryTextures.find( texName ) != m_secondaryTextures.end() )
    { m_displayedTexture = m_secondaryTextures[texName]; }
    else
    { m_displayedTexture = m_fancyTexture.get(); }
}

std::vector<std::string> Renderer::getAvailableTextures() const {
    std::vector<std::string> ret;
    ret.emplace_back( "Final image" );
    std::transform( m_secondaryTextures.begin(),
                    m_secondaryTextures.end(),
                    std::back_inserter( ret ),
                    []( const std::pair<std::string, Data::Texture*> tex ) { return tex.first; } );
    return ret;
}

void Renderer::reloadShaders() {
    m_shaderProgramManager->reloadAllShaderPrograms();
}

std::unique_ptr<uchar[]> Renderer::grabFrame( size_t& w, size_t& h ) const {
    Data::Texture* tex = getDisplayTexture();
    tex->bind();

    // Get a buffer to store the pixels of the OpenGL texture (in float format)
    auto pixels = std::unique_ptr<float[]>( new float[tex->width() * tex->height() * 4] );

    // Grab the texture data
    GL_ASSERT( glGetTexImage( GL_TEXTURE_2D, 0, GL_RGBA, GL_SCALAR, pixels.get() ) );

    // Now we must convert the floats to RGB while flipping the image updisde down.
    auto writtenPixels = std::unique_ptr<uchar[]>( new uchar[tex->width() * tex->height() * 4] );
    for ( uint j = 0; j < tex->height(); ++j )
    {
        for ( uint i = 0; i < tex->width(); ++i )
        {
            auto in = 4 * ( j * tex->width() + i ); // Index in the texture buffer
            auto ou = 4 * ( ( tex->height() - 1 - j ) * tex->width() +
                            i ); // Index in the final image (note the j flipping).

            writtenPixels[ou + 0] =
                (uchar)std::clamp( float( pixels[in + 0] * 255.f ), float( 0 ), float( 255 ) );
            writtenPixels[ou + 1] =
                (uchar)std::clamp( float( pixels[in + 1] * 255.f ), float( 0 ), float( 255 ) );
            writtenPixels[ou + 2] =
                (uchar)std::clamp( float( pixels[in + 2] * 255.f ), float( 0 ), float( 255 ) );
            writtenPixels[ou + 3] =
                (uchar)std::clamp( float( pixels[in + 3] * 255.f ), float( 0 ), float( 255 ) );
        }
    }
    w = tex->width();
    h = tex->height();
    return writtenPixels;
}

void Renderer::addLight( const Scene::Light* light ) {
    for ( auto m : m_lightmanagers )
        m->addLight( light );
}

bool Renderer::hasLight() const {
    int n = 0;
    for ( auto m : m_lightmanagers )
        n += m->count();
    return n != 0;
}

int Renderer::buildAllRenderTechniques() const {
    std::vector<RenderObjectPtr> renderObjects;
    m_renderObjectManager->getRenderObjectsByType( renderObjects, RenderObjectType::Geometry );
    if ( renderObjects.size() > 0 )
    {
        for ( auto& ro : renderObjects )
        { buildRenderTechnique( ro.get() ); }
    }
    return 0;
}

} // namespace Rendering
} // namespace Engine
} // namespace Ra
