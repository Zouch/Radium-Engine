#include <Engine/Data/BlinnPhongMaterial.hpp>

#include <Core/Asset/BlinnPhongMaterialData.hpp>
#include <Engine/Data/MaterialConverters.hpp>
#include <Engine/Data/ShaderConfigFactory.hpp>
#include <Engine/Data/ShaderProgramManager.hpp>
#include <Engine/Data/Texture.hpp>
#include <Engine/Data/TextureManager.hpp>
#include <Engine/RadiumEngine.hpp>
#include <Engine/Rendering/RenderTechnique.hpp>

namespace Ra {
namespace Engine {
namespace Data {
static const std::string materialName { "BlinnPhong" };

BlinnPhongMaterial::BlinnPhongMaterial( const std::string& instanceName ) :
    Material( instanceName, materialName, Material::MaterialAspect::MAT_OPAQUE ) {}

BlinnPhongMaterial::~BlinnPhongMaterial() {
    m_textures.clear();
}

void BlinnPhongMaterial::updateRenderingParameters() {
    // update the rendering parameters
    m_renderParameters.addParameter( "material.kd", m_kd );
    m_renderParameters.addParameter( "material.hasPerVertexKd", m_perVertexColor );
    m_renderParameters.addParameter( "material.renderAsSplat", m_renderAsSplat );
    m_renderParameters.addParameter( "material.ks", m_ks );
    m_renderParameters.addParameter( "material.ns", m_ns );
    m_renderParameters.addParameter( "material.alpha", std::min( m_alpha, m_kd[3] ) );
    Texture* tex = getTexture( BlinnPhongMaterial::TextureSemantic::TEX_DIFFUSE );
    if ( tex != nullptr )
    {
        m_renderParameters.addParameter( "material.tex.kd", tex );
        m_renderParameters.addParameter( "material.tex.hasKd", 1 );
    }
    else
    { m_renderParameters.addParameter( "material.tex.hasKd", 0 ); }
    tex = getTexture( BlinnPhongMaterial::TextureSemantic::TEX_SPECULAR );
    if ( tex != nullptr )
    {
        m_renderParameters.addParameter( "material.tex.ks", tex );
        m_renderParameters.addParameter( "material.tex.hasKs", 1 );
    }
    else
    { m_renderParameters.addParameter( "material.tex.hasKs", 0 ); }
    tex = getTexture( BlinnPhongMaterial::TextureSemantic::TEX_NORMAL );
    if ( tex != nullptr )
    {
        m_renderParameters.addParameter( "material.tex.normal", tex );
        m_renderParameters.addParameter( "material.tex.hasNormal", 1 );
    }
    else
    { m_renderParameters.addParameter( "material.tex.hasNormal", 0 ); }
    tex = getTexture( BlinnPhongMaterial::TextureSemantic::TEX_SHININESS );
    if ( tex != nullptr )
    {
        m_renderParameters.addParameter( "material.tex.ns", tex );
        m_renderParameters.addParameter( "material.tex.hasNs", 1 );
    }
    else
    { m_renderParameters.addParameter( "material.tex.hasNs", 0 ); }
    tex = getTexture( BlinnPhongMaterial::TextureSemantic::TEX_ALPHA );
    if ( tex != nullptr )
    {
        m_renderParameters.addParameter( "material.tex.alpha", tex );
        m_renderParameters.addParameter( "material.tex.hasAlpha", 1 );
    }
    else
    { m_renderParameters.addParameter( "material.tex.hasAlpha", 0 ); }
}

void BlinnPhongMaterial::updateGL() {
    if ( !m_isDirty ) { return; }

    // Load textures
    auto texManager = RadiumEngine::getInstance()->getTextureManager();
    for ( const auto& tex : m_pendingTextures )
    {
        // ask to convert color textures from sRGB to Linear RGB
        bool tolinear         = ( tex.first == TextureSemantic::TEX_DIFFUSE ||
                          tex.first == TextureSemantic::TEX_SPECULAR );
        auto texture          = texManager->getOrLoadTexture( tex.second, tolinear );
        m_textures[tex.first] = texture;
        // do not call addTexture since it invalidate m_pendingTextures itr
        //       addTexture( tex.first, texture );
    }

    m_pendingTextures.clear();
    m_isDirty = false;

    updateRenderingParameters();
}

bool BlinnPhongMaterial::isTransparent() const {
    return ( m_alpha < 1_ra ) || ( m_kd[3] < 1_ra ) || Material::isTransparent();
}

void BlinnPhongMaterial::registerMaterial() {
    // For resources access (glsl files) in a filesystem
    auto resourcesRootDir { RadiumEngine::getInstance()->getResourcesDir() };
    auto shaderProgramManager = RadiumEngine::getInstance()->getShaderProgramManager();

    // Defining the material converter
    EngineMaterialConverters::registerMaterialConverter( materialName,
                                                         BlinnPhongMaterialConverter() );

    // adding the material glsl implementation file
    shaderProgramManager->addNamedString(
        "/BlinnPhong.glsl", resourcesRootDir + "Shaders/Materials/BlinnPhong/BlinnPhong.glsl" );
    // registering re-usable shaders
    Data::ShaderConfiguration lpconfig(
        "BlinnPhong",
        resourcesRootDir + "Shaders/Materials/BlinnPhong/BlinnPhong.vert.glsl",
        resourcesRootDir + "Shaders/Materials/BlinnPhong/BlinnPhong.frag.glsl" );
    Data::ShaderConfigurationFactory::addConfiguration( lpconfig );

    Data::ShaderConfiguration zprepassconfig(
        "ZprepassBlinnPhong",
        resourcesRootDir + "Shaders/Materials/BlinnPhong/BlinnPhong.vert.glsl",
        resourcesRootDir + "Shaders/Materials/BlinnPhong/BlinnPhongZPrepass.frag.glsl" );
    Data::ShaderConfigurationFactory::addConfiguration( zprepassconfig );

    Data::ShaderConfiguration transparentpassconfig(
        "LitOITBlinnPhong",
        resourcesRootDir + "Shaders/Materials/BlinnPhong/BlinnPhong.vert.glsl",
        resourcesRootDir + "Shaders/Materials/BlinnPhong/LitOITBlinnPhong.frag.glsl" );
    Data::ShaderConfigurationFactory::addConfiguration( transparentpassconfig );

    // Registering technique
    Rendering::EngineRenderTechniques::registerDefaultTechnique(
        materialName,

        []( Rendering::RenderTechnique& rt, bool isTransparent ) {
            // Configure the technique to render this object using forward Renderer or any
            // compatible one Main pass (Mandatory) : BlinnPhong
            auto lightpass = Data::ShaderConfigurationFactory::getConfiguration( "BlinnPhong" );
            rt.setConfiguration( *lightpass, Rendering::DefaultRenderingPasses::LIGHTING_OPAQUE );

            // Z prepass (Recommended) : DepthAmbiantPass
            auto zprepass =
                Data::ShaderConfigurationFactory::getConfiguration( "ZprepassBlinnPhong" );
            rt.setConfiguration( *zprepass, Rendering::DefaultRenderingPasses::Z_PREPASS );
            // Transparent pass (0ptional) : If Transparent ... add LitOIT
            if ( isTransparent )
            {
                auto transparentpass =
                    Data::ShaderConfigurationFactory::getConfiguration( "LitOITBlinnPhong" );
                rt.setConfiguration( *transparentpass,
                                     Rendering::DefaultRenderingPasses::LIGHTING_TRANSPARENT );
            }
        } );
}

void BlinnPhongMaterial::unregisterMaterial() {
    EngineMaterialConverters::removeMaterialConverter( materialName );
    Rendering::EngineRenderTechniques::removeDefaultTechnique( materialName );
}

Material*
BlinnPhongMaterialConverter::operator()( const Ra::Core::Asset::MaterialData* toconvert ) {
    auto result = new BlinnPhongMaterial( toconvert->getName() );
    // we are sure here that the concrete type of "toconvert" is BlinnPhongMaterialData
    // static cst is safe here
    auto source = static_cast<const Ra::Core::Asset::BlinnPhongMaterialData*>( toconvert );

    if ( source->hasDiffuse() ) result->m_kd = source->m_diffuse;
    if ( source->hasSpecular() ) result->m_ks = source->m_specular;
    if ( source->hasShininess() ) result->m_ns = source->m_shininess;
    if ( source->hasOpacity() ) result->m_alpha = source->m_opacity;
    if ( source->hasDiffuseTexture() )
        result->addTexture( BlinnPhongMaterial::TextureSemantic::TEX_DIFFUSE,
                            source->m_texDiffuse );
    if ( source->hasSpecularTexture() )
        result->addTexture( BlinnPhongMaterial::TextureSemantic::TEX_SPECULAR,
                            source->m_texSpecular );
    if ( source->hasShininessTexture() )
        result->addTexture( BlinnPhongMaterial::TextureSemantic::TEX_SHININESS,
                            source->m_texShininess );
    if ( source->hasOpacityTexture() )
        result->addTexture( BlinnPhongMaterial::TextureSemantic::TEX_ALPHA, source->m_texOpacity );
    if ( source->hasNormalTexture() )
        result->addTexture( BlinnPhongMaterial::TextureSemantic::TEX_NORMAL, source->m_texNormal );

    return result;
}

} // namespace Data
} // namespace Engine
} // namespace Ra
