#pragma once
#include <Gui/RaGui.hpp>

#include <memory>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QObject>

#include <Core/CoreMacros.hpp>
#include <Core/Types.hpp>
#include <Core/Utils/Log.hpp>

#include <Core/Asset/Camera.hpp>
#include <Engine/Scene/CameraComponent.hpp>
#include <Gui/Utils/KeyMappingManager.hpp>

namespace Ra {
namespace Engine {
namespace Scene {
class Light;
}

} // namespace Engine

namespace Gui {

/// The CameraManipulator class is the generic class for camera manipulators.
class RA_GUI_API CameraManipulator : public QObject
{
    Q_OBJECT

  public:
    /// Initializes a manipulator for a given viewport size.
    CameraManipulator();

    /// Initializes a manipulator keeping properties from an already existing one.
    /// This allows to switch from one manipulator to another while keeping the same visual
    /// experience.
    explicit CameraManipulator( const CameraManipulator& other );

    /// Destructor.
    /// As a Manipulator does not have ownership over the associated Camera, do not release the
    /// associated Camera.
    virtual ~CameraManipulator();

    /// @return the mapping context for keymapping, nullptr if no mapping is available
    virtual KeyMappingManager::Context mappingContext();

    /// @return true if the event has been taken into account, false otherwise
    virtual bool handleMousePressEvent( QMouseEvent* event,
                                        const Qt::MouseButtons& buttons,
                                        const Qt::KeyboardModifiers& modifiers,
                                        int key );
    /// @return true if the event has been taken into account, false otherwise
    virtual bool handleMouseReleaseEvent( QMouseEvent* event );
    /// @return true if the event has been taken into account, false otherwise
    virtual bool handleMouseMoveEvent( QMouseEvent* event,
                                       const Qt::MouseButtons& buttons,
                                       const Qt::KeyboardModifiers& modifiers,
                                       int key );
    /// @return true if the event has been taken into account, false otherwise
    virtual bool handleWheelEvent( QWheelEvent* event,
                                   const Qt::MouseButtons& buttons,
                                   const Qt::KeyboardModifiers& modifiers,
                                   int key );

    /// @return true if the event has been taken into account, false otherwise
    virtual bool handleKeyPressEvent( QKeyEvent* event,
                                      const KeyMappingManager::KeyMappingAction& action );
    /// @return true if the event has been taken into account, false otherwise
    virtual bool handleKeyReleaseEvent( QKeyEvent* event,
                                        const KeyMappingManager::KeyMappingAction& action );

    /// Pointer access to the camera.
    const Core::Asset::Camera* getCamera() const { return m_camera; }

    /// Pointer access to the camera.
    Core::Asset::Camera* getCamera() { return m_camera; }

    /// Reset manipulator internal data according to current active camera from manager.
    /// Call each time the active camera is changed to have coherent data.
    virtual void updateCamera();

    /// Set the Light attached to the camera.
    /// \note CameraManipulator doesn't have ownership.
    void attachLight( Engine::Scene::Light* light );

    /// @return true if a Light is attached to the camera, false otherwise.
    bool hasLightAttached() const { return m_light != nullptr; }

    /// pointer acces to the attached light if it exists, returns nullptr otherwise.
    Engine::Scene::Light* getLight() { return m_light; }

  public slots:
    /// \name Camera properties setters
    ///@{
    void setCameraSensitivity( Scalar sensitivity );
    void setCameraFov( Scalar fov );
    void setCameraFovInDegrees( Scalar fov );
    void setCameraZNear( Scalar zNear );
    void setCameraZFar( Scalar zFar );
    ///@}

    /// Set the AABB to restrain the camera behavior against.
    void mapCameraBehaviourToAabb( const Core::Aabb& aabb );

    /// Free the camera from AABB restriction.
    void unmapCameraBehaviourToAabb();

    /// Setup the Camera according to the AABB of the scene to render.
    virtual void fitScene( const Core::Aabb& aabb ) = 0;

    /// Set the Camera position to \p position.
    virtual void setCameraPosition( const Core::Vector3& position ) = 0;

    /// Set the Camera target to \p target.
    virtual void setCameraTarget( const Core::Vector3& target ) = 0;

    /// Reset the Camera settings to default values.
    virtual void resetCamera() = 0;

  signals:

  protected:
    /// the Camera sensitivity to manipulation.
    Scalar m_cameraSensitivity { 1_ra };
    /// Additional factor for camera sensitivity.
    Scalar m_quickCameraModifier { 1_ra };
    /// Speed modifier on mouse wheel events.
    Scalar m_wheelSpeedModifier { 0.02_ra };

    Core::Aabb m_targetedAabb;                 ///< Camera behavior restriction AABB.
    Scalar m_targetedAabbVolume { 0_ra };      ///< Volume of the m_targetedAabb
    bool m_mapCameraBahaviourToAabb { false }; ///< whether the camera is restrained or not

    /// Target point of the camera (usefull for most of the manipulator metaphor)
    /// Be aware that m_target must always be on the line of sight of the camera so that it
    /// could be used as a "focus" point by a manipulator.
    Core::Vector3 m_target { 0_ra, 0_ra, 0_ra };

    /// Whether the corresponding camera movement is active or not.
    KeyMappingManager::KeyMappingAction m_currentAction {};

    Core::Asset::Camera* m_camera { nullptr }; ///< The Camera.
    Engine::Scene::Light* m_light { nullptr }; ///< The light attached to the Camera.
};

} // namespace Gui
} // namespace Ra
