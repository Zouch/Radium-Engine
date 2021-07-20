#pragma once
#include <Gui/RaGui.hpp>

#include <Core/Asset/Camera.hpp>
#include <Engine/Scene/CameraComponent.hpp>
#include <Gui/Viewer/CameraManipulator.hpp>

namespace Ra {
namespace Gui {

/// A Trackball manipulator for Cameras.
class RA_GUI_API TrackballCameraManipulator
    : public CameraManipulator,
      public KeyMappingManageable<TrackballCameraManipulator>
{
    Q_OBJECT
    friend class KeyMappingManageable<TrackballCameraManipulator>;

  public:
    using KeyMapping = KeyMappingManageable<TrackballCameraManipulator>;

    /// Default constructor
    TrackballCameraManipulator();

    /// Copy constructor used when switching camera manipulator
    /// Requires that m_target is on the line of sight of the camera.
    explicit TrackballCameraManipulator( const CameraManipulator& other );

    /// Destructor.
    virtual ~TrackballCameraManipulator();

    // KeyMappingManager::Context mappingContext();

    bool handleMousePressEvent( QMouseEvent* event,
                                const Qt::MouseButtons& buttons,
                                const Qt::KeyboardModifiers& modifiers,
                                int key ) override;
    bool handleMouseReleaseEvent( QMouseEvent* event ) override;
    bool handleMouseMoveEvent( QMouseEvent* event,
                               const Qt::MouseButtons& buttons,
                               const Qt::KeyboardModifiers& modifiers,
                               int key ) override;
    bool handleWheelEvent( QWheelEvent* event,
                           const Qt::MouseButtons& buttons,
                           const Qt::KeyboardModifiers& modifiers,
                           int key ) override;

    bool handleKeyPressEvent( QKeyEvent* event,
                              const KeyMappingManager::KeyMappingAction& action ) override;

    void toggleRotateAround();
    void updateCamera() override;

    /// Set the distance from the camera to the target point.
    /// update target m_referenceFrame.translation();
    /// \note doesn't modify the camera.
    void setTrackballRadius( Scalar rad );

    /// Return the distance from the camera to the target point.
    Scalar getTrackballRadius() const;

    /// Return the trackball center.
    /// \note doesn't modify the camera.
    const Core::Transform::ConstTranslationPart getTrackballCenter() const;

    KeyMappingManager::Context mappingContext() override;

  public slots:
    void setCameraPosition( const Core::Vector3& position ) override;
    void setCameraTarget( const Core::Vector3& target ) override;
    void fitScene( const Core::Aabb& aabb ) override;

    void resetCamera() override;

  protected:
    virtual void handleCameraRotate( Scalar dx, Scalar dy );
    virtual void handleCameraPan( Scalar dx, Scalar dy );
    virtual void handleCameraZoom( Scalar dx, Scalar dy );
    virtual void handleCameraZoom( Scalar z );
    virtual void handleCameraMoveForward( Scalar dx, Scalar dy );
    virtual void handleCameraMoveForward( Scalar z );

    /// Update the polar coordinates of the Camera w.r.t. the trackball center.
    void updatePhiTheta();

  protected:
    // the center of the trackball is defined by the m_referenceFrame.translation()

    /// x-position of the mouse on the screen at the manipulation start.
    Scalar m_lastMouseX { 0_ra };

    /// y-position of the mouse on the screen at the manipulation start.
    Scalar m_lastMouseY { 0_ra };

    /// Spherical coordinates   (ISO 80000-2:2019 convention)
    /// https://en.wikipedia.org/wiki/Spherical_coordinate_system
    /// phi is azimutal
    /// theta is polar, from y which is world "up" direction
    /// rest pose correspond to camera view direction at m_referenceFrame -z;
    Scalar m_phi { 0_ra };
    Scalar m_theta { 0_ra };
    /// sign of m_theta at mousePressEvent, to guide the phi rotation direction.
    Scalar m_phiDir { 1_ra };

    /// initial frame of the camera, centered on target, to compute angles.
    Core::Transform m_referenceFrame;

    /// The distance from the camera to the trackball center.
    Scalar m_distFromCenter { 0_ra };

  private:
    bool checkIntegrity( const std::string& mess ) const;
    static void configureKeyMapping_impl();
    void clampThetaPhi();

  protected:
    ///\todo move CAMERA_ to CameraManipulator, will be done soon ;)
#define KeyMappingCamera                      \
    KMA_VALUE( TRACKBALLCAMERA_ROTATE )       \
    KMA_VALUE( TRACKBALLCAMERA_PAN )          \
    KMA_VALUE( TRACKBALLCAMERA_ZOOM )         \
    KMA_VALUE( TRACKBALLCAMERA_PROJ_MODE )    \
    KMA_VALUE( TRACKBALLCAMERA_MOVE_FORWARD ) \
    KMA_VALUE( CAMERA_TOGGLE_QUICK )

#define KMA_VALUE( XX ) static KeyMappingManager::KeyMappingAction XX;
    KeyMappingCamera
#undef KMA_VALUE
};

} // namespace Gui
} // namespace Ra
