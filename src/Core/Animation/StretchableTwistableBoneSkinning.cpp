#include <Core/Animation/StretchableTwistableBoneSkinning.hpp>

#include <Core/Animation/PoseOperation.hpp>
#include <Core/Animation/Skeleton.hpp>
#include <Core/Animation/SkinningData.hpp>
#include <Core/Geometry/DistanceQueries.hpp>

namespace Ra {
namespace Core {
namespace Animation {

WeightMatrix computeSTBS_weights( const Vector3Array& inMesh, const Skeleton& skel ) {
    std::vector<Eigen::Triplet<Scalar>> triplets;
    for ( int i = 0; i < int( inMesh.size() ); ++i )
    {
        const auto& pi = inMesh[uint( i )];
        for ( int j = 0; j < int( skel.size() ); ++j )
        {
            Vector3 a, b;
            skel.getBonePoints( uint( j ), a, b );
            const Vector3 ab = b - a;
            Scalar t         = Geometry::projectOnSegment( pi, a, ab );
            if ( t > 0 ) { triplets.push_back( Eigen::Triplet<Scalar>( i, j, t ) ); }
        }
    }
    WeightMatrix weights( uint( inMesh.size() ), skel.size() );
    weights.setFromTriplets( triplets.begin(), triplets.end() );
    return weights;
}

void linearBlendSkinningSTBS( const SkinningRefData& refData,
                              const Vector3Array& tangents,
                              const Vector3Array& bitangents,
                              SkinningFrameData& frameData ) {
    // reset all outputs
#pragma omp parallel for
    for ( int i = 0; i < int( frameData.m_currentPosition.size() ); ++i )
    {
        frameData.m_currentPosition[i]  = Vector3::Zero();
        frameData.m_currentNormal[i]    = Vector3::Zero();
        frameData.m_currentTangent[i]   = Vector3::Zero();
        frameData.m_currentBitangent[i] = Vector3::Zero();
    }
    // get all needed data
    const auto& refPose  = refData.m_skeleton.getPose( HandleArray::SpaceType::MODEL );
    const auto& curPose  = frameData.m_skeleton.getPose( HandleArray::SpaceType::MODEL );
    const auto relPose   = relativePose( curPose, refPose );
    const auto& vertices = refData.m_referenceMesh.vertices();
    const auto& normals  = refData.m_referenceMesh.normals();
    // for each bone
    for ( uint k = 0; k < refData.m_weights.outerSize(); ++k )
    {
        // get the bone's full stretch vector
        Vector3 a, b, a_, b_;
        refData.m_skeleton.getBonePoints( k, a, b );
        frameData.m_skeleton.getBonePoints( k, a_, b_ );
        const Vector3 ab = b - a;
        const Vector3 es = ( std::sqrt( ( b_ - a_ ).squaredNorm() / ab.squaredNorm() ) - 1 ) * ab;
        // fetch bone transforms
        const auto R_rel  = relPose[k].linear();
        const auto& T_ref = refPose[k];
        const auto& T_cur = curPose[k];
        const auto& B     = refData.m_bindMatrices[k];
        // then go through the skinned vertices
        const int nonZero = refData.m_weights.col( k ).nonZeros();
        WeightMatrix::InnerIterator it0( refData.m_weights, k );
#pragma omp parallel for
        for ( int nz = 0; nz < nonZero; ++nz )
        {
            WeightMatrix::InnerIterator it = it0 + Eigen::Index( nz );
            const uint i                   = it.row();
            const uint j                   = it.col();
            const Scalar w                 = it.value();
            const Scalar eis               = refData.m_weightSTBS.coeff( i, j );
            frameData.m_currentPosition[i] +=
                w * ( refData.m_meshTransformInverse *
                      ( a_ + R_rel * ( eis * es - a + T_ref * B * vertices[i] ) ) );
            auto J = refData.m_meshTransformInverse * T_cur * B;
            frameData.m_currentNormal[i] += w * J.linear() * normals[i];
            frameData.m_currentTangent[i] += w * J.linear() * tangents[i];
            frameData.m_currentBitangent[i] += w * J.linear() * bitangents[i];
        }
    }
}

DQList computeDQSTBS( const SkinningRefData& refData, const Skeleton& poseSkel ) {
    const auto& refPose    = refData.m_skeleton.getPose( HandleArray::SpaceType::MODEL );
    const auto& curPose    = poseSkel.getPose( HandleArray::SpaceType::MODEL );
    const auto relPose     = relativePose( curPose, refPose );
    const auto& weight     = refData.m_weights;
    const auto& weightSTBS = refData.m_weightSTBS;
    CORE_ASSERT( ( refPose.size() == size_t( weight.cols() ) ), "pose/weight size mismatch." );
    DQList DQ( uint( weight.rows() ),
               DualQuaternion( Quaternion( 0, 0, 0, 0 ), Quaternion( 0, 0, 0, 0 ) ) );
    // Stores the first non-zero quaternion for each vertex.
    std::vector<uint> firstNonZero( weight.rows(), std::numeric_limits<uint>::max() );
    // Contains the converted dual quaternions from the pose
    std::vector<DualQuaternion> poseDQ( relPose.size() );

    // for each bone
    for ( uint k = 0; k < weight.outerSize(); ++k )
    {
        // get the bone's full stretch vector "e_s"
        Vector3 a, b, a_, b_;
        refData.m_skeleton.getBonePoints( k, a, b );
        poseSkel.getBonePoints( k, a_, b_ );
        const Vector3 ab = b - a;
        const Vector3 es = ( std::sqrt( ( b_ - a_ ).squaredNorm() / ab.squaredNorm() ) - 1 ) * ab;
        // fetch bone transforms
        const auto R_rel  = relPose[k].linear();
        const auto& T_ref = refPose[k];
        const auto& B     = refData.m_bindMatrices[k];
        poseDQ[k]         = DualQuaternion( refData.m_meshTransformInverse * curPose[k] * B );
        // then go through the skinned vertices
        const int nonZero = weight.col( k ).nonZeros();
        WeightMatrix::InnerIterator it0( weight, k );
#pragma omp parallel for
        for ( int nz = 0; nz < nonZero; ++nz )
        {
            WeightMatrix::InnerIterator itn = it0 + Eigen::Index( nz );
            const uint i                    = itn.row();
            const uint j                    = itn.col();
            const Scalar w                  = itn.value();
            const Scalar eis                = weightSTBS.coeff( i, j );
            // build the dual quaternion from the STBS transform
            Transform Ti     = Transform::Identity();
            Ti.translation() = a_ + R_rel * ( eis * es - a );
            const Transform Ri( R_rel * T_ref * B );
            DualQuaternion D( refData.m_meshTransformInverse * Ti * Ri );
            D.normalize();
            // mix it with the dual quaternion from the other skinning bones
            firstNonZero[i] = std::min( firstNonZero[i], uint( k ) );
            const Scalar sign =
                Math::signNZ( poseDQ[j].getQ0().dot( poseDQ[firstNonZero[i]].getQ0() ) );
            DQ[i] += D * w * sign;
        }
    }
    // Normalize all dual quats.
#pragma omp parallel for
    for ( int i = 0; i < int( DQ.size() ); ++i )
    {
        DQ[i].normalize();
    }
    return DQ;
}

void RA_CORE_API dualQuaternionSkinningSTBS( const SkinningRefData& refData,
                                             const Vector3Array& tangents,
                                             const Vector3Array& bitangents,
                                             SkinningFrameData& frameData ) {
    // compute the STBS dual quaternion for each vertex
    const auto DQ = computeDQSTBS( refData, frameData.m_skeleton );
    // apply DQS
    const auto& vertices = refData.m_referenceMesh.vertices();
    const auto& normals  = refData.m_referenceMesh.normals();
#pragma omp parallel for
    for ( int i = 0; i < int( frameData.m_currentPosition.size() ); ++i )
    {
        const auto& DQi                 = DQ[i];
        frameData.m_currentPosition[i]  = DQi.transform( vertices[i] );
        frameData.m_currentNormal[i]    = DQi.rotate( normals[i] );
        frameData.m_currentTangent[i]   = DQi.rotate( tangents[i] );
        frameData.m_currentBitangent[i] = DQi.rotate( bitangents[i] );
    }
}

} // namespace Animation
} // namespace Core
} // namespace Ra
