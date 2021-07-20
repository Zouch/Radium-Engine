#include <Core/Asset/Camera.hpp>
#include <Core/Math/Math.hpp>
#include <Engine/Scene/CameraComponent.hpp>

#include <catch2/catch.hpp>

using namespace Ra::Core;
using namespace Ra::Core::Utils;
using namespace Ra::Core::Asset;

void testProjectUnproject( const Camera& cam, const Vector3& x, const Vector2& ref ) {
    std::cout << "x" << x.transpose() << " ref " << ref.transpose() << "\n";
    Vector3 p = cam.projectToScreen( x );
    std::cout << "p " << p.transpose() << "\n";
    if ( ref.head<2>().norm() > 0_ra )
        REQUIRE( p.head<2>().isApprox( ref ) );
    else
        REQUIRE( Math::areApproxEqual( p.head<2>().norm(), 0_ra ) );

    Vector3 q = cam.unProjectFromScreen( p );
    std::cout << "q " << q.transpose() << "\n";
    if ( x.norm() > 0_ra )
        REQUIRE( q.isApprox( x ) );
    else
        REQUIRE( Math::areApproxEqual( q.norm(), 0_ra ) );

    Vector3 r = cam.unProjectFromScreen( Vector2 { p.head<2>() } );
    std::cout << "r " << r.transpose() << "\n";

    Vector3 rRef = ( x - cam.getPosition() );
    std::cout << "rref " << rRef.transpose() << "\n";

    Scalar ratio = cam.getZNear() / cam.getDirection().dot( rRef );
    rRef *= ratio;
    rRef += cam.getPosition();
    std::cout << "rref " << rRef.transpose() << "\n";
    if ( rRef.norm() > 0_ra )
        REQUIRE( r.isApprox( rRef ) );
    else
        REQUIRE( Math::areApproxEqual( r.norm(), 0_ra ) );
}

TEST_CASE( "Core/Camera" ) {

    auto cam = std::make_unique<Camera>( 10, 20 );
    REQUIRE( cam->getWidth() == 10 );
    REQUIRE( cam->getHeight() == 20 );
    REQUIRE( Math::areApproxEqual( cam->getAspect(), 10_ra / 20_ra ) );
    REQUIRE( Math::areApproxEqual( cam->getZNear(), 0.1_ra ) );
    REQUIRE( Math::areApproxEqual( cam->getZFar(), 1000_ra ) );

    Vector3 dir { 1, 1, 1 };
    Vector3 pos { 0, 0, 0 };
    cam->setPosition( pos );
    cam->setDirection( dir );

    REQUIRE( dir.normalized().isApprox( cam->getDirection() ) );

    SECTION( "copy" ) {
        Camera cam2;
        cam2.setViewport( 200, 300 );
        REQUIRE( cam2.getWidth() == 200 );
        REQUIRE( cam2.getHeight() == 300 );
        REQUIRE( Math::areApproxEqual( cam2.getAspect(), 200_ra / 300_ra ) );

        cam2 = *cam;

        REQUIRE( Math::areApproxEqual( cam2.getZNear(), 0.1_ra ) );
        REQUIRE( Math::areApproxEqual( cam2.getZFar(), 1000_ra ) );
        REQUIRE( dir.normalized().isApprox( cam2.getDirection() ) );
        REQUIRE( cam2.getWidth() == 10 );
        REQUIRE( cam2.getHeight() == 20 );
        REQUIRE( Math::areApproxEqual( cam2.getAspect(), 10_ra / 20_ra ) );
        auto xymag  = cam->getXYmag();
        auto xymag2 = cam2.getXYmag();
        REQUIRE( Math::areApproxEqual( xymag.first, xymag2.first ) );
        REQUIRE( Math::areApproxEqual( xymag.second, xymag2.second ) );
    }

    SECTION( "setters" ) {
        Ra::Core::Vector3 newDir { 2, 3, 4 };
        cam->setDirection( newDir );
        REQUIRE( newDir.normalized().isApprox( cam->getDirection() ) );
        // it's still an orthonormal basis
        REQUIRE( Math::areApproxEqual( cam->getDirection().dot( cam->getUpVector() ), 0_ra ) );
        REQUIRE( Math::areApproxEqual( cam->getDirection().dot( cam->getRightVector() ), 0_ra ) );
        REQUIRE( Math::areApproxEqual( cam->getUpVector().dot( cam->getRightVector() ), 0_ra ) );

        Ra::Core::Vector3 newPos { 2, 3, 4 };
        cam->setPosition( newPos );
        REQUIRE( newPos.isApprox( cam->getPosition() ) );

        REQUIRE( Math::areApproxEqual( cam->getDirection().dot( cam->getUpVector() ), 0_ra ) );
        REQUIRE( Math::areApproxEqual( cam->getDirection().dot( cam->getRightVector() ), 0_ra ) );
        REQUIRE( Math::areApproxEqual( cam->getUpVector().dot( cam->getRightVector() ), 0_ra ) );

        Ra::Core::Vector3 newUp { 2, 3, 4 };
        cam->setUpVector( newUp );
        REQUIRE( newUp.normalized().isApprox( cam->getUpVector() ) );

        REQUIRE( Math::areApproxEqual( cam->getDirection().dot( cam->getUpVector() ), 0_ra ) );
        REQUIRE( Math::areApproxEqual( cam->getDirection().dot( cam->getRightVector() ), 0_ra ) );
        REQUIRE( Math::areApproxEqual( cam->getUpVector().dot( cam->getRightVector() ), 0_ra ) );

        Ra::Core::Vector3 oppositeDir = -cam->getDirection();
        Ra::Core::Vector3 oldUp       = cam->getUpVector();

        cam->setDirection( oppositeDir );

        REQUIRE( oppositeDir.normalized().isApprox( cam->getDirection() ) );
        // it's still an orthonormal basis
        REQUIRE( Math::areApproxEqual( cam->getDirection().dot( cam->getUpVector() ), 0_ra ) );
        REQUIRE( Math::areApproxEqual( cam->getDirection().dot( cam->getRightVector() ), 0_ra ) );
        REQUIRE( Math::areApproxEqual( cam->getUpVector().dot( cam->getRightVector() ), 0_ra ) );
        // and up do not have changed
        REQUIRE( oldUp.isApprox( cam->getUpVector() ) );

        cam->setZNear( 10_ra );
        REQUIRE( Math::areApproxEqual( cam->getZNear(), 10_ra ) );
        cam->setZFar( 100_ra );
        REQUIRE( Math::areApproxEqual( cam->getZFar(), 100_ra ) );
        REQUIRE( cam->getType() == Camera::ProjType::PERSPECTIVE );
        cam->setType( Camera::ProjType::ORTHOGRAPHIC );
        REQUIRE( cam->getType() == Camera::ProjType::ORTHOGRAPHIC );
        cam->updateProjMatrix();
        // aspect is 10/20, near 10, far 100
        Ra::Core::Matrix4 ref;
        ref << 1_ra, 0_ra, 0_ra, 0_ra, 0_ra, 0.5_ra, 0_ra, 0_ra, 0_ra, 0_ra, -2_ra / 90_ra,
            -110_ra / 90_ra, 0_ra, 0_ra, 0_ra, 1_ra;
        REQUIRE( cam->getProjMatrix().isApprox( ref ) );

        // reset frame
        cam->setFrame( Transform::Identity() );
        REQUIRE( cam->getFrame().isApprox( Transform::Identity() ) );
        REQUIRE( cam->getViewMatrix().isApprox( Matrix4::Identity() ) );

        // reset proj
        cam->setProjMatrix( Matrix4::Identity() );
        REQUIRE( cam->getProjMatrix().isApprox( Matrix4::Identity() ) );

        // and set fov
        cam->setFOV( Math::PiDiv2 );
        REQUIRE( Math::areApproxEqual( cam->getFOV(), Math::PiDiv2 ) );
        cam->setFOV( Math::PiDiv4 );
        REQUIRE( Math::areApproxEqual( cam->getFOV(), Math::PiDiv4 ) );

        // set x and y mag
        cam->setXYmag( 1.414_ra, 1.732_ra );
        auto xymag = cam->getXYmag();
        REQUIRE( Math::areApproxEqual( xymag.first, 1.414_ra ) );
        REQUIRE( Math::areApproxEqual( xymag.second, 1.732_ra ) );

        REQUIRE( Math::areApproxEqual( cam->getZoomFactor(), 1_ra ) );
        cam->setZoomFactor( 2_ra );
        REQUIRE( Math::areApproxEqual( cam->getZoomFactor(), 2_ra ) );
    }

    SECTION( "fit aabb" ) {
        Ra::Core::Aabb aabb { Ra::Core::Vector3 { 3, 3, 3 }, Ra::Core::Vector3 { 6, 6, 6 } };
        cam->fitZRange( aabb );
        REQUIRE(
            ( Math::areApproxEqual( cam->getZNear(), ( cam->getPosition() - aabb.min() ).norm() ) ||
              cam->getZNear() == cam->m_minZNear ) );
        REQUIRE( Math::areApproxEqual(
            cam->getZFar(), ( cam->getPosition() - aabb.max() ).norm(), 16_ra ) );
        REQUIRE( ( cam->getZNear() > cam->m_minZNear ||
                   Math::areApproxEqual( cam->getZNear(), cam->m_minZNear ) ) );
        REQUIRE(
            ( ( cam->getZFar() - cam->getZNear() ) > cam->m_minZRange ||
              Math::areApproxEqual( ( cam->getZFar() - cam->getZNear() ), cam->m_minZRange ) ) );

        aabb = { Ra::Core::Vector3 { -1, -1, -1 }, Ra::Core::Vector3 { 1, 1, 1 } };
        cam->fitZRange( aabb );
        REQUIRE( Math::areApproxEqual( cam->getZNear(), cam->m_minZNear ) );
        REQUIRE(
            ( Math::areApproxEqual( cam->getZFar(), ( cam->getPosition() - aabb.max() ).norm() ) ||
              Math::areApproxEqual( cam->getZFar() - cam->getZNear(), cam->m_minZRange ) ) );
        REQUIRE( ( cam->getZNear() > cam->m_minZNear ||
                   Math::areApproxEqual( cam->getZNear(), cam->m_minZNear ) ) );
        REQUIRE(
            ( ( cam->getZFar() - cam->getZNear() ) > cam->m_minZRange ||
              Math::areApproxEqual( ( cam->getZFar() - cam->getZNear() ), cam->m_minZRange ) ) );

        aabb = { Ra::Core::Vector3 { -2, -1, 1 }, Ra::Core::Vector3 { -4, -1, 0 } };
        cam->fitZRange( aabb );
        REQUIRE( Math::areApproxEqual( cam->getZNear(), cam->m_minZNear ) );
        REQUIRE( Math::areApproxEqual( cam->getZFar(), cam->getZNear() + cam->m_minZRange ) );

        aabb = { Ra::Core::Vector3 { 0, -3, 0 }, Ra::Core::Vector3 { 1, -2, 1 } };
        cam->fitZRange( aabb );
        REQUIRE( Math::areApproxEqual( cam->getZNear(), cam->m_minZNear ) );
        REQUIRE( Math::areApproxEqual( cam->getZFar(), cam->getZNear() + cam->m_minZRange ) );
    }

    SECTION( "projection" ) {
        auto n      = 1_ra;
        auto f      = 10_ra;
        auto fov    = Math::Pi / 4_ra;
        auto scale  = std::tan( fov * .5_ra ) * n;
        auto aspect = 2_ra;

        auto frustum = Camera::frustum( -scale, +scale, -scale / aspect, +scale / aspect, n, f );
        REQUIRE( frustum.allFinite() );
        auto perspective = Camera::perspective( aspect, fov, n, f );
        REQUIRE( perspective.allFinite() );

        REQUIRE( frustum.isApprox( perspective ) );

        Camera projection { 10_ra, 20_ra };
        projection.setFOV( fov );
        projection.setZNear( 1_ra );
        projection.setZFar( 10_ra );
        projection.setPosition( { 0_ra, 0_ra, 2_ra } );
        projection.setDirection( { 0_ra, 0_ra, -1_ra } );
        projection.updateProjMatrix();

        Vector3 x { 0_ra, 0_ra, 0_ra };

        testProjectUnproject( projection, x, Vector2 { 5_ra, 10_ra } );
        x = { 0_ra, 0_ra, -3_ra };
        testProjectUnproject( projection, x, Vector2 { 5_ra, 10_ra } );
        x = { -std::tan( Math::Pi / 8_ra ), 2_ra * std::tan( Math::Pi / 8_ra ), 1_ra };
        testProjectUnproject( projection, x, Vector2 { 0_ra, 0_ra } );
        x = { +std::tan( Math::Pi / 8_ra ), -2_ra * std::tan( Math::Pi / 8_ra ), 1_ra };
        testProjectUnproject( projection, x, Vector2 { 10_ra, 20_ra } );
    }
}
