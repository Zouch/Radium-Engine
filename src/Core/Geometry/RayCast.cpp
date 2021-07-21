#include <Core/Geometry/RayCast.hpp>
#include <Core/Geometry/TriangleMesh.hpp>
#include <Core/Math/LinearAlgebra.hpp> // Math::sign

namespace Ra {
namespace Core {
// useful : http://www.realtimerendering.com/intersections.html
namespace Geometry {
/// Intersect a ray with an axis-aligned bounding box.
bool RayCastAabb( const Ray& r, const Core::Aabb& aabb, Scalar& hitOut, Vector3& normalOut ) {
    // Based on optimized Woo version (ray vs 3 slabs)
    // Ref : Graphics Gems p.395
    // http://www.codercorner.com/RayAABB.cpp

    CORE_ASSERT( r.direction().squaredNorm() > 0.f, "Invalid Ray" );
    CORE_ASSERT( !aabb.isEmpty(), "Empty AABB" ); // Or return false ?

    // Vector of bool telling which components of the direction are not 0;
    auto nEqualZero = r.direction().array() != Core::Vector3::Zero().array();

    // Vector of bool telling which components of the ray origin are respectively
    // smaller than the aabb min or higher than aabb max.
    auto infMin = r.origin().array() < aabb.min().array();
    auto supMax = r.origin().array() > aabb.max().array();

    // Get rid of the case where the origin of the ray is inside the box
    if ( !( infMin.any() ) && !( supMax.any() ) ) {
        hitOut    = 0;
        normalOut = -r.direction();
        return true;
    }

    // Precompute the t values for each plane.
    const Core::Vector3 invDir  = r.direction().cwiseInverse();
    const Core::Vector3 minOrig = ( aabb.min() - r.origin() ).cwiseProduct( invDir );
    const Core::Vector3 maxOrig = ( aabb.max() - r.origin() ).cwiseProduct( invDir );

    // The following Eigen dark magic should be equivalent to this pseudo code
    // For  i = 1..3
    // if (direction[i] !=0)
    //    if (origin[i] < aabb.min) then maxT[i] = (min[i] - origin[i]) / direction[i])
    //    else if (origin[i] > aabb.max) then maxT[i] = (max[i] - origin[i]) / direction[i]

    auto maxT = nEqualZero.select(
        infMin.select( minOrig.array(),
                       supMax.select( maxOrig.array(), -std::numeric_limits<Scalar>::max() ) ),
        -std::numeric_limits<Scalar>::max() );

    // Find candidate t.
    uint i;
    const Scalar t = maxT.maxCoeff( &i );

    // Check if the point is actually in the box's face.
    const Vector3 p = r.pointAt( t );
    const Vector3 s = Vector3::Unit( i );

    auto inFace =
        s.select( 0, ( p.array() < aabb.min().array() || p.array() > aabb.max().array() ) );

    // Ignore negative t (box behind the origin), and points outside the aabb.
    if ( t >= 0 && !inFace.any() ) {
        hitOut    = t;
        normalOut = -s * Math::sign( r.direction()[i] );
        return true;
    }
    return false;
}

bool RayCastSphere( const Ray& r,
                    const Core::Vector3& center,
                    Scalar radius,
                    std::vector<Scalar>& hitsOut ) {

    CORE_ASSERT( r.direction().squaredNorm() > 0.f, "Invalid Ray" );
    CORE_ASSERT( radius > 0.f, "Invalid radius" );

    // Solve a 2nd degree eqn. in t
    // X = ray.origin + t* ray.direction
    // ||X - center|| = radius.

    const Core::Vector3 co = r.origin() - center;
    const Scalar co2       = co.squaredNorm();
    const Scalar dirDotCO  = r.direction().dot( co );

    // t is one solution of at^2 + bt + c = 0;
    // with a = || direction || ^2 = 1.f (rays are normalized in Eigen)
    const Scalar c = co2 - ( radius * radius );
    const Scalar b = 2.f * dirDotCO;

    const Scalar delta = ( b * b ) - ( 4.f * c );

    if ( delta == 0.f ) {
        const Scalar t       = -b * 0.5f;
        const bool tPositive = ( t >= 0.f );
        if ( tPositive ) { hitsOut.push_back( t ); }
        return tPositive;
    }
    else if ( delta > 0.f ) {
        const Scalar t1 = ( -b - std::sqrt( delta ) ) * 0.5f;
        const Scalar t2 = ( -b + std::sqrt( delta ) ) * 0.5f;

        // We know this because a is > 0;
        CORE_ASSERT( t1 < t2, "Your math is wrong." );

        const bool t1Positive = ( t1 >= 0.f );
        const bool t2Positive = ( t2 >= 0.f );
        if ( t1Positive ) { hitsOut.push_back( t1 ); }
        if ( t2Positive ) { hitsOut.push_back( t2 ); }

        return ( t1Positive || t2Positive );
    }
    return false;
}

bool RayCastPlane( const Ray& r,
                   const Core::Vector3 a,
                   const Core::Vector3& normal,
                   std::vector<Scalar>& hitsOut ) {
    CORE_ASSERT( r.direction().squaredNorm() > 0.f, "Invalid Ray" );
    CORE_ASSERT( normal.squaredNorm() > 0.f, "Invalid plane normal" );

    // Solve for t the first order eqn.
    // P = O + t. d
    // AP . n =  0
    // gives t = (d.n / OA.n)

    const Scalar ddotn  = r.direction().dot( normal );
    const Scalar OAdotn = ( a - r.origin() ).dot( normal );

    // If d.n is non zero, the line intersects the plane.
    // we check that the ray intersects for t>=0 by checking that d.n and OA.n have the same sign.
    if ( ddotn != 0 && ( ddotn * OAdotn ) >= 0 ) {
        hitsOut.push_back( OAdotn / ddotn );
        return true;
    }
    // If d.n is 0 the ray is parallel to the plane, so there is only an intersection
    // if the ray is completely in the plane (i.e. if OA.n = 0).
    else if ( ddotn == 0 && OAdotn == 0 ) {
        hitsOut.push_back( 0 );
        return true;
    }

    return false;
}

// TODO : this needs serious optimizing if we want it fast :p
bool RayCastCylinder( const Ray& r,
                      const Core::Vector3& a,
                      const Core::Vector3& b,
                      Scalar radius,
                      std::vector<Scalar>& hitsOut ) {
    /// Ref : Graphics Gem IV.
    /// http://www.realtimerendering.com/resources/GraphicsGems//gemsiv/ray_cyl.c
    const Scalar radiusSquared = radius * radius;

    const Core::Vector3 cylAxis = b - a;
    const Core::Vector3 ao      = r.origin() - a;

    // Intersect the ray against plane A and B.
    std::vector<Scalar> hitsA;
    const bool vsA    = RayCastPlane( r, a, cylAxis, hitsA );
    const Scalar hitA = vsA ? hitsA[0] : -1.f;

    std::vector<Scalar> hitsB;
    const bool vsB    = RayCastPlane( r, b, cylAxis, hitsB );
    const Scalar hitB = vsB ? hitsB[0] : -1.f;

    auto n = r.direction().cross( cylAxis );
    // Degenerated case : cylinder axis parallel to ray.
    if ( UNLIKELY( n.squaredNorm() == 0 ) ) {
        // Distance between two parallel lines.
        const Scalar distSquared =
            ao.cross( r.direction() ).squaredNorm() / r.direction().squaredNorm();

        // Is the ray inside the cylinder ?
        if ( distSquared <= ( radiusSquared ) ) {
            // In this case we must hit at least one of the planes
            CORE_ASSERT( vsA || vsB, "Ray must hit at least one of the planes !" );

            // The most common case is that both plane are hits (i.e. the ray's origin is outside
            // the cylinder). In that case we just return the smallest hit.
            if ( LIKELY( vsA && vsB ) ) {
                CORE_ASSERT( std::min( hitA, hitB ) >= 0, "Invalid hit result" );
                hitsOut.push_back( std::min( hitA, hitB ) );
                return true;
            }

            // If only one plane is hit, then the ray is inside the cylinder, so we return the ray
            // origin as the result point.
            hitsOut.push_back( 0 );
            return true;
        }
        // Ray is outside the diameter, so the result is a miss.
        return false;
    }
    else // Ray not parallel to the cylinder. We compute the ray/infinite cylinder intersection
    {
        const Scalar ln = n.norm();
        n.normalize();

        // Distance between two skew lines ray and cylinder axis.
        const Scalar dist = std::abs( ao.dot( n ) );

        // If the distance is bigger than radius, no further processing is needed
        // and it's an early exit. If not then an intersection with the infinite
        // cylinder is guaranteed.
        if ( dist <= radius ) {
            // TODO : this could be optimized with squared norms.
            auto v1        = cylAxis.cross( ao );
            const Scalar t = v1.dot( n ) / ln;
            auto v2        = n.cross( cylAxis ).normalized();
            const Scalar s =
                std::sqrt( radiusSquared - ( dist * dist ) ) / std::abs( r.direction().dot( v2 ) );

            Scalar tIn  = t - s;
            Scalar tOut = t + s;
            CORE_ASSERT( tIn <= tOut, " " );

            // Now clip the ray along planes. (TODO : refactor plane clipping with vsPlane ?)

            const Scalar ddotAxis = r.direction().dot( cylAxis );

            // Ray has an opposite direction cylinder axis, so it may enter through plane B and exit
            // through plane A.
            if ( ddotAxis < 0 ) {
                const Scalar tInPlaneB  = ( b - r.origin() ).dot( cylAxis ) / ddotAxis;
                const Scalar tOutPlaneA = ( a - r.origin() ).dot( cylAxis ) / ddotAxis;

                // Early exit condition if the ray misses the capped cylinder
                if ( tInPlaneB > tOut || tOutPlaneA < tIn ) { return false; }
                // Cap the in and out
                if ( tInPlaneB > tIn && tInPlaneB < tOut ) { tIn = tInPlaneB; }
                if ( tOutPlaneA > tIn && tOutPlaneA < tOut ) { tOut = tOutPlaneA; }
            }
            // Ray has the same direction as the cylinder axis it may enter through plane A and exit
            // through B.
            else if ( ddotAxis > 0 ) {
                const Scalar tInPlaneA  = ( a - r.origin() ).dot( cylAxis ) / ddotAxis;
                const Scalar tOutPlaneB = ( b - r.origin() ).dot( cylAxis ) / ddotAxis;

                // Early exit condition if the ray misses the capped cylinder
                if ( tInPlaneA > tOut || tOutPlaneB < tIn ) { return false; }
                // Cap the in and out
                if ( tInPlaneA > tIn && tInPlaneA < tOut ) { tIn = tInPlaneA; }

                if ( tOutPlaneB > tIn && tOutPlaneB < tOut ) { tOut = tOutPlaneB; }
            }
            // Degenerate case : ray parallel to end planes.
            // in that case we check that the ray does not miss the cylinder.
            else if ( UNLIKELY( ddotAxis == 0 ) ) {
                const Scalar h       = ao.dot( cylAxis );
                const Scalar lAxisSq = cylAxis.squaredNorm();
                if ( h < 0 || h > lAxisSq ) { return false; }
            }

            // At this point our tIn and tOut are valid within the capped cylinder.
            // The only thing left is to find whether the ray hits (i.e. tIn is positive).
            if ( tIn >= 0 ) {
                hitsOut.push_back( tIn );
                return true;
            }
            // tIn is negative but tOut is positive, which means the origin is inside the cylinder,
            // so we return 0
            else if ( tIn * tOut < 0 ) {
                hitsOut.push_back( 0 );
                return true;
            }
        } // End if (distance between ray and cyl axis < radius)
    }     // End of else (ray not parallel to the cylinder.

    return false;
}

bool RayCastTriangle( const Ray& ray,
                      const Vector3 a,
                      const Vector3& b,
                      const Vector3& c,
                      std::vector<Scalar>& hitsOut ) {
    const Vector3 ab = b - a;
    const Vector3 ac = c - a;

    ON_ASSERT( const Vector3 n = ab.cross( ac ) );
    CORE_ASSERT( n.squaredNorm() > 0, "Degenerate triangle" );

    // Compute determinant
    const Vector3 pvec = ray.direction().cross( ac );
    const Scalar det   = ab.dot( pvec );

    const Vector3 tvec   = ray.origin() - a;
    const Scalar inv_det = 1.f / det;

    const Vector3 qvec = tvec.cross( ab );

    if ( det > 0 ) {
        Scalar u = tvec.dot( pvec );
        if ( u < 0 || u > det ) {
            return false; // We're out of the slab across ab
        }

        Scalar v = ray.direction().dot( qvec );
        if ( v < 0 || u + v > det ) { return false; }
    }
    else if ( det < 0 ) {
        Scalar u = tvec.dot( pvec );
        if ( u > 0 || u < det ) { return false; }

        Scalar v = ray.direction().dot( qvec );
        if ( v > 0 || u + v < det ) { return false; }
    }
    else // line parallel to plane. Maybe we should intersect with the triangle edges ?
    {
        return false;
    }

    // If we're here we really intersect the triangle so let's compute T.
    const Scalar t = ac.dot( qvec ) * inv_det;
    if ( t >= 0 ) { hitsOut.push_back( t ); }
    return ( t >= 0 );
}

bool RayCastTriangleMesh( const Ray& r,
                          const TriangleMesh& mesh,
                          std::vector<Scalar>& hitsOut,
                          std::vector<Vector3ui>& trianglesIdxOut ) {
    bool hit = false;
    for ( size_t i = 0; i < mesh.getIndices().size(); ++i ) {
        const auto& t    = mesh.getIndices()[i];
        const Vector3& a = mesh.vertices()[t[0]];
        const Vector3& b = mesh.vertices()[t[1]];
        const Vector3& c = mesh.vertices()[t[2]];
        if ( RayCastTriangle( r, a, b, c, hitsOut ) ) {
            trianglesIdxOut.push_back( t );
            hit = true;
        }
    }

    return hit;
}
} // namespace Geometry
} // namespace Core
} // namespace Ra
