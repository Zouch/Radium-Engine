#pragma once

#include <Core/Utils/Color.hpp>
#include <Engine/RaEngine.hpp>
#include <Engine/Scene/Component.hpp>

namespace Ra {
namespace Engine {
namespace Data {
class RenderParameters;
}

namespace Scene {

class Entity;

// Radium-V2 : this class could totally be renamed LightComponent and get a Light struct embedded.
// Thoughts are welcome !
/**
 * Light base class for rendering
 */
class RA_ENGINE_API Light : public Component
{
  public:
    /** supported light type.
     * @note POLYGONAL is not really supported.
     * @todo : replace this enum by something extensible as we plan to add new light type by plugins
     */
    enum LightType { DIRECTIONAL = 0, POINT, SPOT, POLYGONAL };

    /**
     * Define the attenuation of the light source
     * @todo for the moment, the attenuation is non physically coherent. make attenuation be
     * quadratic by default
     */
    struct Attenuation {
        Scalar constant { 1 };
        Scalar linear { 0 };
        Scalar quadratic { 0 };
        Attenuation() = default;
    };

  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    /**
     * Create a new light. The new light is a component attached to the given entity
     * @param entity
     * @param type
     * @param name
     */
    Light( Entity* entity, const LightType& type, const std::string& name = "light" );
    ~Light() override = default;

    /**
     *
     * @return the color of the light
     */
    inline const Core::Utils::Color& getColor() const;
    /**
     * Set the color of the light
     * @param color
     */
    inline void setColor( const Core::Utils::Color& color );

    /**
     * set the direction of lighting
     * @todo put this only on directional and spot light sources
     * @param dir
     */
    virtual void setDirection( const Eigen::Matrix<Scalar, 3, 1>& /*dir*/ ) {}
    /**
     * Set the position of the light source
     * @todo put this only on point and spot light sources
     * @param pos
     */
    virtual void setPosition( const Eigen::Matrix<Scalar, 3, 1>& /*pos*/ ) {}
    // ...

    /**
     *
     * @return the type (an enum Light::LightType member) of the light source
     */
    inline const LightType& getType() const;

    /**
     * Extrac the set of parameters that must be given to a shader for rendering and lighting with
     * this light.
     * @param params
     */
    virtual void getRenderParameters( Data::RenderParameters& params ) const;

    /**
     * Abstract method that define the glsl code that manage this light type
     * For the moment, this is not use (except by experimental plugins) but will part of the shader
     * composition process of radium v2
     * @return
     */
    virtual std::string getShaderInclude() const;

    /**
     * Nothing to initialize here ...
     */
    void initialize() override;

  private:
    Core::Utils::Color m_color = Core::Utils::Color::White();

    LightType m_type { LightType::DIRECTIONAL };
};

} // namespace Scene
} // namespace Engine
} // namespace Ra

#include "Light.inl"
