#include <Core/Geometry/RayCast.hpp>
#include <Core/Math/Math.hpp>
#include <catch2/catch.hpp>

TEST_CASE( "Core/Geometry/RayCast", "[Core][Core/Geometry][RayCast]" ) {
    using namespace Ra::Core;
    Aabb ones( -Vector3::Ones(), Vector3::Ones() );
    // For all directions x, y, z
    for ( int i = 0; i < 3; ++i )
    {
        // From negative x and positive x
        for ( int sig = -1; sig < 2; sig += 2 )
        {
            Vector3 s = 3 * sig * Vector3::Unit( i );
            Vector3 d = -sig * Vector3::Unit( i );

            for ( int p = -10; p < 11; p++ )
            {
                for ( int q = -10; q < 11; q++ )
                {
                    Vector3 dir = d + Scalar( p ) / 10_ra * Vector3::Unit( ( i + 1 ) % 3 ) +
                                  Scalar( q ) / 10_ra * Vector3::Unit( ( i + 2 ) % 3 );

                    // Fire a ray towards the box (some hit, some miss).
                    {
                        Eigen::ParametrizedLine<Scalar, 3> r( s, dir.normalized() );

                        Scalar t          = 0_ra;
                        Vector3 n         = Vector3::Zero();
                        const bool result = Geometry::RayCastAabb( r, ones, t, n );

                        if ( std::abs( p ) <= 5 && std::abs( q ) <= 5 )
                        {
                            // The ray should have hit
                            REQUIRE( result );
                            // Wrong normal
                            REQUIRE(
                                Math::areApproxEqual( n.dot( sig * Vector3::Unit( i ) ), 1_ra ) );
                            // Wrong hit point
                            REQUIRE( Math::areApproxEqual( r.pointAt( t )[i], Scalar( sig ) ) );
                        }
                        else
                        { REQUIRE( !result ); } // The ray should have missed
                    }

                    // Fire a ray on the other direction (which should miss)
                    {
                        Eigen::ParametrizedLine<Scalar, 3> r( s, -dir.normalized() );

                        Scalar t;
                        Vector3 n;
                        const bool result = Geometry::RayCastAabb( r, ones, t, n );

                        // The ray should have missed (t<0)
                        REQUIRE( !result );
                    }

                    // Fire a ray from within the box.
                    {
                        Eigen::ParametrizedLine<Scalar, 3> r( Vector3::Zero(), dir.normalized() );

                        Scalar t;
                        Vector3 n { 0, 0, 0 };
                        const bool result = Geometry::RayCastAabb( r, ones, t, n );

                        // The ray should have hit (inside hit)
                        REQUIRE( result );
                        // Hit should be at origin
                        REQUIRE( Math::areApproxEqual( t, 0_ra ) );
                        // Wrong normal (inside hit)
                        REQUIRE( Math::areApproxEqual( n.dot( dir.normalized() ), -1_ra ) );
                    }
                }
            }
        }
    }
}
