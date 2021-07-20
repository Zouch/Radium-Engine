#include <Gui/Viewer/TrackballCameraManipulator.hpp>

#include <Core/Asset/Camera.hpp>
#include <Core/Math/Math.hpp>
#include <Core/Utils/Log.hpp>
#include <Engine/RadiumEngine.hpp>
#include <Engine/Rendering/RenderObject.hpp>
#include <Engine/Rendering/RenderObjectManager.hpp>
#include <Engine/Scene/Light.hpp>
#include <Engine/Scene/SystemDisplay.hpp>
#include <Gui/Utils/KeyMappingManager.hpp>
#include <Gui/Utils/Keyboard.hpp>

#include <Engine/Scene/CameraComponent.hpp>
#include <QApplication>
#include <QMessageBox>
#include <algorithm>
#include <iostream>

namespace Ra {

using Core::Math::Pi;
using namespace Ra::Core::Utils;

namespace Gui {

#define KMA_VALUE( XX ) KeyMappingManager::KeyMappingAction TrackballCameraManipulator::XX;
KeyMappingCamera
#undef KMA_VALUE

void TrackballCameraManipulator::configureKeyMapping_impl() {

    KeyMapping::setContext( KeyMappingManager::getInstance()->getContext( "CameraContext" ) );
    if ( KeyMapping::getContext().isInvalid() )
    {
        LOG( logINFO )
            << "CameraContext not defined (maybe the configuration file do not contains it)";
        LOG( logERROR ) << "CameraContext all keymapping invalide !";
        return;
    }

#define KMA_VALUE( XX ) \
    XX = KeyMappingManager::getInstance()->getActionIndex( KeyMapping::getContext(), #XX );
    KeyMappingCamera
#undef KMA_VALUE
}

TrackballCameraManipulator::TrackballCameraManipulator() : CameraManipulator() {
    resetCamera();
}

TrackballCameraManipulator::TrackballCameraManipulator( const CameraManipulator& other ) :
    CameraManipulator( other ) {
    m_referenceFrame = m_camera->getFrame();
    m_referenceFrame.translation() =
        m_camera->getPosition() + m_distFromCenter * m_camera->getDirection();
    m_distFromCenter = ( m_referenceFrame.translation() - m_camera->getPosition() ).norm();
    updatePhiTheta();
}

TrackballCameraManipulator::~TrackballCameraManipulator() = default;

void TrackballCameraManipulator::resetCamera() {
    m_camera->setFrame( Core::Transform::Identity() );
    m_camera->setPosition( Core::Vector3( 0_ra, 0_ra, 2_ra ) );
    m_camera->setDirection( Core::Vector3( 0_ra, 0_ra, -1_ra ) );
    m_distFromCenter               = 2.0_ra;
    m_referenceFrame               = Core::Transform::Identity();
    m_referenceFrame.translation() = Core::Vector3::Zero();

    updatePhiTheta();

    ///\todo get rid of these light stuff
    if ( m_light != nullptr )
    {
        m_light->setPosition( m_camera->getPosition() );
        m_light->setDirection( m_camera->getDirection() );
    }
}

void TrackballCameraManipulator::updateCamera() {
    // try to keep target near the previous camera's one, take it at the same distance from
    // camera, but in the new direction.
    m_distFromCenter = ( m_referenceFrame.translation() - m_camera->getPosition() ).norm();
    m_referenceFrame = m_camera->getFrame();
    m_referenceFrame.translation() =
        m_camera->getPosition() + m_distFromCenter * m_camera->getDirection();

    updatePhiTheta();

    if ( m_light != nullptr )
    {
        m_light->setPosition( m_camera->getPosition() );
        m_light->setDirection( m_camera->getDirection() );
    }
}

void TrackballCameraManipulator::setTrackballRadius( Scalar rad ) {
    m_distFromCenter = rad;
    m_referenceFrame.translation() =
        m_camera->getPosition() + m_distFromCenter * m_camera->getDirection();
}

Scalar TrackballCameraManipulator::getTrackballRadius() const {
    return m_distFromCenter;
}

Core::Transform::ConstTranslationPart TrackballCameraManipulator::getTrackballCenter() const {
    return m_referenceFrame.translation();
}

KeyMappingManager::Context TrackballCameraManipulator::mappingContext() {
    return KeyMapping::getContext();
}

bool TrackballCameraManipulator::handleMousePressEvent( QMouseEvent* event,
                                                        const Qt::MouseButtons& buttons,
                                                        const Qt::KeyboardModifiers& modifiers,
                                                        int key ) {
    m_lastMouseX    = event->pos().x();
    m_lastMouseY    = event->pos().y();
    m_phiDir        = -Core::Math::signNZ( m_theta );
    m_currentAction = KeyMappingManager::getInstance()->getAction(
        KeyMapping::getContext(), buttons, modifiers, key, false );

    return m_currentAction.isValid();
}

bool TrackballCameraManipulator::handleMouseMoveEvent( QMouseEvent* event,
                                                       const Qt::MouseButtons& /*buttons*/,
                                                       const Qt::KeyboardModifiers& /* modifiers*/,
                                                       int /*key*/ ) {

    Scalar dx = ( event->pos().x() - m_lastMouseX ) / m_camera->getWidth();
    Scalar dy = ( event->pos().y() - m_lastMouseY ) / m_camera->getHeight();

    if ( m_currentAction == TRACKBALLCAMERA_ROTATE )
        handleCameraRotate( dx, dy );
    else if ( m_currentAction == TRACKBALLCAMERA_PAN )
        handleCameraPan( dx, dy );
    else if ( m_currentAction == TRACKBALLCAMERA_ZOOM )
        handleCameraZoom( dx, dy );
    else if ( m_currentAction == TRACKBALLCAMERA_MOVE_FORWARD )
        handleCameraMoveForward( dx, dy );

    m_lastMouseX = event->pos().x();
    m_lastMouseY = event->pos().y();

    if ( m_light != nullptr )
    {
        m_light->setPosition( m_camera->getPosition() );
        m_light->setDirection( m_camera->getDirection() );
    }

    return m_currentAction.isValid();
}

bool TrackballCameraManipulator::handleMouseReleaseEvent( QMouseEvent* /*event*/ ) {
    m_currentAction = KeyMappingManager::KeyMappingAction::Invalid();

    return true;
}

bool TrackballCameraManipulator::handleWheelEvent( QWheelEvent* event,
                                                   const Qt::MouseButtons& buttons,
                                                   const Qt::KeyboardModifiers& modifiers,
                                                   int key

) {
    auto action = KeyMappingManager::getInstance()->getAction(
        KeyMapping::getContext(), buttons, modifiers, key, true );

    if ( action == TRACKBALLCAMERA_MOVE_FORWARD )
    {
        handleCameraMoveForward(
            ( event->angleDelta().y() * 0.01_ra + event->angleDelta().x() * 0.01_ra ) *
            m_wheelSpeedModifier );
    }
    else if ( action == TRACKBALLCAMERA_ZOOM )
    {
        handleCameraZoom(
            ( event->angleDelta().y() * 0.01_ra + event->angleDelta().x() * 0.01_ra ) *
            m_wheelSpeedModifier );
    }

    if ( m_light != nullptr )
    {
        m_light->setPosition( m_camera->getPosition() );
        m_light->setDirection( m_camera->getDirection() );
    }

    return action.isValid();
}

bool TrackballCameraManipulator::handleKeyPressEvent(
    QKeyEvent* /*event*/,
    const KeyMappingManager::KeyMappingAction& action ) {

    static bool quick = false;
    using ProjType    = Ra::Core::Asset::Camera::ProjType;
    if ( action == TRACKBALLCAMERA_PROJ_MODE )
    {
        m_camera->setType( m_camera->getType() == ProjType::ORTHOGRAPHIC ? ProjType::PERSPECTIVE
                                                                         : ProjType::ORTHOGRAPHIC );
        return true;
    }
    else if ( action == CAMERA_TOGGLE_QUICK )
    {
        quick = !quick;
        if ( quick ) { m_quickCameraModifier = 10.0_ra; }
        else
        { m_quickCameraModifier = 1.0_ra; }
    }

    return false;
}

void TrackballCameraManipulator::setCameraPosition( const Core::Vector3& position ) {
    if ( position == m_referenceFrame.translation() )
    {
        QMessageBox::warning( nullptr, "Error", "Position cannot be set to target point" );
        return;
    }
    m_camera->setPosition( position );
    m_referenceFrame.translation() = position + m_distFromCenter * m_camera->getDirection();

    updatePhiTheta();

    if ( m_light != nullptr )
    {
        m_light->setPosition( m_camera->getPosition() );
        m_light->setDirection( m_camera->getDirection() );
    }
}

void TrackballCameraManipulator::setCameraTarget( const Core::Vector3& target ) {
    if ( m_camera->getPosition() == m_referenceFrame.translation() )
    {
        QMessageBox::warning( nullptr, "Error", "Target cannot be set to current camera position" );
        return;
    }

    m_referenceFrame.translation() = target;

    m_camera->setDirection( ( target - m_camera->getPosition() ).normalized() );
    m_distFromCenter = ( target - m_camera->getPosition() ).norm();
    updatePhiTheta();

    if ( m_light != nullptr ) { m_light->setDirection( m_camera->getDirection() ); }
}

void TrackballCameraManipulator::fitScene( const Core::Aabb& aabb ) {

    Scalar f = m_camera->getFOV();
    Scalar a = m_camera->getAspect();

    const Scalar r = ( aabb.max() - aabb.min() ).norm() / 2_ra;
    const Scalar x = r / std::sin( f / 2_ra );
    const Scalar y = r / std::sin( f * a / 2_ra );
    Scalar d       = std::max( std::max( x, y ), 0.001_ra );

    m_camera->setFrame( Core::Transform::Identity() );
    Core::Vector3 camPos { aabb.center().x(), aabb.center().y(), aabb.center().z() + d };
    m_camera->setPosition( camPos );
    Core::Vector3 camDir { aabb.center() - camPos };
    m_distFromCenter = camDir.norm();
    m_camera->setDirection( camDir / m_distFromCenter );

    // no ref camera here, use wolrd frame to align with
    m_referenceFrame.setIdentity();
    m_referenceFrame.translation() = aabb.center();

    updatePhiTheta();

    if ( m_light != nullptr )
    {
        m_light->setPosition( m_camera->getPosition() );
        m_light->setDirection( m_camera->getDirection() );
    }
}

void TrackballCameraManipulator::handleCameraRotate( Scalar dx, Scalar dy ) {
    Scalar dphi   = m_phiDir * dx * m_cameraSensitivity * m_quickCameraModifier;
    Scalar dtheta = -dy * m_cameraSensitivity * m_quickCameraModifier;

    Scalar phi   = m_phi + dphi;
    Scalar theta = m_theta + dtheta;
    Core::Vector3 dir { std::sin( phi ) * std::sin( theta ),
                        std::cos( theta ),
                        std::cos( phi ) * std::sin( theta ) };

    Core::Vector3 right { -dir[2], 0, dir[0] };
    right.normalize();
    if ( ( m_referenceFrame.linear().inverse() * m_camera->getRightVector() ).dot( right ) < 0 )
        right = -right;

    Core::Vector3 up = dir.cross( right ).normalized();

    dir   = m_referenceFrame.linear() * dir;
    right = m_referenceFrame.linear() * right;
    up    = m_referenceFrame.linear() * up;

    Core::Matrix3 m;
    // clang-format off
    m << right[0], up[0], dir[0], //
         right[1], up[1], dir[1], //
         right[2], up[2], dir[2]; //
    // clang-format on
    Core::Transform t;

    t.setIdentity();
    t.linear()        = m;
    Core::Vector3 pos = m_referenceFrame.translation() + m_distFromCenter * dir;
    t.translation()   = pos;
    m_camera->setFrame( t );

    m_phi   = phi;
    m_theta = theta;

    clampThetaPhi();
}

void TrackballCameraManipulator::handleCameraPan( Scalar dx, Scalar dy ) {
    Scalar x = dx * m_cameraSensitivity * m_quickCameraModifier * m_distFromCenter * 0.1_ra;
    Scalar y = dy * m_cameraSensitivity * m_quickCameraModifier * m_distFromCenter * 0.1_ra;
    // Move camera and trackball center, keep the distance to the center
    Core::Vector3 R = -m_camera->getRightVector();
    Core::Vector3 U = m_camera->getUpVector();

    Core::Transform T( Core::Transform::Identity() );
    Core::Vector3 t = x * R + y * U;
    T.translate( t );

    m_camera->applyTransform( T );
    m_referenceFrame.translation() += t;
}

void TrackballCameraManipulator::handleCameraMoveForward( Scalar dx, Scalar dy ) {
    handleCameraMoveForward( Ra::Core::Math::sign( dy ) * Ra::Core::Vector2 { dx, dy }.norm() );
}

void TrackballCameraManipulator::handleCameraMoveForward( Scalar z ) {

    Scalar moveFactor = z * m_distFromCenter * m_cameraSensitivity * m_quickCameraModifier;

    Core::Transform T( Core::Transform::Identity() );
    T.translate( moveFactor * m_camera->getDirection() );

    m_camera->applyTransform( T );

    m_distFromCenter = ( m_referenceFrame.translation() - m_camera->getPosition() ).norm();
}

void TrackballCameraManipulator::handleCameraZoom( Scalar dx, Scalar dy ) {
    handleCameraZoom( Ra::Core::Math::sign( dy ) * Ra::Core::Vector2 { dx, dy }.norm() );
}

void TrackballCameraManipulator::handleCameraZoom( Scalar z ) {
    Scalar zoom = m_camera->getZoomFactor() - z * m_cameraSensitivity * m_quickCameraModifier;
    m_camera->setZoomFactor( zoom );
}

void TrackballCameraManipulator::updatePhiTheta() {
    using Core::Math::areApproxEqual;
    const Core::Vector3 R = m_referenceFrame.linear().inverse() * ( -m_camera->getDirection() );

    m_theta = std::acos( R.y() );

    // unlikely to have z and x to 0, unless direction is perfectly aligned with
    // m_referenceFrame.z() in this case phi is given by the relative orientation of right/up in the
    // z/x plane of m_reference frame.
    if ( UNLIKELY( areApproxEqual( R.z(), 0_ra ) && areApproxEqual( R.x(), 0_ra ) ) )
    {
        Scalar fx = m_referenceFrame.matrix().block<3, 1>( 0, 2 ).dot( m_camera->getRightVector() );
        Scalar fy = m_referenceFrame.matrix().block<3, 1>( 0, 2 ).dot( m_camera->getUpVector() );
        m_phi     = std::atan2( fx, fy );
    }
    else
    { m_phi = std::atan2( R.x(), R.z() ); }

    // no need to clamp, atan2 is by def \in [-pi,pi]
    // acos in [0, pi]
    // clampThetaPhi();
    CORE_ASSERT( std::isfinite( m_theta ) && std::isfinite( m_phi ), "Error in trackball camera" );
}

void TrackballCameraManipulator::clampThetaPhi() {
    // Keep phi between 0 and 2pi
    if ( m_phi < 0_ra ) { m_phi += 2_ra * Pi; }
    // Keep theta in [-pi, pi] (instead of [0,pi]) to allows scene flip
    if ( m_theta < -Pi ) { m_theta += 2_ra * Pi; }
    if ( m_theta > Pi ) { m_theta -= 2_ra * Pi; }
}

bool TrackballCameraManipulator::checkIntegrity( const std::string& mess ) const {
    Core::Vector3 c = m_camera->getPosition() + m_distFromCenter * m_camera->getDirection();
    Scalar d        = ( m_referenceFrame.translation() - c ).norm();
    if ( d > 0.001_ra )
    {
        LOG( logWARNING ) << "TrackballCameraManipulator Integrity problem : " << mess;
        LOG( logWARNING ) << "\t Position  " << m_camera->getPosition().transpose();
        LOG( logWARNING ) << "\t Ref       "
                          << ( m_referenceFrame.translation() +
                               m_distFromCenter * ( -m_camera->getDirection() ) )
                                 .transpose();
        LOG( logWARNING ) << "\t Direction " << m_camera->getDirection().transpose();
        LOG( logWARNING ) << "\t Center    " << c.transpose();
        LOG( logWARNING ) << "\t Distance  " << d;
        LOG( logWARNING ) << "\t angles    " << m_phi << " " << m_theta;
    }
    return d < 0.001_ra;
}

} // namespace Gui
} // namespace Ra
