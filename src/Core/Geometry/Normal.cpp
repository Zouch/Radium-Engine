#include <Core/Geometry/Normal.hpp>

#include <Core/Geometry/TriangleOperation.hpp>
#include <Core/Math/LinearAlgebra.hpp> // Math::angle
#include <Core/Utils/CircularIndex.hpp>
#include <Core/Utils/Timer.hpp>

namespace Ra {
namespace Core {
namespace Geometry {

//////////////
/// GLOBAL ///
//////////////

void uniformNormal( const VectorArray<Vector3>& p,
                    const AlignedStdVector<Vector3ui>& T,
                    VectorArray<Vector3>& normal ) {
    const size_t N = p.size();
    normal.clear();
    normal.resize( N, Vector3::Zero() );

    for ( const auto& t : T ) {
        const uint i       = t( 0 );
        const uint j       = t( 1 );
        const uint k       = t( 2 );
        const Vector3 triN = triangleNormal( p[i], p[j], p[k] );
        if ( !triN.allFinite() ) { continue; }
        normal[i] += triN;
        normal[j] += triN;
        normal[k] += triN;
    }

#pragma omp parallel for
    for ( int i = 0; i < int( N ); ++i ) {
        if ( !normal[i].isApprox( Vector3::Zero() ) ) { normal[i].normalize(); }
    }

    // could also do:
    // normal.getMap().colwise().normalize();
}

Vector3 localUniformNormal( const uint ii,
                            const VectorArray<Vector3>& p,
                            const AlignedStdVector<Vector3ui>& T,
                            const Sparse& adj ) {
    Vector3 normal = Vector3::Zero();
    for ( Sparse::InnerIterator it( adj, ii ); it; ++it ) {
        const size_t t = it.row();
        const uint i   = T[t]( 0 );
        const uint j   = T[t]( 1 );
        const uint k   = T[t]( 2 );
        normal += triangleNormal( p[i], p[j], p[k] );
    }
    return normal; //.normalized();
}

void angleWeightedNormal( const VectorArray<Vector3>& p,
                          const AlignedStdVector<Vector3ui>& T,
                          VectorArray<Vector3>& normal ) {
    const size_t N = p.size();
    normal.clear();
    normal.resize( N, Vector3::Zero() );
    for ( const auto& t : T ) {
        uint i               = t( 0 );
        uint j               = t( 1 );
        uint k               = t( 2 );
        const Vector3 triN   = triangleNormal( p[i], p[j], p[k] );
        const Scalar theta_i = Math::angle( ( p[j] - p[i] ), ( p[k] - p[i] ) );
        const Scalar theta_j = Math::angle( ( p[i] - p[j] ), ( p[k] - p[j] ) );
        const Scalar theta_k = Math::angle( ( p[i] - p[k] ), ( p[j] - p[k] ) );
        normal[i] += theta_i * triN;
        normal[j] += theta_j * triN;
        normal[k] += theta_k * triN;
    }
    for ( auto& n : normal ) {
        n.normalize();
    }
}

void areaWeightedNormal( const VectorArray<Vector3>& p,
                         const AlignedStdVector<Vector3ui>& T,
                         VectorArray<Vector3>& normal ) {
    const size_t N = p.size();
    normal.clear();
    normal.resize( N, Vector3::Zero() );
    for ( const auto& t : T ) {
        uint i             = t( 0 );
        uint j             = t( 1 );
        uint k             = t( 2 );
        const Scalar area  = triangleArea( p[i], p[j], p[k] );
        const Vector3 triN = area * triangleNormal( p[i], p[j], p[k] );
        normal[i] += triN;
        normal[j] += triN;
        normal[k] += triN;
    }
    for ( auto& n : normal ) {
        n.normalize();
    }
}

////////////////
/// ONE RING ///
////////////////

Vector3 uniformNormal( const Vector3& v, const VectorArray<Vector3>& one_ring ) {
    Vector3 normal;
    normal.setZero();
    size_t N = one_ring.size();
    Utils::CircularIndex i;
    i.setSize( N );
    for ( uint j = 0; j < N; ++j ) {
        i.setValue( j );
        normal += triangleNormal( v, one_ring[i], one_ring[i - 1] );
    }
    if ( !normal.isApprox( Vector3::Zero() ) ) { return normal.normalized(); }
    return Vector3::Zero();
}

Vector3 angleWeightedNormal( const Vector3& v, const VectorArray<Vector3>& one_ring ) {
    Vector3 normal;
    normal.setZero();
    size_t N = one_ring.size();
    Utils::CircularIndex i;
    i.setSize( N );
    for ( uint j = 0; j < N; ++j ) {
        i.setValue( j );
        Scalar theta = Math::angle( ( one_ring[i] - v ), ( one_ring[i - 1] - v ) );
        normal += theta * triangleNormal( v, one_ring[i], one_ring[i - 1] );
    }
    if ( !normal.isApprox( Vector3::Zero() ) ) { return normal.normalized(); }
    return Vector3::Zero();
}

Vector3 areaWeightedNormal( const Vector3& v, const VectorArray<Vector3>& one_ring ) {
    Vector3 normal;
    normal.setZero();
    size_t N = one_ring.size();
    Utils::CircularIndex i;
    i.setSize( N );
    for ( uint j = 0; j < N; ++j ) {
        i.setValue( j );
        Scalar area = triangleArea( v, one_ring[i], one_ring[i - 1] );
        normal += area * triangleNormal( v, one_ring[i], one_ring[i - 1] );
    }
    if ( !normal.isApprox( Vector3::Zero() ) ) { return normal.normalized(); }
    return Vector3::Zero();
}

} // namespace Geometry
} // namespace Core
} // namespace Ra
