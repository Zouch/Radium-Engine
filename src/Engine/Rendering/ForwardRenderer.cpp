#include <Engine/Rendering/ForwardRenderer.hpp>

#include <Core/Containers/MakeShared.hpp>
#include <Core/Geometry/TopologicalMesh.hpp>
#include <Core/Utils/Color.hpp>
#include <Core/Utils/Log.hpp>

#include <Engine/Data/LambertianMaterial.hpp>
#include <Engine/Data/Material.hpp>
#include <Engine/Data/RenderParameters.hpp>
#include <Engine/Data/ShaderProgramManager.hpp>
#include <Engine/Data/Texture.hpp>
#include <Engine/Data/ViewingParameters.hpp>
#include <Engine/OpenGL.hpp>
#include <Engine/Rendering/DebugRender.hpp>
#include <Engine/Rendering/RenderObject.hpp>
#include <Engine/Scene/DefaultLightManager.hpp>
#include <Engine/Scene/Light.hpp>
#include <globjects/Framebuffer.h>

/* Test Point cloud parameter provider */
#include <Core/RaCore.hpp>
#include <Engine/Data/ShaderProgram.hpp>
#include <Engine/Scene/GeometryComponent.hpp>

#include <Engine/Scene/SystemDisplay.hpp>

#include <map>

#include <globjects/Texture.h>

namespace Ra {
using namespace Core;
using namespace Core::Utils; // log

namespace Engine {
namespace Rendering {
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

ForwardRenderer::ForwardRenderer() : Renderer() {}

ForwardRenderer::~ForwardRenderer() = default;

void ForwardRenderer::initializeInternal() {
    initShaders();
    initBuffers();

    auto lightManager = new Scene::DefaultLightManager();
    Ra::Engine::RadiumEngine::getInstance()->registerSystem( "DefaultLightManager", lightManager );
    m_lightmanagers.push_back( lightManager );

    if ( !DebugRender::getInstance() ) {
        DebugRender::createInstance();
        DebugRender::getInstance()->initialize();
    }
}

void ForwardRenderer::initShaders() {
    /// For internal resources management in a filesystem
    auto resourcesRootDir { RadiumEngine::getInstance()->getResourcesDir() };
    m_shaderProgramManager->addShaderProgram(
        { { "Hdr2Ldr" },
          resourcesRootDir + "Shaders/2DShaders/Basic2D.vert.glsl",
          resourcesRootDir + "Shaders/2DShaders/Hdr2Ldr.frag.glsl" } );
    m_shaderProgramManager->addShaderProgram(
        { { "ComposeOIT" },
          resourcesRootDir + "Shaders/2DShaders/Basic2D.vert.glsl",
          resourcesRootDir + "Shaders/2DShaders/ComposeOIT.frag.glsl" } );

    Data::ShaderConfiguration wireframe { { "Wireframe" },
                                          resourcesRootDir + "Shaders/Lines/Wireframe.vert.glsl",
                                          resourcesRootDir + "Shaders/Lines/Wireframe.frag.glsl" };
    wireframe.addShader( Data::ShaderType::ShaderType_GEOMETRY,
                         resourcesRootDir + "Shaders/Lines/Wireframe.geom.glsl" );
    m_shaderProgramManager->addShaderProgram( wireframe );
}

void ForwardRenderer::initBuffers() {
    m_fbo            = std::make_unique<globjects::Framebuffer>();
    m_oitFbo         = std::make_unique<globjects::Framebuffer>();
    m_postprocessFbo = std::make_unique<globjects::Framebuffer>();
    m_uiXrayFbo      = std::make_unique<globjects::Framebuffer>();
    m_volumeFbo      = std::make_unique<globjects::Framebuffer>();
    // Forward renderer internal textures texture

    Data::TextureParameters texparams;
    texparams.width  = m_width;
    texparams.height = m_height;
    texparams.target = GL_TEXTURE_2D;

    // Depth texture
    texparams.minFilter                = GL_NEAREST;
    texparams.magFilter                = GL_NEAREST;
    texparams.internalFormat           = GL_DEPTH_COMPONENT24;
    texparams.format                   = GL_DEPTH_COMPONENT;
    texparams.type                     = GL_UNSIGNED_INT;
    texparams.name                     = "Depth (fw renderer)";
    m_textures[RendererTextures_Depth] = std::make_unique<Data::Texture>( texparams );

    // Color texture
    texparams.internalFormat = GL_RGBA32F;
    texparams.format         = GL_RGBA;
    texparams.type           = GL_SCALAR;
    texparams.minFilter      = GL_LINEAR;
    texparams.magFilter      = GL_LINEAR;

    texparams.name                   = "HDR";
    m_textures[RendererTextures_HDR] = std::make_unique<Data::Texture>( texparams );

    texparams.name                      = "Normal";
    m_textures[RendererTextures_Normal] = std::make_unique<Data::Texture>( texparams );

    texparams.name                       = "Diffuse";
    m_textures[RendererTextures_Diffuse] = std::make_unique<Data::Texture>( texparams );

    texparams.name                        = "Specular";
    m_textures[RendererTextures_Specular] = std::make_unique<Data::Texture>( texparams );

    texparams.name                        = "OIT Accum";
    m_textures[RendererTextures_OITAccum] = std::make_unique<Data::Texture>( texparams );

    texparams.name                            = "OIT Revealage";
    m_textures[RendererTextures_OITRevealage] = std::make_unique<Data::Texture>( texparams );

    texparams.name                      = "Volume";
    m_textures[RendererTextures_Volume] = std::make_unique<Data::Texture>( texparams );

    m_secondaryTextures["Depth (fw)"]       = m_textures[RendererTextures_Depth].get();
    m_secondaryTextures["HDR Texture"]      = m_textures[RendererTextures_HDR].get();
    m_secondaryTextures["Normal Texture"]   = m_textures[RendererTextures_Normal].get();
    m_secondaryTextures["Diffuse Texture"]  = m_textures[RendererTextures_Diffuse].get();
    m_secondaryTextures["Specular Texture"] = m_textures[RendererTextures_Specular].get();
    m_secondaryTextures["OIT Accum"]        = m_textures[RendererTextures_OITAccum].get();
    m_secondaryTextures["OIT Revealage"]    = m_textures[RendererTextures_OITRevealage].get();

    // Volume texture is not exposed ...
    m_secondaryTextures["Volume"] = m_textures[RendererTextures_Volume].get();
}

void ForwardRenderer::updateStepInternal( const Data::ViewingParameters& renderData ) {
    CORE_UNUSED( renderData );

    m_transparentRenderObjects.clear();
    m_volumetricRenderObjects.clear();
    for ( auto it = m_fancyRenderObjects.begin(); it != m_fancyRenderObjects.end(); ) {
        if ( ( *it )->isTransparent() ) {
            m_transparentRenderObjects.push_back( *it );
            it = m_fancyRenderObjects.erase( it );
        }
        else {
            auto material = ( *it )->getMaterial();
            if ( material &&
                 material->getMaterialAspect() == Data::Material::MaterialAspect::MAT_DENSITY ) {
                m_volumetricRenderObjects.push_back( *it );
                it = m_fancyRenderObjects.erase( it );
            }
            else {
                ++it;
            }
        }
    }
    m_fancyTransparentCount = m_transparentRenderObjects.size();
    m_fancyVolumetricCount  = m_volumetricRenderObjects.size();

    // simple hack to clean wireframes ...
    if ( m_fancyRenderObjects.size() < m_wireframes.size() ) { m_wireframes.clear(); }
}

template <typename IndexContainerType>
void computeIndices( Core::Geometry::LineMesh::IndexContainerType& indices,
                     IndexContainerType& other ) {

    for ( const auto& index : other ) {
        auto s = index.size();
        for ( unsigned int i = 0; i < s; ++i ) {
            int i1 = index[i];
            int i2 = index[( i + 1 ) % s];
            if ( i1 > i2 ) std::swap( i1, i2 );
            indices.emplace_back( i1, i2 );
        }
    }

    std::sort( indices.begin(),
               indices.end(),
               []( const Core::Geometry::LineMesh::IndexType& a,
                   const Core::Geometry::LineMesh::IndexType& b ) {
                   return a[0] < b[0] || ( a[0] == b[0] && a[1] < b[1] );
               } );
    indices.erase( std::unique( indices.begin(), indices.end() ), indices.end() );
}

// store LineMesh and Core, define the observer functor to update data one core update for wireframe
// linemesh
template <typename CoreGeometry>
class VerticesUpdater
{
  public:
    VerticesUpdater( std::shared_ptr<Data::LineMesh> disp, CoreGeometry& core ) :
        m_disp { disp }, m_core { core } {};

    void operator()() { m_disp->getCoreGeometry().setVertices( m_core.vertices() ); }
    std::shared_ptr<Data::LineMesh> m_disp;
    CoreGeometry& m_core;
};

template <typename CoreGeometry>
class IndicesUpdater
{
  public:
    IndicesUpdater( std::shared_ptr<Data::LineMesh> disp, CoreGeometry& core ) :
        m_disp { disp }, m_core { core } {};

    void operator()() {
        auto lineIndices = m_disp->getCoreGeometry().getIndicesWithLock();
        computeIndices( lineIndices, m_core.getIndices() );
        m_disp->getCoreGeometry().indicesUnlock();
    }
    std::shared_ptr<Data::LineMesh> m_disp;
    CoreGeometry& m_core;
};

// create a linemesh to draw wireframe given a core mesh
template <typename CoreGeometry>
void setupLineMesh( std::shared_ptr<Data::LineMesh>& disp, CoreGeometry& core ) {

    Core::Geometry::LineMesh lines;
    Core::Geometry::LineMesh::IndexContainerType indices;

    lines.setVertices( core.vertices() );
    computeIndices( indices, core.getIndices() );
    if ( indices.size() > 0 ) {
        lines.setIndices( std::move( indices ) );
        disp =
            Ra::Core::make_shared<Data::LineMesh>( std::string( "wireframe" ), std::move( lines ) );
        disp->updateGL();

        // add observer
        auto handle = core.template getAttribHandle<typename CoreGeometry::Point>(
            Data::Mesh::getAttribName( Data::Mesh::VERTEX_POSITION ) );
        core.vertexAttribs().getAttrib( handle ).attach( VerticesUpdater( disp, core ) );

        core.attach( IndicesUpdater( disp, core ) );
    }
    else {
        disp.reset();
    }
}

void ForwardRenderer::renderInternal( const Data::ViewingParameters& renderData ) {

    m_fbo->bind();

    GL_ASSERT( glEnable( GL_DEPTH_TEST ) );
    GL_ASSERT( glDepthMask( GL_TRUE ) );
    GL_ASSERT( glColorMask( 1, 1, 1, 1 ) );

    GL_ASSERT( glDrawBuffers( 4, buffers ) );
    if ( m_wireframe ) {
        ///\todo make offset relative to wireframe line width
        glEnable( GL_POLYGON_OFFSET_FILL );
        glPolygonOffset( 1.f, 3.f );
        //    GL_ASSERT( glDepthRange( 0.001, 1.0 ) );
    }
    else {
        glDisable( GL_POLYGON_OFFSET_FILL );
    }
    static const auto clearZeros = Core::Utils::Color::Black().cast<GL_SCALAR_PLAIN>().eval();
    static const auto clearOnes  = Core::Utils::Color::White().cast<GL_SCALAR_PLAIN>().eval();
    static const float clearDepth { 1.0f };

    ///\todo use globjects.
    auto bgColor = getBackgroundColor().cast<GL_SCALAR_PLAIN>().eval();
    GL_ASSERT( glClearBufferfv( GL_COLOR, 0, bgColor.data() ) );    // Clear color
    GL_ASSERT( glClearBufferfv( GL_COLOR, 1, clearZeros.data() ) ); // Clear normals
    GL_ASSERT( glClearBufferfv( GL_COLOR, 2, clearZeros.data() ) ); // Clear diffuse
    GL_ASSERT( glClearBufferfv( GL_COLOR, 3, clearZeros.data() ) ); // Clear specular
    GL_ASSERT( glClearBufferfv( GL_DEPTH, 0, &clearDepth ) );       // Clear depth

    // Z prepass
    GL_ASSERT( glDepthFunc( GL_LESS ) );
    GL_ASSERT( glDisable( GL_BLEND ) );
    GL_ASSERT( glPointSize( 3.f ) );

    // Set in RenderParam the configuration about ambiant lighting (instead of hard constant
    // direclty in shaders)
    Data::RenderParameters zprepassParams;
    for ( const auto& ro : m_fancyRenderObjects ) {
        ro->render( zprepassParams, renderData, DefaultRenderingPasses::Z_PREPASS );
    }
    // Transparent objects are rendered in the Z-prepass, but only their fully opaque fragments
    // (if any) might influence the z-buffer.
    // Rendering transparent objects assuming that they
    // discard all their non-opaque fragments
    for ( const auto& ro : m_transparentRenderObjects ) {
        ro->render( zprepassParams, renderData, DefaultRenderingPasses::Z_PREPASS );
    }
    // Volumetric objects are not rendered in the Z-prepass

    // Opaque Lighting pass
    GL_ASSERT( glDepthFunc( GL_LEQUAL ) );
    GL_ASSERT( glDepthMask( GL_FALSE ) );

    GL_ASSERT( glEnable( GL_BLEND ) );
    GL_ASSERT( glBlendFunc( GL_ONE, GL_ONE ) );

    GL_ASSERT( glDrawBuffers( 1, buffers ) ); // Draw color texture

    // Radium V2 : this render loop might be greatly improved by inverting light and objects
    // loop.
    // Make shaders bounded only once, minimize full stats-changes, ...
    if ( m_lightmanagers[0]->count() > 0 ) {
        // for ( const auto& l : m_lights )
        for ( size_t i = 0; i < m_lightmanagers[0]->count(); ++i ) {
            const auto l = m_lightmanagers[0]->getLight( i );
            Data::RenderParameters lightingpassParams;
            l->getRenderParameters( lightingpassParams );

            for ( const auto& ro : m_fancyRenderObjects ) {
                ro->render(
                    lightingpassParams, renderData, DefaultRenderingPasses::LIGHTING_OPAQUE );
            }
            // Rendering transparent objects assuming that they discard all their non-opaque
            // fragments
            for ( const auto& ro : m_transparentRenderObjects ) {
                ro->render(
                    lightingpassParams, renderData, DefaultRenderingPasses::LIGHTING_OPAQUE );
            }
        }
    }
    else {
        LOG( logINFO ) << "Opaque : no light sources, unable to render";
    }

    // Transparency (blending) pass
    if ( !m_transparentRenderObjects.empty() ) {
        m_fbo->unbind();

        m_oitFbo->bind();

        GL_ASSERT( glDrawBuffers( 2, buffers ) );
        GL_ASSERT( glClearBufferfv( GL_COLOR, 0, clearZeros.data() ) );
        GL_ASSERT( glClearBufferfv( GL_COLOR, 1, clearOnes.data() ) );

        GL_ASSERT( glDepthFunc( GL_LESS ) );
        GL_ASSERT( glEnable( GL_BLEND ) );

        GL_ASSERT( glBlendEquation( GL_FUNC_ADD ) );
        GL_ASSERT( glBlendFunci( 0, GL_ONE, GL_ONE ) );
        GL_ASSERT( glBlendFunci( 1, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA ) );

        if ( m_lightmanagers[0]->count() > 0 ) {
            // for ( const auto& l : m_lights )
            for ( size_t i = 0; i < m_lightmanagers[0]->count(); ++i ) {
                const auto l = m_lightmanagers[0]->getLight( i );
                Data::RenderParameters trasparencypassParams;
                l->getRenderParameters( trasparencypassParams );

                for ( const auto& ro : m_transparentRenderObjects ) {
                    ro->render( trasparencypassParams,
                                renderData,
                                DefaultRenderingPasses::LIGHTING_TRANSPARENT );
                }
            }
        }
        else {
            LOG( logINFO ) << "Transparent : no light sources, unable to render";
        }

        m_oitFbo->unbind();

        m_fbo->bind();
        GL_ASSERT( glDrawBuffers( 1, buffers ) );
        GL_ASSERT( glDisable( GL_DEPTH_TEST ) );
        GL_ASSERT( glBlendFunc( GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA ) );
        {
            auto shader = m_shaderProgramManager->getShaderProgram( "ComposeOIT" );
            shader->bind();
            shader->setUniform( "u_OITSumColor", m_textures[RendererTextures_OITAccum].get(), 0 );
            shader->setUniform(
                "u_OITSumWeight", m_textures[RendererTextures_OITRevealage].get(), 1 );

            m_quadMesh->render( shader );
        }
        GL_ASSERT( glEnable( GL_DEPTH_TEST ) );
    }
    // Volumetric pass
    // Z-test is enabled but z-write must be disable to allow access to the z-buffer in the
    // shader.
    // This pass render in its own FBO and copy the result to the main colortexture
    if ( !m_volumetricRenderObjects.empty() ) {
        m_volumeFbo->bind();
        GL_ASSERT( glDrawBuffers( 1, buffers ) );
        auto alpha = Core::Utils::Color::Alpha().cast<GL_SCALAR_PLAIN>().eval();
        GL_ASSERT( glClearBufferfv( GL_COLOR, 0, alpha.data() ) );
        GL_ASSERT( glDisable( GL_BLEND ) );
        // for ( const auto& l : m_lights )
        for ( size_t i = 0; i < m_lightmanagers[0]->count(); ++i ) {
            const auto l = m_lightmanagers[0]->getLight( i );
            Data::RenderParameters passParams;
            l->getRenderParameters( passParams );
            passParams.addParameter( "imageColor", m_textures[RendererTextures_HDR].get() );
            passParams.addParameter( "imageDepth", m_textures[RendererTextures_Depth].get() );

            for ( const auto& ro : m_volumetricRenderObjects ) {
                ro->render( passParams, renderData, DefaultRenderingPasses::LIGHTING_VOLUMETRIC );
            }
        }
        m_volumeFbo->unbind();

        m_fbo->bind();
        GL_ASSERT( glDrawBuffers( 1, buffers ) );
        GL_ASSERT( glDisable( GL_DEPTH_TEST ) );
        GL_ASSERT( glEnable( GL_BLEND ) );
        GL_ASSERT( glBlendFunc( GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA ) );
        {
            auto shader = m_shaderProgramManager->getShaderProgram( "ComposeVolume" );
            shader->bind();
            shader->setUniform( "volumeImage", m_textures[RendererTextures_Volume].get(), 0 );
            m_quadMesh->render( shader );
        }
        GL_ASSERT( glEnable( GL_DEPTH_TEST ) );
    }

    if ( m_wireframe ) {

        m_fbo->bind();

        glDisable( GL_POLYGON_OFFSET_FILL );
        GL_ASSERT( glDepthFunc( GL_LEQUAL ) );
        GL_ASSERT( glEnable( GL_BLEND ) );
        glBlendEquationSeparate( GL_FUNC_ADD, GL_FUNC_ADD );
        glBlendFuncSeparate( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO );
        GL_ASSERT( glDrawBuffers( 1, buffers ) ); // Draw color texture

        auto drawWireframe = [this, &renderData]( const auto& ro ) {
            std::shared_ptr<Data::Displayable> wro;

            WireMap::iterator it = m_wireframes.find( ro.get() );
            if ( it == m_wireframes.end() ) {
                std::shared_ptr<Data::LineMesh> disp;

                using trimesh = Ra::Engine::Data::IndexedGeometry<Ra::Core::Geometry::TriangleMesh>;
                using polymesh = Ra::Engine::Data::IndexedGeometry<Ra::Core::Geometry::PolyMesh>;

                auto displayable = ro->getMesh();
                auto tm          = std::dynamic_pointer_cast<trimesh>( displayable );
                auto tp          = std::dynamic_pointer_cast<polymesh>( displayable );

                auto processLineMesh = []( auto cm, std::shared_ptr<Data::LineMesh>& lm ) {
                    if ( cm->getRenderMode() ==
                         Data::AttribArrayDisplayable::MeshRenderMode::RM_TRIANGLES ) {
                        setupLineMesh( lm, cm->getCoreGeometry() );
                    }
                };
                if ( tm ) { processLineMesh( tm, disp ); }
                if ( tp ) { processLineMesh( tp, disp ); }

                m_wireframes[ro.get()] = disp;
                wro                    = disp;
            }
            else {
                wro = it->second;
            }

            const Data::ShaderProgram* shader =
                m_shaderProgramManager->getShaderProgram( "Wireframe" );

            if ( shader && wro ) {
                shader->bind();
                if ( ro->isVisible() ) {
                    wro->updateGL();

                    Core::Matrix4 modelMatrix = ro->getTransformAsMatrix();
                    shader->setUniform( "transform.proj", renderData.projMatrix );
                    shader->setUniform( "transform.view", renderData.viewMatrix );
                    shader->setUniform( "transform.model", modelMatrix );
                    shader->setUniform( "viewport", Core::Vector2 { m_width, m_height } );
                    wro->render( shader );

                    GL_CHECK_ERROR;
                }
            }
        };

        for ( const auto& ro : m_fancyRenderObjects ) {
            drawWireframe( ro );
        }
        for ( const auto& ro : m_transparentRenderObjects ) {
            drawWireframe( ro );
        }
    }

    // Restore state
    GL_ASSERT( glDepthFunc( GL_LESS ) );
    GL_ASSERT( glDisable( GL_BLEND ) );
    m_fbo->unbind();
}

// Draw debug stuff, do not overwrite depth map but do depth testing
void ForwardRenderer::debugInternal( const Data::ViewingParameters& renderData ) {
    if ( m_drawDebug ) {
        m_postprocessFbo->bind();
        GL_ASSERT( glDisable( GL_BLEND ) );
        GL_ASSERT( glEnable( GL_DEPTH_TEST ) );
        GL_ASSERT( glDepthMask( GL_FALSE ) );
        GL_ASSERT( glDepthFunc( GL_LESS ) );

        glDrawBuffers( 1, buffers );

        for ( const auto& ro : m_debugRenderObjects ) {
            ro->render( Data::RenderParameters {}, renderData );
        }

        DebugRender::getInstance()->render( renderData.viewMatrix, renderData.projMatrix );

        m_postprocessFbo->unbind();

        m_uiXrayFbo->bind();
        // Draw X rayed objects always on top of normal objects
        GL_ASSERT( glDepthMask( GL_TRUE ) );
        GL_ASSERT( glClear( GL_DEPTH_BUFFER_BIT ) );
        Data::RenderParameters xrayLightParams;
        xrayLightParams.addParameter( "light.color", Ra::Core::Utils::Color::Grey( 5.0 ) );
        xrayLightParams.addParameter( "light.type", Scene::Light::LightType::DIRECTIONAL );
        xrayLightParams.addParameter( "light.directional.direction", Core::Vector3( 0, -1, 0 ) );
        for ( const auto& ro : m_xrayRenderObjects ) {
            if ( ro->isVisible() ) { ro->render( xrayLightParams, renderData ); }
        }
        m_uiXrayFbo->unbind();
    }
}

// Draw UI stuff, always drawn on top of everything else + clear ZMask
void ForwardRenderer::uiInternal( const Data::ViewingParameters& renderData ) {

    m_uiXrayFbo->bind();
    glDrawBuffers( 1, buffers );
    // Enable z-test
    GL_ASSERT( glDepthMask( GL_TRUE ) );
    GL_ASSERT( glEnable( GL_DEPTH_TEST ) );
    GL_ASSERT( glDepthFunc( GL_LESS ) );
    GL_ASSERT( glClear( GL_DEPTH_BUFFER_BIT ) );
    for ( const auto& ro : m_uiRenderObjects ) {
        if ( ro->isVisible() ) {
            auto shader = ro->getRenderTechnique()->getShader();

            // bind data
            shader->bind();

            Core::Matrix4 M  = ro->getTransformAsMatrix();
            Core::Matrix4 MV = renderData.viewMatrix * M;
            Core::Vector3 V  = MV.block<3, 1>( 0, 3 );
            Scalar d         = V.norm();

            Core::Matrix4 S    = Core::Matrix4::Identity();
            S.coeffRef( 0, 0 ) = S.coeffRef( 1, 1 ) = S.coeffRef( 2, 2 ) = d;

            M = M * S;

            shader->setUniform( "transform.proj", renderData.projMatrix );
            shader->setUniform( "transform.view", renderData.viewMatrix );
            shader->setUniform( "transform.model", M );

            auto shaderParameter = ro->getRenderTechnique()->getParametersProvider();
            if ( shaderParameter != nullptr ) shaderParameter->getParameters().bind( shader );

            // render
            ro->getMesh()->render( shader );
        }
    }
    m_uiXrayFbo->unbind();
}

void ForwardRenderer::postProcessInternal( const Data::ViewingParameters& renderData ) {
    CORE_UNUSED( renderData );

    m_postprocessFbo->bind();

    GL_ASSERT( glDrawBuffers( 1, buffers ) );

    GL_ASSERT( glDisable( GL_DEPTH_TEST ) );
    GL_ASSERT( glDepthMask( GL_FALSE ) );

    const Data::ShaderProgram* shader = m_shaderProgramManager->getShaderProgram( "Hdr2Ldr" );
    shader->bind();
    shader->setUniform( "screenTexture", m_textures[RendererTextures_HDR].get(), 0 );
    m_quadMesh->render( shader );

    GL_ASSERT( glDepthMask( GL_TRUE ) );
    GL_ASSERT( glEnable( GL_DEPTH_TEST ) );

    m_postprocessFbo->unbind();
}

void ForwardRenderer::resizeInternal() {
    m_pingPongSize = std::pow( uint( 2 ), uint( std::log2( std::min( m_width, m_height ) ) ) );

    m_textures[RendererTextures_Depth]->resize( m_width, m_height );
    m_textures[RendererTextures_HDR]->resize( m_width, m_height );
    m_textures[RendererTextures_Normal]->resize( m_width, m_height );
    m_textures[RendererTextures_Diffuse]->resize( m_width, m_height );
    m_textures[RendererTextures_Specular]->resize( m_width, m_height );
    m_textures[RendererTextures_OITAccum]->resize( m_width, m_height );
    m_textures[RendererTextures_OITRevealage]->resize( m_width, m_height );
    m_textures[RendererTextures_Volume]->resize( m_width, m_height );

    m_fbo->bind();
    m_fbo->attachTexture( GL_DEPTH_ATTACHMENT, m_textures[RendererTextures_Depth]->texture() );
    m_fbo->attachTexture( GL_COLOR_ATTACHMENT0, m_textures[RendererTextures_HDR]->texture() );
    m_fbo->attachTexture( GL_COLOR_ATTACHMENT1, m_textures[RendererTextures_Normal]->texture() );
    m_fbo->attachTexture( GL_COLOR_ATTACHMENT2, m_textures[RendererTextures_Diffuse]->texture() );
    m_fbo->attachTexture( GL_COLOR_ATTACHMENT3, m_textures[RendererTextures_Specular]->texture() );
    if ( m_fbo->checkStatus() != GL_FRAMEBUFFER_COMPLETE ) {
        LOG( logERROR ) << "FBO Error (ForwardRenderer::m_fbo): " << m_fbo->checkStatus();
    }

    m_volumeFbo->bind();
    m_volumeFbo->attachTexture( GL_DEPTH_ATTACHMENT,
                                m_textures[RendererTextures_Depth]->texture() );
    m_volumeFbo->attachTexture( GL_COLOR_ATTACHMENT0,
                                m_textures[RendererTextures_Volume]->texture() );
    if ( m_volumeFbo->checkStatus() != GL_FRAMEBUFFER_COMPLETE ) {
        LOG( logERROR ) << "FBO Error (ForwardRenderer::m_volumeFbo) : "
                        << m_volumeFbo->checkStatus();
    }

    m_oitFbo->bind();
    m_oitFbo->attachTexture( GL_DEPTH_ATTACHMENT, m_textures[RendererTextures_Depth]->texture() );
    m_oitFbo->attachTexture( GL_COLOR_ATTACHMENT0,
                             m_textures[RendererTextures_OITAccum]->texture() );
    m_oitFbo->attachTexture( GL_COLOR_ATTACHMENT1,
                             m_textures[RendererTextures_OITRevealage]->texture() );
    if ( m_oitFbo->checkStatus() != GL_FRAMEBUFFER_COMPLETE ) {
        LOG( logERROR ) << "FBO Error (ForwardRenderer::m_oitFbo) : " << m_oitFbo->checkStatus();
    }

    m_postprocessFbo->bind();
    m_postprocessFbo->attachTexture( GL_DEPTH_ATTACHMENT,
                                     m_textures[RendererTextures_Depth]->texture() );
    m_postprocessFbo->attachTexture( GL_COLOR_ATTACHMENT0, m_fancyTexture->texture() );
    if ( m_postprocessFbo->checkStatus() != GL_FRAMEBUFFER_COMPLETE ) {
        LOG( logERROR ) << "FBO Error (ForwardRenderer::m_postprocessFbo) : "
                        << m_postprocessFbo->checkStatus();
    }

    // FIXED : when m_postprocessFbo use the RendererTextures_Depth, the depth buffer is erased
    // and is therefore useless for future computation. Do not use this post-process FBO to
    // render eveything else than the scene. Create several FBO with ther own configuration
    // (uncomment Renderer::m_depthTexture->texture() to see the difference.)
    m_uiXrayFbo->bind();
    m_uiXrayFbo->attachTexture( GL_DEPTH_ATTACHMENT, Renderer::m_depthTexture->texture() );
    m_uiXrayFbo->attachTexture( GL_COLOR_ATTACHMENT0, m_fancyTexture->texture() );
    if ( m_uiXrayFbo->checkStatus() != GL_FRAMEBUFFER_COMPLETE ) {
        LOG( logERROR ) << "FBO Error (ForwardRenderer::m_uiXrayFbo) : "
                        << m_uiXrayFbo->checkStatus();
    }
    // finished with fbo, undbind to bind default
    globjects::Framebuffer::unbind();
}

/* Test Point cloud parameter provider */
/*
 * WARNING : this class is here only for testing and experimentation purpose.
 * It will be replace soon by a better management of the way components could add specific
 * properties to a rendertechnique
 * TODO : see PR Draft and gist subShaderBlob
 */
class PointCloudParameterProvider : public Data::ShaderParameterProvider
{
  public:
    PointCloudParameterProvider( std::shared_ptr<Data::Material> mat,
                                 Scene::PointCloudComponent* pointCloud ) :
        ShaderParameterProvider(), m_displayMaterial( mat ), m_component( pointCloud ) {}
    ~PointCloudParameterProvider() override = default;
    void updateGL() override {
        m_displayMaterial->updateGL();
        m_renderParameters = m_displayMaterial->getParameters();
        m_renderParameters.addParameter( "pointCloudSplatRadius", m_component->getSplatSize() );
    }

  private:
    std::shared_ptr<Data::Material> m_displayMaterial;
    Scene::PointCloudComponent* m_component;
};

/*
 * Build renderTechnique for Forward Renderer : this is the default in Radium, so create Default
 * Render Technique
 */
bool ForwardRenderer::buildRenderTechnique( RenderObject* ro ) const {
    auto material = ro->getMaterial();
    if ( !material ) {
        LOG( logWARNING ) << "ForwardRenderer : no material found when building RenderTechnique"
                          << " - adding red Lambertian material";
        auto defMat     = new Data::LambertianMaterial( "ForwardRenderer::Default material" );
        defMat->m_color = Ra::Core::Utils::Color::Red();
        material.reset( defMat );
    }
    auto builder = EngineRenderTechniques::getDefaultTechnique( material->getMaterialName() );
    auto rt      = Core::make_shared<RenderTechnique>();
    // define the technique for rendering this RenderObject (here, using the default from
    // Material name)
    builder.second( *rt, material->isTransparent() );
    // If renderObject is a point cloud,  add geometry shader for splatting
    auto RenderedGeometry = ro->getMesh().get();

    /*
     * WARNING : this way of managing specific geometries is here only for testing and
     * experimentation purpose. It will be replace soon by a better management of the way
     * components could add specific properties to a rendertechnique
     * TODO : see PR Draft and gist subShaderBlob
     */
    if ( RenderedGeometry && RenderedGeometry->getNumFaces() == 0 ) {
        auto pointCloud = dynamic_cast<Scene::PointCloudComponent*>( ro->getComponent() );
        if ( pointCloud ) {
            auto addGeomShader = [&rt]( Core::Utils::Index pass ) {
                if ( rt->hasConfiguration( pass ) ) {
                    Data::ShaderConfiguration config = rt->getConfiguration( pass );
                    config.addShader( Data::ShaderType_GEOMETRY,
                                      RadiumEngine::getInstance()->getResourcesDir() +
                                          "Shaders/Points/PointCloud.geom.glsl" );
                    rt->setConfiguration( config, pass );
                }
            };

            addGeomShader( DefaultRenderingPasses::LIGHTING_OPAQUE );
            addGeomShader( DefaultRenderingPasses::LIGHTING_TRANSPARENT );
            addGeomShader( DefaultRenderingPasses::Z_PREPASS );
            // construct the parameter provider for the technique
            auto pr = std::make_shared<PointCloudParameterProvider>( material, pointCloud );
            rt->setParametersProvider( pr );
        }
        else {
            rt->setParametersProvider( material );
        }
    }
    else {
        // make the material the parameter provider for the technique
        rt->setParametersProvider( material );
    }
    ro->setRenderTechnique( rt );
    return true;
}

void ForwardRenderer::updateShadowMaps() {
    // Radium V2 : implement shadow mapping
}

} // namespace Rendering
} // namespace Engine
} // namespace Ra
