#include <Core/Geometry/DistanceQueries.hpp>
#include <Core/Math/LinearAlgebra.hpp> // Math::getOrthogonalVectors
#include <Core/Math/Math.hpp>          //  Math::areApproxEqual
#include <catch2/catch.hpp>

TEST_CASE( "Core/Geometry/DistanceQueries", "[Core][Core/Geometry][DistanceQueries]" ) {

    using namespace Ra::Core;
    SECTION( "Simple tests" ) {
        Vector3 a( -2.f, 0.f, 0.f );
        Vector3 b( 2.f, 0.f, 0.f );
        Vector3 c( 0.f, 3.f, 0.f );
        Vector3 d( 0.f, -3.f, 0.f );
        Vector3 e( -4.f, -3.f, 0.f );
        Vector3 f( 4.f, 3.f, 0.f );

        // Test point to triangle query
        Scalar distPointToTri = std::sqrt( Geometry::pointToTriSq( c, a, b, d ).distanceSquared );
        // Distance point to triangle
        REQUIRE( Math::areApproxEqual( distPointToTri, ( c - ( .5_ra * ( a + b ) ) ).norm() ) );

        // Test line to segment distance query
        const Vector3& lineOrigin = a;
        Vector3 lineDirection     = d - a;
        const Vector3& segCenter  = .5_ra * ( c + b );
        Vector3 segDirection      = b - c;
        Scalar segExtent          = .5_ra * std::sqrt( ( b - c ).dot( b - c ) );
        Scalar distLineToSeg =
            Geometry::lineToSegSq( lineOrigin, lineDirection, segCenter, segDirection, segExtent )
                .sqrDistance;
        REQUIRE( Math::areApproxEqual( distLineToSeg, Geometry::pointToSegmentSq( a, c, b - c ) ) );

        // Test line to triangle distance query
        Vector3 v[3]         = { a, d, e };
        Scalar distLineToTri = Geometry::lineToTriSq( segCenter, segDirection, v ).sqrDistance;
        REQUIRE( Math::areApproxEqual( distLineToTri, Geometry::pointToSegmentSq( a, c, b - c ) ) );

        // Test segment to triangle distance query
        Scalar distSegToTri =
            Geometry::segmentToTriSq( segCenter, segDirection, segExtent, v ).sqrDistance;
        REQUIRE( Math::areApproxEqual( distSegToTri, Geometry::pointToSegmentSq( a, c, b - c ) ) );

        // Test triangle to triangle distance query
        Vector3 v2[3]       = { c, f, b };
        Scalar distTriToTri = Geometry::triangleToTriSq( v, v2 ).sqrDistance;
        REQUIRE( Math::areApproxEqual( distTriToTri, Geometry::pointToSegmentSq( a, c, b - c ) ) );
    }

    Vector3 a( 1_ra, 2.3_ra, 4.5_ra );
    Vector3 b( -6_ra, 7_ra, 8.9_ra );
    Vector3 c( -3_ra, 12.3_ra, -42.1_ra );

    // Midpoint.
    Vector3 m = 0.5_ra * ( a + b );

    // Point on the line, before A
    Vector3 na = a - 12_ra * ( b - a );

    // Point on the line after B
    Vector3 nb = a + 42_ra * ( b - a );

    Vector3 y, z;
    Math::getOrthogonalVectors( ( b - a ).normalized(), y, z );

    SECTION( "Test line queries" ) {
        // distance from A to AB
        REQUIRE( Math::areApproxEqual( Geometry::pointToLineSq( a, a, b - a ), 0_ra ) );
        // distance from B to AB
        REQUIRE( Math::areApproxEqual( Geometry::pointToLineSq( b, a, b - a ), 0_ra ) );
        // point on the line
        REQUIRE( Math::areApproxEqual( Geometry::pointToLineSq( na, a, b - a ), 0_ra ) );
        REQUIRE( Math::areApproxEqual( Geometry::pointToLineSq( nb, a, b - a ), 0_ra ) );
        // point perpendicular to segment.
        REQUIRE(
            Math::areApproxEqual( Geometry::pointToLineSq( m + y, a, b - a ), y.squaredNorm() ) );
    }

    SECTION( "Test segment queries" ) {
        // segment extremity
        REQUIRE( Math::areApproxEqual( Geometry::pointToSegmentSq( a, a, b - a ), 0_ra ) );
        REQUIRE( Math::areApproxEqual( Geometry::pointToSegmentSq( b, a, b - a ), 0_ra ) );
        // point on the line
        REQUIRE( Math::areApproxEqual( Geometry::pointToSegmentSq( na, a, b - a ),
                                       ( na - a ).squaredNorm() ) );
        REQUIRE( Math::areApproxEqual( Geometry::pointToSegmentSq( nb, a, b - a ),
                                       ( nb - b ).squaredNorm() ) );
        // point perpendicular to segment
        REQUIRE( Math::areApproxEqual( Geometry::pointToSegmentSq( m + y, a, b - a ),
                                       y.squaredNorm() ) );
    }

    SECTION( "Triangle queries: Test that each vertex returns itself" ) {
        // distance from A to ABC
        auto da = Geometry::pointToTriSq( a, a, b, c );
        REQUIRE( Math::areApproxEqual( da.distanceSquared, 0_ra ) );
        REQUIRE( da.meshPoint == a );
        REQUIRE( da.flags == Geometry::FlagsInternal::HIT_A );

        // distance from B to ABC
        auto db = Geometry::pointToTriSq( b, a, b, c );
        REQUIRE( Math::areApproxEqual( db.distanceSquared, 0_ra ) );
        REQUIRE( db.meshPoint == b );
        REQUIRE( db.flags == Geometry::FlagsInternal::HIT_B );

        // distance from C to ABC
        auto dc = Geometry::pointToTriSq( c, a, b, c );
        REQUIRE( Math::areApproxEqual( dc.distanceSquared, 0_ra ) );
        REQUIRE( dc.meshPoint == c );
        REQUIRE( dc.flags == Geometry::FlagsInternal::HIT_C );
    }

    SECTION( "Triangle queries: Test midpoints of edges" ) {
        Vector3 mab = .5_ra * ( a + b );
        Vector3 mac = .5_ra * ( a + c );
        Vector3 mbc = .5_ra * ( b + c );

        // Distance from AB midpoint to ABC
        auto dmab = Geometry::pointToTriSq( mab, a, b, c );
        REQUIRE( Math::areApproxEqual( dmab.distanceSquared, 0_ra ) );
        REQUIRE( dmab.meshPoint.isApprox( mab ) );
        REQUIRE( dmab.flags == Geometry::FlagsInternal::HIT_AB );

        // Distance from AC midpoint to ABC
        auto dmac = Geometry::pointToTriSq( mac, a, b, c );
        REQUIRE( Math::areApproxEqual( dmac.distanceSquared, 0_ra ) );
        REQUIRE( dmac.meshPoint.isApprox( mac ) );
        REQUIRE( dmac.flags == Geometry::FlagsInternal::HIT_CA );

        // Distance from BC midpoint to ABC
        auto dmbc = Geometry::pointToTriSq( mbc, a, b, c );
        REQUIRE( Math::areApproxEqual( dmbc.distanceSquared, 0_ra ) );
        REQUIRE( dmbc.meshPoint.isApprox( mbc ) );
        REQUIRE( dmbc.flags == Geometry::FlagsInternal::HIT_BC );
    }

    SECTION( "Triangle queries: Test point inside the triangle" ) {
        Vector3 g = ( 1_ra / 3_ra ) * ( a + b + c );

        auto dg = Geometry::pointToTriSq( g, a, b, c );
        // Distance from centroid to ABC
        REQUIRE( Math::areApproxEqual( dg.distanceSquared, 0_ra ) );
        REQUIRE( dg.meshPoint.isApprox( g ) );
        REQUIRE( dg.flags == Geometry::FlagsInternal::HIT_FACE );
    }
}
