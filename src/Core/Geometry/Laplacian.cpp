#include <Core/Geometry/Laplacian.hpp>
#include <Core/Math/LinearAlgebra.hpp> // Math::cotan
#include <Core/Utils/CircularIndex.hpp>

namespace Ra {
namespace Core {
namespace Geometry {

/////////////////////
/// GLOBAL MATRIX ///
/////////////////////

LaplacianMatrix standardLaplacian( const DegreeMatrix& D,
                                   const AdjacencyMatrix& A,
                                   const bool POSITIVE_SEMI_DEFINITE ) {
    if ( POSITIVE_SEMI_DEFINITE ) { return ( D - A ); }
    return ( A - D );
}

LaplacianMatrix symmetricNormalizedLaplacian( const DegreeMatrix& D, const AdjacencyMatrix& A ) {
    Sparse I( D.rows(), D.cols() );
    I.setIdentity();
    DegreeMatrix sqrt_inv_D = D.cwiseInverse().cwiseSqrt();
    return ( I - ( sqrt_inv_D * A * sqrt_inv_D ) );
}

LaplacianMatrix randomWalkNormalizedLaplacian( const DegreeMatrix& D, const AdjacencyMatrix& A ) {
    return ( D.cwiseInverse() * A );
}

LaplacianMatrix powerLaplacian( const LaplacianMatrix& L, const uint k ) {
    LaplacianMatrix lap( L.rows(), L.cols() );
    lap.setIdentity();
    for ( uint i = 0; i < k; ++i ) {
        lap = L * lap;
    }
    return lap;
}

LaplacianMatrix cotangentWeightLaplacian( const VectorArray<Vector3>& p,
                                          const AlignedStdVector<Vector3ui>& T ) {
    LaplacianMatrix L( p.size(), p.size() );
    for ( const auto& t : T ) {
        uint i      = t( 0 );
        uint j      = t( 1 );
        uint k      = t( 2 );
        Vector3 IJ  = p[j] - p[i];
        Vector3 JK  = p[k] - p[j];
        Vector3 KI  = p[i] - p[k];
        Scalar cotI = Math::cotan( IJ, ( -KI ).eval() );
        Scalar cotJ = Math::cotan( JK, ( -IJ ).eval() );
        Scalar cotK = Math::cotan( KI, ( -JK ).eval() );
        L.coeffRef( i, j ) -= cotK;
        L.coeffRef( j, i ) -= cotK;
        L.coeffRef( j, k ) -= cotI;
        L.coeffRef( k, j ) -= cotI;
        L.coeffRef( k, i ) -= cotJ;
        L.coeffRef( i, k ) -= cotJ;
        L.coeffRef( i, i ) += cotJ + cotK;
        L.coeffRef( j, j ) += cotI + cotK;
        L.coeffRef( k, k ) += cotI + cotJ;
    }
    return ( 0.5 * L );
}

////////////////
/// ONE RING ///
////////////////

Vector3 uniformLaplacian( const Vector3& v, const VectorArray<Vector3>& p ) {
    Vector3 L;
    L.setZero();
    for ( const auto& pi : p ) {
        L += ( v - pi );
    }
    return L;
}

Vector3 cotangentWeightLaplacian( const Vector3& v, const VectorArray<Vector3>& p ) {
    Vector3 L;
    L.setZero();
    uint N = p.size();
    Utils::CircularIndex i;
    i.setSize( N );
    for ( uint j = 0; j < N; ++j ) {
        i.setValue( j );
        Scalar cot_a = Math::cotan( ( v - p[i - 1] ), ( p[i] - p[i - 1] ) );
        Scalar cot_b = Math::cotan( ( v - p[i + 1] ), ( p[i] - p[i + 1] ) );
        L += ( cot_a + cot_b ) * ( v - p[i] );
    }
    return ( 0.5 * L );
}

} // namespace Geometry
} // namespace Core
} // namespace Ra
