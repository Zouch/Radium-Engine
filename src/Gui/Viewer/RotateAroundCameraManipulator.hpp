#include <Gui/Viewer/TrackballCameraManipulator.hpp>

namespace Ra {
namespace Gui {
class Viewer;

class RA_GUI_API RotateAroundCameraManipulator
    : public Ra::Gui::TrackballCameraManipulator,
      public Ra::Gui::KeyMappingManageable<RotateAroundCameraManipulator>
{
    friend class Ra::Gui::KeyMappingManageable<RotateAroundCameraManipulator>;

  public:
    using KeyMapping = KeyMappingManageable<RotateAroundCameraManipulator>;
    RotateAroundCameraManipulator( const CameraManipulator& cm, Ra::Gui::Viewer* viewer );

    /// @copydoc TrackballCameraManipulator::handleMouseMoveEvent()
    bool handleMouseMoveEvent( QMouseEvent* event,
                               const Qt::MouseButtons& buttons,
                               const Qt::KeyboardModifiers& modifiers,
                               int key ) override;
    /// @copydoc TrackballCameraManipulator::handleKeyPressEvent()
    bool handleKeyPressEvent( QKeyEvent* event,
                              const Ra::Gui::KeyMappingManager::KeyMappingAction& action ) override;

    void setPivot( Ra::Core::Vector3 pivot );
    /// Set 'm_pivot' world coordinate from screen coordinates 'x' and 'y'.
    void setPivotFromPixel( Scalar x, Scalar y );

    /// Align the camera direction and up vector with the closest world axis.
    /// Keep m_target at the same  screen coordinates.
    void alignOnClosestAxis();

    KeyMappingManager::Context mappingContext() override;

  protected:
    virtual void handleCameraRotate( Scalar dx, Scalar dy ) override;
    virtual void handleCameraForward( Scalar z );
    virtual void handleCameraPan( Scalar dx, Scalar dy ) override;

    /// Compute the rotation of the camera from the mouse motion.
    /// Inspired by <a href="http://libqglviewer.com/">libQGLViewer</a>.
    Ra::Core::Quaternion deformedBallQuaternion( Scalar x, Scalar y, Scalar cx, Scalar cy );
    Scalar projectOnBall( Scalar x, Scalar y );

  private:
    Ra::Core::Vector3 m_pivot { 0.0_ra, 0.0_ra, 0.0_ra };

    Ra::Gui::Viewer* m_viewer;

    static void configureKeyMapping_impl();

#define KeyMappingRotateAroundCamera                  \
    KMA_VALUE( ROTATEAROUND_ALIGN_WITH_CLOSEST_AXIS ) \
    KMA_VALUE( ROTATEAROUND_SET_PIVOT )

#define KMA_VALUE( XX ) static KeyMappingManager::KeyMappingAction XX;
    KeyMappingRotateAroundCamera
#undef KMA_VALUE
};

} // namespace Gui
} // namespace Ra
