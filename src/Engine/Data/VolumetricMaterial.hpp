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

class ShaderProgram;

/**
 * Implementation of a simple Volumetric Material.
 * Based on a 3D texture defining the density distribution in the volume, perform ray-marching
 * and absorption based rendering of the volume..
 * @todo This material does not implement the MaterialGLSL interface. Shader compositing is not
 * allowed.
 */
class RA_ENGINE_API VolumetricMaterial final : public Material
{

  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    /**
     * Construct a named VolumetricMaterial
     * @param name The name of the material
     */
    explicit VolumetricMaterial( const std::string& name );
    /**
     * Destructor.
     * @note The material does not have ownership on its texture. This destructor do not delete the
     * associated textures.
     */
    ~VolumetricMaterial() override;

    void updateGL() override;
    bool isTransparent() const override;

    /**
     * Add an already existing texture to control the specified BSDF parameter.
     * @param semantic The texture semantic
     * @param texture  The texture to use
     */
    inline void setTexture( Texture* texture );

    /**
     * Get the texture associated to the given semantic.
     * @param semantic
     * @return the corresponding texture
     */
    inline Texture* getTexture() const;

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
    /// Absorption coefficient, default to Air (0.0011, 0.0024, 0.014)
    Core::Utils::Color m_sigma_a { 0.0011_ra, 0.0024_ra, 0.014_ra };
    /// Scattering coefficient, default to Air (2.55, 3.21, 3.77)
    Core::Utils::Color m_sigma_s { 2.55_ra, 3.21_ra, 3.77_ra };
    /// phase function assymetry factor, default to 0
    Scalar m_g { 0_ra };
    /// Scale factor applied to the absorption and scattering coefficients
    Scalar m_scale { 1_ra };
    /// Step size for ray-marching rendering. If negative whane rendering, interpreted as
    /// 1/textureDim.
    Scalar m_stepsize { -1_ra };
    /// Transformation matrix to go from the associated geometry frame to the
    /// canonical [0, 1]^3 density domain
    Core::Transform m_modelToMedium { Core::Transform::Identity() };

  private:
    // The density matrix
    Texture* m_texture;

    /**
     * Update the rendering parameters for the Material
     */
    void updateRenderingParameters();
};

} // namespace Data
} // namespace Engine
} // namespace Ra

#include <Engine/Data/VolumetricMaterial.inl>
