#include <Core/Geometry/Adjacency.hpp>
#include <Core/Math/LinearAlgebra.hpp> // Math::cotan

namespace Ra {
namespace Core {
namespace Geometry {

/////////////////////
/// GLOBAL MATRIX ///
/////////////////////

// //////////////// //
// ADJACENCY MATRIX //
// //////////////// //

AdjacencyMatrix uniformAdjacency( const uint point_size, const AlignedStdVector<Vector3ui>& T ) {
    AdjacencyMatrix A( point_size, point_size );
    for ( const auto& t : T ) {
        uint i             = t( 0 );
        uint j             = t( 1 );
        uint k             = t( 2 );
        A.coeffRef( i, j ) = 1;
        A.coeffRef( j, k ) = 1;
        A.coeffRef( k, i ) = 1;
    }
    return A;
}

AdjacencyMatrix uniformAdjacency( const VectorArray<Vector3>& p,
                                  const AlignedStdVector<Vector3ui>& T ) {
    AdjacencyMatrix A( p.size(), p.size() );
    for ( const auto& t : T ) {
        uint i             = t( 0 );
        uint j             = t( 1 );
        uint k             = t( 2 );
        A.coeffRef( i, j ) = 1;
        A.coeffRef( j, k ) = 1;
        A.coeffRef( k, i ) = 1;
    }
    return A;
}

void uniformAdjacency( const VectorArray<Vector3>& p,
                       const AlignedStdVector<Vector3ui>& T,
                       AdjacencyMatrix& Adj ) {
    Adj.resize( p.size(), p.size() );
#pragma omp parallel for
    for ( int n = 0; n < int( T.size() ); ++n ) {
        const Vector3ui& t = T[n];
        const uint i       = t( 0 );
        const uint j       = t( 1 );
        const uint k       = t( 2 );
#pragma omp critical
        {
            Adj.coeffRef( i, j ) = 1;
            Adj.coeffRef( j, k ) = 1;
            Adj.coeffRef( k, i ) = 1;
        }
    }
}

TVAdj triangleUniformAdjacency( const VectorArray<Vector3>& p,
                                const AlignedStdVector<Vector3ui>& T ) {
    const uint p_size = p.size();
    const uint t_size = T.size();
    TVAdj A( t_size, p_size );
    for ( uint t = 0; t < t_size; ++t ) {
        const uint i       = T[t]( 0 );
        const uint j       = T[t]( 1 );
        const uint k       = T[t]( 2 );
        A.coeffRef( t, i ) = 1;
        A.coeffRef( t, j ) = 1;
        A.coeffRef( t, k ) = 1;
    }
    return A;
}

AdjacencyMatrix cotangentWeightAdjacency( const VectorArray<Vector3>& p,
                                          const AlignedStdVector<Vector3ui>& T ) {
    AdjacencyMatrix A( p.size(), p.size() );
    std::vector<Eigen::Triplet<Scalar>> triplets( T.size() * 6 );
#pragma omp parallel for
    for ( int t_ = 0; t_ < int( T.size() ); ++t_ ) {
        const auto& t        = T[t_];
        uint i               = t( 0 );
        uint j               = t( 1 );
        uint k               = t( 2 );
        const Vector3 IJ     = p[j] - p[i];
        const Vector3 JK     = p[k] - p[j];
        const Vector3 KI     = p[i] - p[k];
        const Scalar cotI    = Math::cotan( IJ, ( -KI ).eval() );
        const Scalar cotJ    = Math::cotan( JK, ( -IJ ).eval() );
        const Scalar cotK    = Math::cotan( KI, ( -JK ).eval() );
        triplets[6 * t_]     = Eigen::Triplet<Scalar>( i, j, cotK );
        triplets[6 * t_ + 1] = Eigen::Triplet<Scalar>( j, i, cotK );
        triplets[6 * t_ + 2] = Eigen::Triplet<Scalar>( j, k, cotI );
        triplets[6 * t_ + 3] = Eigen::Triplet<Scalar>( k, j, cotI );
        triplets[6 * t_ + 4] = Eigen::Triplet<Scalar>( k, i, cotJ );
        triplets[6 * t_ + 5] = Eigen::Triplet<Scalar>( i, k, cotJ );
    }
    A.setFromTriplets( triplets.begin(), triplets.end() );
    return ( 0.5 * A );
}

// ///////////// //
// DEGREE MATRIX //
// ///////////// //

DegreeMatrix adjacencyDegree( const AdjacencyMatrix& A ) {
    DegreeMatrix D( A.rows(), A.cols() );
    D.reserve( A.rows() );
    for ( int i = 0; i < D.diagonal().size(); ++i ) {
        D.coeffRef( i, i ) = A.row( i ).sum();
    }
    return D;
}

} // namespace Geometry
} // namespace Core
} // namespace Ra
