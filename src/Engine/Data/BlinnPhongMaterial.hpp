#pragma once

#include <Engine/RaEngine.hpp>

#include <map>
#include <string>

#include <Core/Utils/Color.hpp>
#include <Engine/Data/Material.hpp>
#include <Engine/Data/Texture.hpp>

namespace Ra {
namespace Core {
namespace Asset {
class MaterialData;
}
} // namespace Core

namespace Engine {

namespace Data {
class ShaderProgram;

/**
 * Implementation of the Blinn-Phong Material BSDF.
 * @todo due to "Material.glsl" interface modification, must test this version with all plugins,
 * apps, ... that uses Radium Renderer
 */
class RA_ENGINE_API BlinnPhongMaterial final : public Material
{
    friend class BlinnPhongMaterialConverter;

  public:
    /// Semantic of the texture : define which BSDF parameter is controled by the texture
    enum class TextureSemantic { TEX_DIFFUSE, TEX_SPECULAR, TEX_NORMAL, TEX_SHININESS, TEX_ALPHA };

  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    /**
     * Construct a named Blinn-Phongmaterial
     * @param instanceName The name of this instance of the material
     */
    explicit BlinnPhongMaterial( const std::string& instanceName );
    /**
     * Destructor.
     * @note The material does not have ownership on its texture. This destructor do not delete the
     * associated textures.
     */
    ~BlinnPhongMaterial() override;

    void updateGL() override;
    bool isTransparent() const override;

    /**
     * Add an already existing texture to control the specified BSDF parameter.
     * @param semantic The texture semantic
     * @param texture  The texture to use
     */
    inline void addTexture( const TextureSemantic& semantic, Texture* texture );

    /**
     * Get the texture associated to the given semantic.
     * @param semantic
     * @return
     */
    inline Texture* getTexture( const TextureSemantic& semantic ) const;

    /**
     * Register the material in the material library.
     * After registration, the material could be instantiated by any Radium system, renderer,
     * plugin, ...
     */
    static void registerMaterial();

    /**
     * Remove the material from the material library.
     * After removal, the material is no more available, ...
     */
    static void unregisterMaterial();

  public:
    Core::Utils::Color m_kd { 0.7, 0.7, 0.7, 1.0 };
    Core::Utils::Color m_ks { 0.3, 0.3, 0.3, 1.0 };
    Scalar m_ns { 64.0 };
    Scalar m_alpha { 1.0 };
    bool m_perVertexColor { false };
    bool m_renderAsSplat { false };

    /**
     * Add an new texture, from a TextureData, to control the specified BSDF parameter.
     * @param semantic The texture semantic
     * @param texture  The texture to use (file)
     * @return the corresponding TextureData struct
     */
    inline TextureParameters& addTexture( const TextureSemantic& semantic,
                                          const TextureParameters& texture );

  private:
    std::map<TextureSemantic, Texture*> m_textures;
    std::map<TextureSemantic, TextureParameters> m_pendingTextures;

    /**
     * Add an new texture, from a given file, to control the specified BSDF parameter.
     * @param semantic The texture semantic
     * @param texture  The texture to use (file)
     * @return the corresponding TextureData struct
     */
    inline TextureParameters& addTexture( const TextureSemantic& semantic,
                                          const std::string& texture );

    /**
     * Update the rendering parameters for the Material
     */
    void updateRenderingParameters();
};

/**
 * Converter from an external representation comming from FileData to internal representation.
 */
class RA_ENGINE_API BlinnPhongMaterialConverter final
{
  public:
    BlinnPhongMaterialConverter()  = default;
    ~BlinnPhongMaterialConverter() = default;

    Material* operator()( const Ra::Core::Asset::MaterialData* toconvert );
};
} // namespace Data
} // namespace Engine
} // namespace Ra

#include <Engine/Data/BlinnPhongMaterial.inl>
