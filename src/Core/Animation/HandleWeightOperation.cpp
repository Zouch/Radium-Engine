#include <Core/Animation/HandleWeightOperation.hpp>
#include <Core/Math/LinearAlgebra.hpp> // Math::checkInvalidNumbers
#include <Core/Utils/Log.hpp>
#include <utility>

namespace Ra {
namespace Core {
namespace Animation {

using namespace Utils; // log

WeightMatrix extractWeightMatrix( const MeshWeight& weight, const uint weight_size ) {
    WeightMatrix W( weight.size(), weight_size );
    W.setZero();
    for ( uint i = 0; i < weight.size(); ++i )
    {
        for ( const auto& w : weight[i] )
        { W.coeffRef( i, w.first ) = w.second; }
    }
    return W;
}

MeshWeight extractMeshWeight( Eigen::Ref<const WeightMatrix> matrix ) {
    MeshWeight W( matrix.rows() );
    for ( int i = 0; i < matrix.rows(); ++i )
    {
        for ( int j = 0; j < matrix.cols(); ++j )
        {
            if ( matrix.coeff( i, j ) != 0.0 )
            {
                std::pair<uint, Scalar> w( uint( j ), matrix.coeff( i, j ) );
                W[i].push_back( w );
            }
        }
    }
    return W;
}

WeightMatrix partitionOfUnity( Eigen::Ref<const WeightMatrix> weights ) {
    WeightMatrix W = weights;
    normalizeWeights( W );
    return W;
}

uint getMaxWeightIndex( Eigen::Ref<const WeightMatrix> weights, const uint vertexID ) {
    uint maxId  = uint( -1 );
    VectorN row = weights.row( vertexID );
    row.maxCoeff( &maxId );
    return maxId;
}

void getMaxWeightIndex( Eigen::Ref<const WeightMatrix> weights, std::vector<uint>& handleID ) {
    handleID.resize( weights.rows() );
    for ( int i = 0; i < weights.rows(); ++i )
    { handleID[i] = getMaxWeightIndex( weights, i ); }
}

bool checkWeightMatrix( Eigen::Ref<const WeightMatrix> matrix,
                        const bool FAIL_ON_ASSERT,
                        const bool MT ) {
    bool ok = Math::checkInvalidNumbers( matrix, FAIL_ON_ASSERT ) &&
              checkNoWeightVertex( matrix, FAIL_ON_ASSERT, MT );

    if ( !ok ) { LOG( logDEBUG ) << "Matrix is not good."; }

    return ok;
}

bool checkNoWeightVertex( Eigen::Ref<const WeightMatrix> matrix,
                          const bool FAIL_ON_ASSERT,
                          const bool MT ) {
    int status = 1;
    LOG( logDEBUG ) << "Searching for empty rows in the matrix...";
    if ( MT )
    {
#pragma omp parallel for
        for ( int i = 0; i < matrix.rows(); ++i )
        {
            Sparse row      = matrix.row( i );
            const int check = ( row.nonZeros() > 0 ) ? 1 : 0;
#pragma omp atomic
            status &= check;
        }
        if ( status == 0 )
        {
            if ( FAIL_ON_ASSERT ) { CORE_ASSERT( false, "At least a vertex as no weights" ); }
            else
            { LOG( logDEBUG ) << "At least a vertex as no weights"; }
        }
    }
    else
    {
        for ( int i = 0; i < matrix.rows(); ++i )
        {
            Sparse row = matrix.row( i );
            if ( row.nonZeros() == 0 )
            {
                status = 0;

                const std::string text = "Vertex " + std::to_string( i ) + " has no weights.";
                if ( FAIL_ON_ASSERT ) { CORE_ASSERT( false, text.c_str() ); }
                else
                { LOG( logDEBUG ) << text; }
            }
        }
    }
    return status != 0;
}

bool normalizeWeights( Eigen::Ref<WeightMatrix> matrix, const bool MT ) {
    CORE_UNUSED( MT );

    bool skinningWeightOk = true;

#pragma omp parallel for if ( MT )
    for ( int k = 0; k < matrix.innerSize(); ++k )
    {
        const Scalar sum = matrix.row( k ).sum();
        if ( !Ra::Core::Math::areApproxEqual( sum, 0_ra ) )
        {
            if ( !Ra::Core::Math::areApproxEqual( sum, 1_ra ) )
            {
                skinningWeightOk = false;
                matrix.row( k ) /= sum;
            }
        }
    }
    return !skinningWeightOk;
}

} // namespace Animation
} // Namespace Core
} // Namespace Ra
