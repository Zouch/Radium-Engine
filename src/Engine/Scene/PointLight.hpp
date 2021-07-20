#pragma once

#include <Engine/RaEngine.hpp>
#include <Engine/Scene/Light.hpp>

namespace Ra {
namespace Engine {

namespace Scene {
class Entity;
/** Point light for rendering
 *
 */
class RA_ENGINE_API PointLight final : public Ra::Engine::Scene::Light
{
  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    explicit PointLight( Entity* entity, const std::string& name = "pointlight" );
    ~PointLight() override = default;

    void getRenderParameters( Data::RenderParameters& params ) const override;

    void setPosition( const Eigen::Matrix<Scalar, 3, 1>& pos ) override;
    inline const Eigen::Matrix<Scalar, 3, 1>& getPosition() const;

    inline void setAttenuation( const Attenuation& attenuation );
    inline void setAttenuation( Scalar constant, Scalar linear, Scalar quadratic );
    inline const Attenuation& getAttenuation() const;

    std::string getShaderInclude() const override;

  private:
    Eigen::Matrix<Scalar, 3, 1> m_position { 0, 0, 0 };

    Attenuation m_attenuation { 1, 0, 0 };
};
} // namespace Scene
} // namespace Engine
} // namespace Ra
#include "PointLight.inl"
