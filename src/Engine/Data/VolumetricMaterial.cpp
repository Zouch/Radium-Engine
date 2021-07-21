#include <Engine/Data/VolumetricMaterial.hpp>

#include <Engine/Data/ShaderConfigFactory.hpp>
#include <Engine/Data/ShaderProgramManager.hpp>
#include <Engine/Data/Texture.hpp>
#include <Engine/Data/TextureManager.hpp>
#include <Engine/RadiumEngine.hpp>
#include <Engine/Rendering/RenderTechnique.hpp>

namespace Ra {
namespace Engine {
namespace Data {

static const std::string materialName { "Volumetric" };

VolumetricMaterial::VolumetricMaterial( const std::string& name ) :
    Material( name, materialName, Material::MaterialAspect::MAT_DENSITY ) {}

VolumetricMaterial::~VolumetricMaterial() {}

void VolumetricMaterial::updateGL() {
    if ( !m_isDirty ) { return; }

    m_isDirty = false;
    updateRenderingParameters();
}

void VolumetricMaterial::updateRenderingParameters() {
    m_renderParameters.addParameter( "material.sigma_a", m_sigma_a );
    m_renderParameters.addParameter( "material.sigma_s", m_sigma_s );
    m_renderParameters.addParameter( "material.g", m_g );
    m_renderParameters.addParameter( "material.scale", m_scale );
    if ( m_stepsize < 0 ) {
        auto dim = std::sqrt( Scalar( m_texture->width() * m_texture->width() +
                                      m_texture->height() * m_texture->height() +
                                      m_texture->depth() * m_texture->depth() ) );
        m_renderParameters.addParameter( "material.stepsize", 1._ra / dim );
    }
    else {
        m_renderParameters.addParameter( "material.stepsize", m_stepsize );
    }
    m_renderParameters.addParameter( "material.density", m_texture );
    m_renderParameters.addParameter( "material.modelToDensity", m_modelToMedium.matrix() );
}

bool VolumetricMaterial::isTransparent() const {
    return false;
}

void VolumetricMaterial::registerMaterial() {
    // For resources access (glsl files) in a filesystem
    auto resourcesRootDir { RadiumEngine::getInstance()->getResourcesDir() };
    auto shaderProgramManager = RadiumEngine::getInstance()->getShaderProgramManager();

    shaderProgramManager->addShaderProgram(
        { { "ComposeVolume" },
          resourcesRootDir + "Shaders/2DShaders/Basic2D.vert.glsl",
          resourcesRootDir + "Shaders/Materials/Volumetric/ComposeVolumeRender.frag.glsl" } );

    // adding the material glsl implementation file
    shaderProgramManager->addNamedString(
        "/Volumetric.glsl", resourcesRootDir + "Shaders/Materials/Volumetric/Volumetric.glsl" );

    // registering re-usable shaders
    Data::ShaderConfiguration lpconfig(
        "Volumetric",
        resourcesRootDir + "Shaders/Materials/Volumetric/Volumetric.vert.glsl",
        resourcesRootDir + "Shaders/Materials/Volumetric/Volumetric.frag.glsl" );
    Data::ShaderConfigurationFactory::addConfiguration( lpconfig );

    // registering re-usable shaders
    Data::ShaderConfiguration trconfig(
        "VolumetricOIT",
        resourcesRootDir + "Shaders/Materials/Volumetric/Volumetric.vert.glsl",
        resourcesRootDir + "Shaders/Materials/Volumetric/VolumetricOIT.frag.glsl" );
    Data::ShaderConfigurationFactory::addConfiguration( trconfig );

    //    Ra::Engine::ShaderConfiguration zprepassconfig(
    //        "ZprepassRayMarching", "Shaders/Materials/RayMarching/RayMarching.vert.glsl",
    //        "Shaders/Materials/RayMarching/DepthAmbientRayMarching.frag.glsl" );
    //    Ra::Engine::ShaderConfigurationFactory::addConfiguration( zprepassconfig );

    //    Ra::Engine::ShaderConfiguration transparentpassconfig(
    //        "LitOITRayMarching", "Shaders/Materials/RayMarching/RayMarching.vert.glsl",
    //        "Shaders/Materials/RayMarching/LitOITRayMarching.frag.glsl" );
    //    Ra::Engine::ShaderConfigurationFactory::addConfiguration( transparentpassconfig );

    // Registering technique
    Rendering::EngineRenderTechniques::registerDefaultTechnique(
        "Volumetric",

        []( Rendering::RenderTechnique& rt, bool /*isTransparent*/ ) {
            /*
            auto lightpassconfig =
                Ra::Engine::ShaderConfigurationFactory::getConfiguration( "Volumetric" );
            rt.setConfiguration( *lightpassconfig,
                                 DefaultRenderingPasses::LIGHTING_OPAQUE );
            */
            auto passconfig = Data::ShaderConfigurationFactory::getConfiguration( "Volumetric" );
            rt.setConfiguration( *passconfig,
                                 Rendering::DefaultRenderingPasses::LIGHTING_VOLUMETRIC );
        } );
}

void VolumetricMaterial::unregisterMaterial() {
    Rendering::EngineRenderTechniques::removeDefaultTechnique( "Volumetric" );
}

} // namespace Data
} // namespace Engine
} // namespace Ra
