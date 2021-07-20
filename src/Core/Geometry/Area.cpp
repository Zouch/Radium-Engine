#include <Core/Geometry/Area.hpp>
#include <Core/Geometry/TriangleOperation.hpp>
#include <Core/Math/LinearAlgebra.hpp> // Math::cotan
#include <Core/Utils/CircularIndex.hpp>

namespace Ra {
namespace Core {
namespace Geometry {

/////////////////////
/// GLOBAL MATRIX ///
/////////////////////

AreaMatrix oneRingArea( const VectorArray<Vector3>& p, const AlignedStdVector<Vector3ui>& T ) {
    AreaMatrix A( p.size(), p.size() );
    A.reserve( p.size() );
    for ( const auto& t : T )
    {
        uint i      = t( 0 );
        uint j      = t( 1 );
        uint k      = t( 2 );
        Scalar area = triangleArea( p[i], p[j], p[k] );
        A.coeffRef( i, i ) += area;
        A.coeffRef( j, j ) += area;
        A.coeffRef( k, k ) += area;
    }
    return A;
}

void oneRingArea( const VectorArray<Vector3>& p,
                  const AlignedStdVector<Vector3ui>& T,
                  AreaMatrix& A ) {
    A.resize( p.size(), p.size() );
    A.reserve( p.size() );
    int size = int( T.size() );
#pragma omp parallel for
    for ( int n = 0; n < size; ++n )
    {
        const Vector3ui& t = T[n];
        const uint i       = t( 0 );
        const uint j       = t( 1 );
        const uint k       = t( 2 );
        const Scalar area  = triangleArea( p[i], p[j], p[k] );
#pragma omp critical
        {
            A.coeffRef( i, i ) += area;
            A.coeffRef( j, j ) += area;
            A.coeffRef( k, k ) += area;
        }
    }
}

AreaMatrix barycentricArea( const VectorArray<Vector3>& p, const AlignedStdVector<Vector3ui>& T ) {
    return ( ( Scalar( 1. ) / Scalar( 3. ) ) * oneRingArea( p, T ) );
}

void barycentricArea( const VectorArray<Vector3>& p,
                      const AlignedStdVector<Vector3ui>& T,
                      AreaMatrix& A ) {
    oneRingArea( p, T, A );
    int size = int( p.size() );
#pragma omp parallel for
    for ( int i = 0; i < size; ++i )
    { A.coeffRef( i, i ) /= Scalar( 3. ); }
}

AreaMatrix voronoiArea( const VectorArray<Vector3>& p, const AlignedStdVector<Vector3ui>& T ) {
    AreaMatrix A( p.size(), p.size() );
    A.reserve( p.size() );
    for ( const auto& t : T )
    {
        uint i = t( 0 );
        uint j = t( 1 );
        uint k = t( 2 );
        A.coeffRef( i, i ) +=
            Math::cotan( ( p[i] - p[k] ), ( p[j] - p[k] ) ) * ( p[i] - p[j] ).squaredNorm();
        A.coeffRef( j, j ) +=
            Math::cotan( ( p[j] - p[i] ), ( p[k] - p[i] ) ) * ( p[j] - p[k] ).squaredNorm();
        A.coeffRef( k, k ) +=
            Math::cotan( ( p[k] - p[j] ), ( p[i] - p[j] ) ) * ( p[k] - p[i] ).squaredNorm();
    }
    return ( ( 1.0 / 8.0 ) * A );
}

AreaMatrix mixedArea( const VectorArray<Vector3>& p, const AlignedStdVector<Vector3ui>& T ) {
    AreaMatrix A( p.size(), p.size() );
    A.reserve( p.size() );

    const Scalar w = ( Scalar( 1. ) / Scalar( 8. ) );

    for ( const auto& t : T )
    {
        uint i = t( 0 );
        uint j = t( 1 );
        uint k = t( 2 );
        if ( !isTriangleObtuse( p[i], p[j], p[k] ) )
        {
            Vector3 ij  = p[j] - p[i];
            Vector3 jk  = p[k] - p[j];
            Vector3 ki  = p[i] - p[k];
            Scalar IJ   = ( ij ).squaredNorm();
            Scalar JK   = ( jk ).squaredNorm();
            Scalar KI   = ( ki ).squaredNorm();
            Scalar cotI = Math::cotan( ij, ( -ki ).eval() );
            Scalar cotJ = Math::cotan( jk, ( -ij ).eval() );
            Scalar cotK = Math::cotan( ki, ( -jk ).eval() );
            A.coeffRef( i, i ) += w * ( ( KI * cotJ ) + ( IJ * cotK ) );
            A.coeffRef( j, j ) += w * ( ( IJ * cotK ) + ( JK * cotI ) );
            A.coeffRef( k, k ) += w * ( ( JK * cotI ) + ( KI * cotJ ) );
        }
        else
        {
            Scalar area = triangleArea( p[i], p[j], p[k] );
            if ( ( ( ( p[j] - p[i] ).normalized() ).dot( ( p[k] - p[i] ).normalized() ) ) <
                 Scalar( 0. ) )
            {
                /* obtuse at i */
                A.coeffRef( i, i ) += area / Scalar( 2. );
                A.coeffRef( j, j ) += area / Scalar( 4. );
                A.coeffRef( k, k ) += area / Scalar( 4. );
            }
            else
            {
                if ( ( ( ( p[k] - p[j] ).normalized() ).dot( ( p[i] - p[j] ).normalized() ) ) <
                     Scalar( 0.0 ) )
                {
                    /* obtuse at j */
                    A.coeffRef( i, i ) += area / Scalar( 4. );
                    A.coeffRef( j, j ) += area / Scalar( 2. );
                    A.coeffRef( k, k ) += area / Scalar( 4. );
                }
                else
                {
                    /* obtuse at k */
                    A.coeffRef( i, i ) += area / Scalar( 4. );
                    A.coeffRef( j, j ) += area / Scalar( 4. );
                    A.coeffRef( k, k ) += area / Scalar( 2. );
                }
            }
        }
    }
    return A;
}

////////////////
/// ONE RING ///
////////////////

Scalar oneRingArea( const Vector3& v, const VectorArray<Vector3>& p ) {
    Scalar area = 0.0;
    uint N      = p.size();
    Utils::CircularIndex i;
    i.setSize( N );
    for ( uint j = 0; j < N; ++j )
    {
        i.setValue( j );
        area += triangleArea( v, p[i], p[i - 1] );
    }
    return area;
}

Scalar barycentricArea( const Vector3& v, const VectorArray<Vector3>& p ) {
    return ( oneRingArea( v, p ) / Scalar( 3. ) );
}

Scalar voronoiArea( const Vector3& v, const VectorArray<Vector3>& p ) {
    Scalar area = 0.0;
    uint N      = p.size();
    Utils::CircularIndex i;
    i.setSize( N );
    for ( uint j = 0; j < N; ++j )
    {
        i.setValue( j );
        Scalar cot_a = Math::cotan( ( v - p[i - 1] ), ( p[i] - p[i - 1] ) );
        Scalar cot_b = Math::cotan( ( v - p[i + 1] ), ( p[i] - p[i + 1] ) );
        area += ( cot_a + cot_b ) * ( v - p[i] ).squaredNorm();
    }
    return ( ( Scalar( 1. ) / Scalar( 8. ) ) * area );
}

Scalar mixedArea( const Vector3& v, const VectorArray<Vector3>& p ) {
    Scalar area = 0.0;
    uint N      = p.size();
    Utils::CircularIndex i;
    i.setSize( N );
    for ( uint j = 0; j < N; ++j )
    {
        i.setValue( j );
        if ( !isTriangleObtuse( v, p[i], p[i - 1] ) )
        {
            // For the triangle PQR ( a.k.a. v, p[i], p[i-1] ), the area for P ( a.k.a. v ) is :
            Scalar PQ   = ( p[i] - v ).squaredNorm();
            Scalar PR   = ( p[i - 1] - v ).squaredNorm();
            Scalar cotQ = Math::cotan( ( p[i - 1] - p[i] ), ( v - p[i] ) );
            Scalar cotR = Math::cotan( ( v - p[i - 1] ), ( p[i] - p[i - 1] ) );
            area += Scalar( 1. ) / Scalar( 8. ) * ( ( PR * cotQ ) + ( PQ * cotR ) );
        }
        else
        {
            if ( ( ( ( p[i] - v ).normalized() ).dot( ( p[i - 1] - v ).normalized() ) ) <
                 Scalar( 0. ) )
            { area += triangleArea( v, p[i], p[i - 1] ) / Scalar( 2. ); }
            else
            { area += triangleArea( v, p[i], p[i - 1] ) / Scalar( 4. ); }
        }
    }
    return area;
}

} // namespace Geometry
} // namespace Core
} // namespace Ra
