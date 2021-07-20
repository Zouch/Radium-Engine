#pragma once

#include <Core/Asset/Camera.hpp>
#include <Core/Math/Math.hpp>
#include <Core/Types.hpp>
#include <Engine/RaEngine.hpp>
#include <Engine/Scene/Component.hpp>

namespace Ra {
namespace Engine {
namespace Scene {

class Entity;

/// A Camera is an Engine Component storing a Camera object.
///\todo attached camera do not take entity's transformation into account. This prevents, for now,
/// keyframed animation of the Camera from its entity ...
class RA_ENGINE_API CameraComponent : public Scene::Component
{
  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    CameraComponent( Entity* entity, const std::string& name, Scalar height = 1, Scalar width = 1 );

    ~CameraComponent() override;

    void initialize() override;

    /// Toggle on/off displaying the Camera.
    void show( bool on );

    /// Retrieve owned camera transform and apply to the RO
    void updateTransform();

    /// Duplicate the camera under a different Entity and a different name
    virtual CameraComponent* duplicate( Entity* cloneEntity, const std::string& cloneName ) const;

    Core::Asset::Camera* getCamera() const { return m_camera.get(); }
    Core::Asset::Camera* getCamera() { return m_camera.get(); }
    const Rendering::RenderObject* getRenderObject() const { return m_RO; }

  protected:
    std::unique_ptr<Core::Asset::Camera> m_camera;
    Rendering::RenderObject* m_RO { nullptr }; ///< Render mesh for the camera.
};
} // namespace Scene
} // namespace Engine
} // namespace Ra
