#include <Core/Geometry/PolyLine.hpp>
#include <catch2/catch.hpp>

TEST_CASE( "Core/Geometry/Polyline", "[Core][Core/Geometry][Polyline]" ) {
    using namespace Ra::Core;
    SECTION( "2 points polyline" ) {
        Vector3Array v2 { { 1, 2, 3 }, { 4, 5, 6 } };
        Geometry::PolyLine p( v2 );

        Vector3 m = 0.5_ra * ( v2[0] + v2[1] );

        // Parametrization fail
        REQUIRE( p.f( 0_ra ).isApprox( v2[0] ) );
        REQUIRE( p.f( 1_ra ).isApprox( v2[1] ) );
        REQUIRE( p.f( .5_ra ).isApprox( m ) );
        // Distance fail
        REQUIRE( Math::areApproxEqual( p.distance( m ), 0_ra ) );
    }
    SECTION( "4 points polyline" ) {
        Vector3Array v4 { { 2, 3, 5 }, { 7, 11, 13 }, { 17, 23, 29 }, { -1, -1, 30 } };
        Geometry::PolyLine p( v4 );

        // Parametrization fail
        REQUIRE( p.f( 0_ra ).isApprox( v4[0] ) );
        REQUIRE( p.f( 1_ra ).isApprox( v4[3] ) );
        // Parametrization clamp fail
        REQUIRE( p.f( -10_ra ).isApprox( p.f( 0_ra ) ) );
        REQUIRE( p.f( 10_ra ).isApprox( p.f( 1_ra ) ) );

        for ( const auto& x : v4 )
        { REQUIRE( Math::areApproxEqual( p.distance( x ), 0_ra ) ); }
    }
}
