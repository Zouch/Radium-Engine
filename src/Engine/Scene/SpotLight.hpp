#pragma once

#include <Core/Math/Math.hpp>
#include <Engine/RaEngine.hpp>
#include <Engine/Scene/Light.hpp>

namespace Ra {
namespace Engine {

namespace Scene {
class Entity;
/** Spot light for rendering.
 *
 */
class RA_ENGINE_API SpotLight final : public Ra::Engine::Scene::Light
{

  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    explicit SpotLight( Entity* entity, const std::string& name = "spotlight" );
    ~SpotLight() override = default;

    void getRenderParameters( Data::RenderParameters& params ) const override;

    void setPosition( const Eigen::Matrix<Scalar, 3, 1>& position ) override;
    inline const Eigen::Matrix<Scalar, 3, 1>& getPosition() const;

    void setDirection( const Eigen::Matrix<Scalar, 3, 1>& direction ) override;
    inline const Eigen::Matrix<Scalar, 3, 1>& getDirection() const;

    inline void setInnerAngleInRadians( Scalar angle );
    inline void setOuterAngleInRadians( Scalar angle );
    inline void setInnerAngleInDegrees( Scalar angle );
    inline void setOuterAngleInDegrees( Scalar angle );

    inline Scalar getInnerAngle() const;
    inline Scalar getOuterAngle() const;

    inline void setAttenuation( const Attenuation& attenuation );
    inline void setAttenuation( Scalar constant, Scalar linear, Scalar quadratic );
    inline const Attenuation& getAttenuation() const;

    std::string getShaderInclude() const override;

  private:
    Eigen::Matrix<Scalar, 3, 1> m_position { 0, 0, 0 };
    Eigen::Matrix<Scalar, 3, 1> m_direction { 0, -1, 0 };

    Scalar m_innerAngle { Core::Math::PiDiv4 };
    Scalar m_outerAngle { Core::Math::PiDiv2 };

    Attenuation m_attenuation { 1, 0, 0 };
};
} // namespace Scene
} // namespace Engine
} // namespace Ra

#include "SpotLight.inl"
