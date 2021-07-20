#pragma once

#include <Engine/RaEngine.hpp>

#include <map>
#include <string>

#include <Core/Utils/Color.hpp>
#include <Engine/Data/Material.hpp>
#include <Engine/Data/Texture.hpp>

namespace Ra {
namespace Engine {
namespace Data {
/**
 * Base implementation for simple, monocolored, materials.
 * This material could not be used as is. Only derived class could be used by a renderer.
 */
class RA_ENGINE_API SimpleMaterial : public Material
{
  public:
    /// Semantic of the texture : define which BSDF parameter is controled by the texture
    enum class TextureSemantic { TEX_COLOR, TEX_MASK };

    /**
     * Construct a named  material
     * @param name The name of the material
     */
    explicit SimpleMaterial( const std::string& instanceName,
                             const std::string& materialName,
                             MaterialAspect aspect = MaterialAspect::MAT_OPAQUE );

    /**
     * Destructor.
     * @note The material does not have ownership on its texture.
     * This destructor do not delete the associated textures.
     */
    ~SimpleMaterial() override;

    /**
     * Update the openGL state of the material.
     * This state only consists on associated textures.
     */
    void updateGL() override final;

    /**
     * Add an new texture, from a TextureParameters, to control the specified BSDF parameter.
     * The textures will be finalized (i.e loaded from a file if needed and transformed to OpenGL
     * texture) only when needed by the updateGL method.
     * @param semantic The texture semantic
     * @param texture  The texture to use (Descriptor of the texture by its parameters)
     * @return the corresponding TextureData struct
     */
    inline TextureParameters& addTexture( const TextureSemantic& semantic,
                                          const TextureParameters& texture );

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

  public:
    /// The base color of the material
    Core::Utils::Color m_color { 0.9, 0.9, 0.9, 1.0 };
    /// Indicates if the material will takes its base color from vertices' attributes.
    bool m_perVertexColor { false };

  private:
    /**
     * Update the rendering parameters for the Material
     */
    void updateRenderingParameters();

    /**
     * The openGL initialized textures.
     */
    std::map<TextureSemantic, Texture*> m_textures;
    /**
     * The textures that are associated with the material but ar not yet loaded nor initialized.
     */
    std::map<TextureSemantic, TextureParameters> m_pendingTextures;
};

} // namespace Data
} // namespace Engine
} // namespace Ra

#include <Engine/Data/SimpleMaterial.inl>
