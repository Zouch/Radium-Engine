#include <Core/Animation/LinearBlendSkinning.hpp>

#include <Core/Animation/SkinningData.hpp>

namespace Ra {
namespace Core {
namespace Animation {

void linearBlendSkinning( const SkinningRefData& refData,
                          const Vector3Array& tangents,
                          const Vector3Array& bitangents,
                          SkinningFrameData& frameData ) {
    const auto& W = refData.m_weights;
    const auto& vertices = refData.m_referenceMesh.vertices();
    const auto& normals = refData.m_referenceMesh.normals();
    const auto& pose = frameData.m_currentPose;
#pragma omp parallel for
    for ( int i = 0; i < int( frameData.m_currentPosition.size() ); ++i )
    {
        frameData.m_currentPosition[i] = Vector3::Zero();
        frameData.m_currentNormal[i] = Vector3::Zero();
        frameData.m_currentTangent[i] = Vector3::Zero();
        frameData.m_currentBitangent[i] = Vector3::Zero();
    }
    for ( int k = 0; k < W.outerSize(); ++k )
    {
        const int nonZero = W.col( k ).nonZeros();
        WeightMatrix::InnerIterator it0( W, k );
#pragma omp parallel for
        for ( int nz = 0; nz < nonZero; ++nz )
        {
            WeightMatrix::InnerIterator it = it0 + Eigen::Index( nz );
            const uint i                   = it.row();
            const uint j                   = it.col();
            const Scalar w                 = it.value();
            frameData.m_currentPosition[i] += w * ( pose[j] * vertices[i] );
            frameData.m_currentNormal[i] += w * ( pose[j].linear() * normals[i] );
            frameData.m_currentTangent[i] += w * ( pose[j].linear() * tangents[i] );
            frameData.m_currentBitangent[i] += w * ( pose[j].linear() * bitangents[i] );
        }
    }
}

} // namespace Animation
} // namespace Core
} // namespace Ra
