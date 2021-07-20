#pragma once

#include <Engine/RaEngine.hpp>

#include <map>
#include <string>

// This include brings only the macro EIGEN_MAKE_ALIGNED_OPERATOR_NEW in the file scope.
// Need to be separated to reduce compilation time
#include <Core/Types.hpp>

#include <Engine/Data/RenderParameters.hpp>

namespace Ra {
namespace Engine {
namespace Data {
/**
 * Base class for materials/
 * Do not assume a given Material representation but only make the difference between opaque and
 * transparent materials.
 *
 */
class RA_ENGINE_API Material : public Data::ShaderParameterProvider
{
  public:
    /**
     * Identifies the type of the material.
     *  MAT_OPAQUE and MAT_TRANSPARENT implements the GLSL "surfacic" BSDF interface
     *  MAT_DENSITY implements the GLSL "volumetric" interface
     */
    enum class MaterialAspect {
        MAT_OPAQUE,      /// <- The material is either OPAQUE or TRANSPARENT
        MAT_TRANSPARENT, /// <- The material is TRANSPARENT
        MAT_DENSITY      /// <- The material implements the VOLUME interface
    };

  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  protected:
    /**
     * Creates a named material with the given aspect
     * @param instanceName
     * @param materialName
     * @param aspect
     */
    explicit Material( const std::string& instanceName,
                       const std::string& materialName,
                       MaterialAspect aspect = MaterialAspect::MAT_OPAQUE );

    /** Change the Material Name
     * @note This method should be used carefully as the name is a key for render technique factory
     */
    inline void setMaterialName( std::string newName );

  public:
    virtual ~Material() = default;

    /**
     * @return the name of the material instance
     */
    inline const std::string& getInstanceName() const;

    /**
     * @return the name of the material, can be used a UUID
     * @note the material name is expected to be used to define the ShaderConfiguration name
     */
    inline const std::string& getMaterialName() const;

    /** set the aspect (MAT_OPAQUE or MAT_TRANSPARENT) of the material.
     * @param aspect
     */
    inline void setMaterialAspect( const MaterialAspect& aspect );

    /** Get the aspect (MAT_OPAQUE or MAT_TRANSPARENT) of the material.
     *
     * @return the current aspect of the Material
     */
    inline const MaterialAspect& getMaterialAspect() const;

    /** Test if material is transperent.
     * @return true if the material is transparent
     */
    virtual bool isTransparent() const;

    /**
     * Get the list of properties the material migh use in a shader.
     * each property will be added to the shader used for rendering this material under the form
     * "#define theProperty". Shaders that support the given property could then fully render the
     * material. Others migh render the meterial eroneously.
     *
     * The defaul implementation returns an empty list.
     *
     * @todo : Validate this proposal
     */
    std::list<std::string> getPropertyList() const override;

    /** Mark the Material as needing update before the next OpenGL call
     *
     */
    inline void needUpdate();

  protected:
    /// Material instance name
    std::string m_instanceName {};
    /// Material aspect
    MaterialAspect m_aspect { MaterialAspect::MAT_OPAQUE };
    /// Dirty mark : true if the openGL state of the material need to be updated before next draw
    /// call
    bool m_isDirty { true };

  private:
    /// Unique material name that can be used to identify the material class
    std::string m_materialName;
};

} // namespace Data
} // namespace Engine
} // namespace Ra
#include <Engine/Data/Material.inl>
