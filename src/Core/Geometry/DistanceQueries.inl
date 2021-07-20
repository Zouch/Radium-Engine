#pragma once
#include <Core/Geometry/DistanceQueries.hpp>

#include <algorithm>

namespace Ra {
namespace Core {
namespace Geometry {
// Line funcs

inline RA_CORE_API Scalar pointToLineSq( const Vector3& q, const Vector3& a, const Vector3& dir ) {
    // http://mathworld.wolfram.com/Point-LineDistance3-Dimensional.html
    return ( dir.cross( q - a ) ).squaredNorm() / dir.squaredNorm();
}

inline RA_CORE_API Scalar projectOnSegment( const Vector3& q,
                                            const Vector3& a,
                                            const Vector3& ab ) {
    // Edge case : segment has length 0
    if ( UNLIKELY( ab.squaredNorm() == 0 ) ) { return 0; }
    return std::clamp( ( q - a ).dot( ab ) / ( ab.squaredNorm() ), Scalar( 0 ), Scalar( 1 ) );
}

inline RA_CORE_API Scalar pointToSegmentSq( const Vector3& q,
                                            const Vector3& a,
                                            const Vector3& ab ) {
    const Scalar t = projectOnSegment( q, a, ab );
    return ( q - ( a + t * ( ab ) ) ).squaredNorm();
}

// Triangle funcs
enum FlagsInternal {
    HIT_FACE = PointToTriangleOutput::HIT_FACE,

    HIT_A = 0x0 | PointToTriangleOutput::HIT_VERTEX,
    HIT_B = 0x4 | PointToTriangleOutput::HIT_VERTEX,
    HIT_C = 0x8 | PointToTriangleOutput::HIT_VERTEX,

    HIT_AB = 0x0 | PointToTriangleOutput::HIT_EDGE,
    HIT_BC = 0x4 | PointToTriangleOutput::HIT_EDGE,
    HIT_CA = 0x8 | PointToTriangleOutput::HIT_EDGE,
};

inline RA_CORE_API PointToTriangleOutput pointToTriSq( const Vector3& q,
                                                       const Vector3& a,
                                                       const Vector3& b,
                                                       const Vector3& c ) {
    /*
     *  This function projects the query point Q on the triangle plane to solve the
     *  planar problem described by the following schema  of Voronoi zones :
     * 3     /
     * --  C
     *     |\\     6
     *     |  \\
     * 5   |    \\     /
     *     |      \\  /
     * --  A ------ B    2
     *     |        |
     *  1  |   4    |
     *
     * It first checks if the nearest point is a vertex (zones 1,2,3), then if
     * it belongs to an edge (zone, 4,5,6). If not the nearest point lies on the
     * triangle.
     * Reference : Fast Distance Queries for Triangles, Lines and Points using SSE instructions
     * Evan Shellshear, Robin Ytterlid.
     * http://jcgt.org/published/0003/04/05/paper.pdf
     * Journal of Computer Graphics Techniques, Vol.3 No. 4 (2014)
     */

    PointToTriangleOutput output;

    const Vector3 ab = b - a;
    const Vector3 ac = c - a;
    const Vector3 aq = q - a;

    CORE_ASSERT( ab.cross( ac ).squaredNorm() > 0, "Triangle ABC is degenerate" );

    const Scalar d1 = ab.dot( aq );
    const Scalar d2 = ac.dot( aq );

    // Closest point is A. (zone 1)
    const bool m1 = d1 <= 0 && d2 <= 0;
    if ( m1 )
    {
        output.meshPoint       = a;
        output.distanceSquared = aq.squaredNorm();
        output.flags           = FlagsInternal::HIT_A;
        return output;
    }

    const Vector3 bq = q - b;
    const Scalar d3  = ab.dot( bq );
    const Scalar d4  = ac.dot( bq );

    // Closest point is B (zone 2)
    const bool m2 = d3 >= 0 && d4 <= d3;
    if ( m2 )
    {
        output.meshPoint       = b;
        output.distanceSquared = bq.squaredNorm();
        output.flags           = FlagsInternal::HIT_B;
        return output;
    }

    const Vector3 cq = q - c;
    const Scalar d5  = ab.dot( cq );
    const Scalar d6  = ac.dot( cq );

    // Closest point is C (zone 3)
    const bool m3 = ( d6 >= 0 && d5 <= d6 );
    if ( m3 )
    {
        output.meshPoint       = c;
        output.distanceSquared = cq.squaredNorm();
        output.flags           = FlagsInternal::HIT_C;
        return output;
    }

    const Scalar vc = d1 * d4 - d3 * d2;
    const Scalar v1 = d1 / ( d1 - d3 );

    // Closest point is on AB (zone 4)
    const bool m4 = vc <= 0 && d1 >= 0 && d3 <= 0;
    if ( m4 )
    {
        output.meshPoint       = a + v1 * ab;
        output.distanceSquared = ( output.meshPoint - q ).squaredNorm();
        output.flags           = FlagsInternal::HIT_AB;
        return output;
    }

    const Scalar vb = d5 * d2 - d1 * d6;
    const Scalar w1 = d2 / ( d2 - d6 );

    // Closest point is on AC (zone 5)
    const bool m5 = vb <= 0 && d2 >= 0 && d6 <= 0;
    if ( m5 )
    {
        output.meshPoint       = a + w1 * ac;
        output.distanceSquared = ( output.meshPoint - q ).squaredNorm();
        output.flags           = FlagsInternal::HIT_CA;
        return output;
    }

    const Scalar va = d3 * d6 - d5 * d4;
    const Scalar w2 = ( d4 - d3 ) / ( ( d4 - d3 ) + ( d5 - d6 ) );

    // Closest point is on BC (zone 6)
    const bool m6 = va <= 0 && ( d4 - d3 ) >= 0 && ( d5 - d6 ) >= 0;
    if ( m6 )
    {
        output.meshPoint       = b + w2 * ( c - b );
        output.distanceSquared = ( output.meshPoint - q ).squaredNorm();
        output.flags           = FlagsInternal::HIT_BC;
        return output;
    }

    // Closest point is on the triangle
    const Scalar d  = Scalar( 1 ) / ( va + vb + vc );
    const Scalar v2 = vb * d;
    const Scalar w3 = vc * d;

    output.meshPoint       = a + v2 * ab + w3 * ac;
    output.distanceSquared = ( output.meshPoint - q ).squaredNorm();
    output.flags           = 0;
    return output;
}

inline RA_CORE_API LineToSegmentOutput lineToSegSq( const Vector3& lineOrigin,
                                                    Vector3 lineDirection,
                                                    const Vector3& segCenter,
                                                    Vector3 segDirection,
                                                    const Scalar segExtent ) {
    // Reference : Geometric Tools,
    // https://github.com/pmjoniak/GeometricTools/blob/master/GTEngine/Include/GteDistLineSegment.h

    LineToSegmentOutput output;

    segDirection.normalize();
    lineDirection.normalize();

    Vector3 diff = lineOrigin - segCenter;
    Scalar a01   = -lineDirection.dot( segDirection );
    Scalar b0    = diff.dot( lineDirection );
    Scalar s0, s1;

    if ( std::abs( a01 ) < (Scalar)1 )
    {
        // The line and the segment are not parallel
        Scalar det    = (Scalar)1 - a01 * a01;
        Scalar extDet = segExtent * det;
        Scalar b1     = -diff.dot( segDirection );
        s1            = a01 * b0 - b1;

        if ( s1 >= -extDet )
        {
            if ( s1 <= extDet )
            {
                // 2 interior points are closest, one on the line and one on the segment
                s0 = ( a01 * b1 - b0 ) / det;
                s1 /= det;
            }
            else
            {
                // The endpoint e1 of the segment and an interior point of the line are closest
                s1 = segExtent;
                s0 = -( a01 * s1 + b0 );
            }
        }
        else
        {
            // The endpoint e0 of the segment and an interior point of the line are closest
            s1 = -segExtent;
            s0 = -( a01 * s1 + b0 );
        }
    }
    else
    {
        // The line and the segment are parallel
        // We choose the closest pair so that one point is at the segment origin
        s1 = (Scalar)0;
        s0 = -b0;
    }

    output.parameter[0]    = s0;
    output.parameter[1]    = s1;
    output.closestPoint[0] = lineOrigin + s0 * lineDirection;
    output.closestPoint[1] = segCenter + s1 * segDirection;
    diff                   = output.closestPoint[0] - output.closestPoint[1];
    output.sqrDistance     = diff.dot( diff );
    output.distance        = std::sqrt( output.sqrDistance );
    return output;
}

inline RA_CORE_API LineToTriangleOutput lineToTriSq( const Vector3& lineOrigin,
                                                     Vector3 lineDirection,
                                                     const Vector3 v[3] ) {
    // Reference : GeometricTools,
    // https://github.com/pmjoniak/GeometricTools/blob/master/GTEngine/Include/GteDistLine3Triangle3.h

    LineToTriangleOutput output;

    lineDirection.normalize();

    Vector3 edge0  = v[1] - v[0];
    Vector3 edge1  = v[2] - v[0];
    Vector3 normal = edge0.cross( edge1 );
    normal.normalize();
    Scalar nd = normal.dot( lineDirection );
    if ( std::abs( nd ) > (Scalar)0 )
    {
        // The line intersects the plane of the triangle or the triangle itself
        // (if the intersection point isn't needed, it possible to use the boolean function
        // vsTriangle of RayCast.hpp)
        Vector3 diff = lineOrigin - v[0];
        Vector3 basis[3]; // {D, U, V}
        basis[0] = lineDirection;
        if ( std::abs( basis[0][0] ) > std::abs( basis[0][1] ) )
        { basis[1] = { -basis[0][2], (Scalar)0, basis[0][0] }; }
        else
        { basis[1] = { (Scalar)0, basis[0][2], -basis[0][1] }; }
        basis[2] = basis[0].cross( basis[1] );
        // Orthonormalize basis
        // Normalize basis[0]
        basis[0] /= std::sqrt( basis[0].dot( basis[0] ) );
        basis[1] -= basis[0] * ( basis[1].dot( basis[0] ) );
        // Normalize basis[1]
        basis[1] /= std::sqrt( basis[1].dot( basis[1] ) );
        basis[2] -= basis[0] * ( basis[2].dot( basis[0] ) );
        basis[2] -= basis[1] * ( basis[2].dot( basis[1] ) );
        // Normalize basis[2]
        basis[2] /= std::sqrt( basis[2].dot( basis[2] ) );

        Scalar UdE0   = basis[1].dot( edge0 );
        Scalar UdE1   = basis[1].dot( edge1 );
        Scalar UdDiff = basis[1].dot( diff );
        Scalar VdE0   = basis[2].dot( edge0 );
        Scalar VdE1   = basis[2].dot( edge1 );
        Scalar VdDiff = basis[2].dot( diff );
        Scalar invDet = (Scalar)1 / ( UdE0 * VdE1 - UdE1 * VdE0 );

        // Barycentric coordinates for the point of intersection
        Scalar b1 = ( VdE1 * UdDiff - UdE1 * VdDiff ) * invDet;
        Scalar b2 = ( UdE0 * VdDiff - VdE0 * UdDiff ) * invDet;
        Scalar b0 = (Scalar)1 - b1 - b2;

        if ( b0 >= (Scalar)0 && b1 >= (Scalar)0 && b2 >= (Scalar)0 )
        {
            // The line intersects the triangle itself
            // Distance between the origin of the line and the intersection point with the triangle
            Scalar DdE0          = lineDirection.dot( edge0 );
            Scalar DdE1          = lineDirection.dot( edge1 );
            Scalar DdDiff        = lineDirection.dot( diff );
            output.lineParameter = b1 * DdE0 + b2 * DdE1 - DdDiff;

            // Barycentric coordinates for the point of intersection
            output.triangleParameter[0] = b0;
            output.triangleParameter[1] = b1;
            output.triangleParameter[2] = b2;

            // The intersection point is inside or on the triangle
            output.closestPoint[0] = lineOrigin + output.lineParameter * lineDirection;
            output.closestPoint[1] = v[0] + b1 * edge0 + b2 * edge1;

            output.distance    = (Scalar)0;
            output.sqrDistance = (Scalar)0;
            return output;
        }
    }

    // Either the line intersects the plane of the triangle but not the triangle itself, or the line
    // is parallel to the triangle. Regardless, the closest point on the triangle is on an edge of
    // the triangle, so we compare the line to all 3 edges of the triangle.
    output.distance    = std::numeric_limits<Scalar>::max();
    output.sqrDistance = std::numeric_limits<Scalar>::max();
    for ( int i0 = 2, i1 = 0; i1 < 3; i0 = i1++ )
    {
        Vector3 segCenter    = ( (Scalar)0.5 ) * ( v[i0] + v[i1] );
        Vector3 segDirection = v[i1] - v[i0];
        Scalar segExtent     = ( (Scalar)0.5 ) * std::sqrt( segDirection.dot( segDirection ) );

        // Distance line-segment is computed
        LineToSegmentOutput lsOutput =
            lineToSegSq( lineOrigin, lineDirection, segCenter, segDirection, segExtent );
        if ( lsOutput.sqrDistance < output.sqrDistance )
        {
            output.sqrDistance   = lsOutput.sqrDistance;
            output.distance      = lsOutput.distance;
            output.lineParameter = lsOutput.parameter[0];
            output.triangleParameter[i0] =
                ( (Scalar)0.5 ) * ( (Scalar)1 - lsOutput.parameter[0] / segExtent );
            output.triangleParameter[i1]          = (Scalar)1 - output.triangleParameter[i0];
            output.triangleParameter[3 - i0 - i1] = (Scalar)0;
            output.closestPoint[0]                = lsOutput.closestPoint[0];
            output.closestPoint[1]                = lsOutput.closestPoint[1];
        }
    }
    return output;
}

inline RA_CORE_API SegmentToTriangleOutput segmentToTriSq( const Vector3& segCenter,
                                                           Vector3 segDirection,
                                                           Scalar segExtent,
                                                           const Vector3 v[3] ) {
    // Reference : Geometric Tools,
    // https://github.com/pmjoniak/GeometricTools/blob/master/GTEngine/Include/GteDistSegment3Triangle3.h

    SegmentToTriangleOutput output;

    segDirection.normalize();

    // Distance line-triangle is computed
    LineToTriangleOutput ltOutput = lineToTriSq( segCenter, segDirection, v );

    if ( ltOutput.lineParameter >= -segExtent )
    {
        // The closest point on the line is on the segment or at its right
        if ( ltOutput.lineParameter <= segExtent )
        {
            // The closest point on the line is on the segment
            // The segment intersects the triangle
            output.distance             = ltOutput.distance;
            output.sqrDistance          = ltOutput.sqrDistance;
            output.segmentParameter     = ltOutput.lineParameter;
            output.triangleParameter[0] = ltOutput.triangleParameter[0];
            output.triangleParameter[1] = ltOutput.triangleParameter[1];
            output.triangleParameter[2] = ltOutput.triangleParameter[2];
            output.closestPoint[0]      = ltOutput.closestPoint[0];
            output.closestPoint[1]      = ltOutput.closestPoint[1];
        }
        else
        {
            // The closest point on the line is at the segment's right
            // We compute the distance between the right endpoint of the segment and the triangle
            Vector3 point                  = segCenter + segExtent * segDirection;
            PointToTriangleOutput ptOutput = pointToTriSq( point, v[0], v[1], v[2] );
            output.sqrDistance             = ptOutput.distanceSquared;
            output.distance                = std::sqrt( output.sqrDistance );
            output.segmentParameter        = segExtent;
            output.closestPoint[0]         = point;
            output.closestPoint[1]         = ptOutput.meshPoint;
        }
    }
    else
    {
        // The closest point on the line is at the segment's left
        // We compute the distance between the left endpoint of the segment and the triangle
        Vector3 point                  = segCenter - segExtent * segDirection;
        PointToTriangleOutput ptOutput = pointToTriSq( point, v[0], v[1], v[2] );
        output.sqrDistance             = ptOutput.distanceSquared;
        output.distance                = std::sqrt( output.sqrDistance );
        output.segmentParameter        = segExtent;
        output.closestPoint[0]         = point;
        output.closestPoint[1]         = ptOutput.meshPoint;
    }
    return output;
}

inline RA_CORE_API TriangleToTriangleOutput triangleToTriSq( const Vector3 v1[3],
                                                             const Vector3 v2[3] ) {
    TriangleToTriangleOutput output;

    SegmentToTriangleOutput stOutput;
    output.sqrDistance = std::numeric_limits<Scalar>::max();

    // We compute the closest distance between v1's edges and v2.
    for ( int i0 = 2, i1 = 0; i1 < 3; i0 = i1++ )
    {
        Vector3 segCenter    = ( (Scalar)0.5 ) * ( v1[i0] + v1[i1] );
        Vector3 segDirection = v1[i1] - v1[i0];
        Scalar segExtent     = ( (Scalar)0.5 ) * std::sqrt( segDirection.dot( segDirection ) );

        stOutput = segmentToTriSq( segCenter, segDirection, segExtent, v2 );
        if ( stOutput.sqrDistance < output.sqrDistance )
        {
            output.sqrDistance = stOutput.sqrDistance;
            output.distance    = stOutput.distance;
            output.triangleParameter1[i0] =
                ( (Scalar)0.5 ) * ( (Scalar)1 - stOutput.segmentParameter / segExtent );
            output.triangleParameter1[i1]          = (Scalar)1 - output.triangleParameter1[i0];
            output.triangleParameter1[3 - i0 - i1] = (Scalar)0;
            output.triangleParameter2[0]           = stOutput.triangleParameter[0];
            output.triangleParameter2[1]           = stOutput.triangleParameter[1];
            output.triangleParameter2[2]           = stOutput.triangleParameter[2];
            output.closestPoint[0]                 = stOutput.closestPoint[0];
            output.closestPoint[1]                 = stOutput.closestPoint[1];
        }
    }

    // We compute the closest distance between v2's edges and v1.
    for ( int i0 = 2, i1 = 0; i1 < 3; i0 = i1++ )
    {
        Vector3 segCenter    = ( (Scalar)0.5 ) * ( v2[i0] + v2[i1] );
        Vector3 segDirection = v2[i1] - v2[i0];
        Scalar segExtent     = ( (Scalar)0.5 ) * std::sqrt( segDirection.dot( segDirection ) );

        stOutput = segmentToTriSq( segCenter, segDirection, segExtent, v1 );
        if ( stOutput.sqrDistance < output.sqrDistance )
        {
            output.sqrDistance           = stOutput.sqrDistance;
            output.distance              = stOutput.distance;
            output.triangleParameter1[0] = stOutput.triangleParameter[0];
            output.triangleParameter1[1] = stOutput.triangleParameter[1];
            output.triangleParameter1[2] = stOutput.triangleParameter[2];
            output.triangleParameter2[i0] =
                ( (Scalar)0.5 ) * ( (Scalar)1 - stOutput.segmentParameter / segExtent );
            output.triangleParameter2[i1]          = (Scalar)1 - output.triangleParameter2[i0];
            output.triangleParameter2[3 - i0 - i1] = (Scalar)0;
            output.closestPoint[0]                 = stOutput.closestPoint[0];
            output.closestPoint[1]                 = stOutput.closestPoint[1];
        }
    }
    return output;
}

} // namespace Geometry
} // namespace Core
} // namespace Ra
