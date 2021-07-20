#pragma once

#include <Engine/RaEngine.hpp>
#include <Engine/Scene/Light.hpp>

namespace Ra {
namespace Engine {
namespace Scene {

class Entity;

/** Directional light for rendering
 *
 */
class RA_ENGINE_API DirectionalLight final : public Ra::Engine::Scene::Light
{
  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    explicit DirectionalLight( Entity* entity, const std::string& name = "dirlight" );
    ~DirectionalLight() override = default;

    void getRenderParameters( Data::RenderParameters& params ) const override;

    void setDirection( const Eigen::Matrix<Scalar, 3, 1>& dir ) override;
    inline const Eigen::Matrix<Scalar, 3, 1>& getDirection() const;

    std::string getShaderInclude() const override;

  private:
    Eigen::Matrix<Scalar, 3, 1> m_direction { 0, -1, 0 };
};
} // namespace Scene
} // namespace Engine
} // namespace Ra

#include "DirLight.inl"
