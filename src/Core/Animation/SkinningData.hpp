#pragma once

#include <Core/Animation/HandleWeight.hpp>
#include <Core/Animation/Pose.hpp>
#include <Core/Animation/Skeleton.hpp>
#include <Core/Geometry/TriangleMesh.hpp>
#include <Core/RaCore.hpp>

namespace Ra {
namespace Core {
namespace Animation {

/// \brief Skinning data that get set at startup including the "reference state".
struct SkinningRefData {
    /// The mesh in reference position.
    Geometry::TriangleMesh m_referenceMesh;

    /// The inverse of the mesh's transform.
    Transform m_meshTransformInverse;

    /// The animation skeleton saved in rest pose.
    Skeleton m_skeleton;

    /// The per-bone bind matrices.
    AlignedStdVector<Transform> m_bindMatrices;

    /// The matrix of skinning weights.
    WeightMatrix m_weights;

    /// The optionnal centers of rotations for CoR skinning.
    Vector3Array m_CoR;

    /// The optional matrix of weights for STBS skinning.
    WeightMatrix m_weightSTBS;
};

/// Pose data for one frame. Poses are in model space.
struct SkinningFrameData {
    /// The animation skeleton in the current pose.
    Skeleton m_skeleton;

    /// The current mesh vertex position.
    Vector3Array m_currentPosition;

    /// The current mesh vertex normals.
    Vector3Array m_currentNormal;

    /// The current mesh vertex tangent vectors.
    Vector3Array m_currentTangent;

    /// The current mesh vertex bitangent vectors.
    Vector3Array m_currentBitangent;

    /// The number of the current frame.
    uint m_frameCounter;

    /// Whether skinning must be processed.
    /// It is set to true if the current pose is different from previous.
    bool m_doSkinning;

    /// Whether the skin must be reset to its initial reference configuration.
    bool m_doReset;
};

} // namespace Skinning
} // namespace Core
} // namespace Ra
