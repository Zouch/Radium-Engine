#pragma once

#include <Core/Asset/Camera.hpp>
#include <Engine/Scene/CameraComponent.hpp>
#include <Gui/Viewer/Gizmo/Gizmo.hpp>

namespace Ra {
namespace Gui {

/**
 * A ScaleGizmo manipulates the scaling part of a transformation.
 * \note Scaling is always applied in local frame.
 * \note Scaling is always strictly positive.
 */
// TODO: find how to apply it in global frame.
class ScaleGizmo : public Gizmo
{
  public:
    // Draw a frame with : 3 RGB cube-shaped arrows and 3 RGB squares
    ScaleGizmo( Engine::Scene::Component* c,
                const Core::Transform& worldTo,
                const Core::Transform& t,
                Mode mode );

    void updateTransform( Gizmo::Mode mode,
                          const Core::Transform& worldTo,
                          const Core::Transform& t ) override;
    void selectConstraint( int drawableIndex ) override;
    bool isSelected() override { return m_selectedAxis != -1 || m_selectedPlane != -1; }
    void setInitialState( const Core::Asset::Camera& cam, const Core::Vector2& initialXY ) override;
    Core::Transform mouseMove( const Core::Asset::Camera& cam,
                               const Core::Vector2& nextXY,
                               bool stepped,
                               bool whole ) override;

  private:
    Ra::Core::Vector3 m_prevScale;  ///< The previously applied scale.
    Ra::Core::Vector3 m_startPos;   ///< The position of the gizmo when edition starts.
    Ra::Core::Vector3 m_startPoint; ///< The picked 3D point on the gizmo.
    Core::Vector2 m_initialPix;     ///< The pixel position when edition starts.
    int m_selectedAxis;             ///< The axis to scale in.
    int m_selectedPlane;            ///< The plane to scale in.
    bool m_start { false };         ///< Did the edition start.
};

} // namespace Gui
} // namespace Ra
