#pragma once

#include <Core/Animation/HandleWeight.hpp>
#include <Core/Animation/Pose.hpp>
#include <Core/Animation/Skeleton.hpp>
#include <Core/Geometry/TriangleMesh.hpp>
#include <Core/RaCore.hpp>

namespace Ra {
namespace Core {
namespace Animation {

/// Skinning data that gets set at startup including the "reference state"
struct SkinningRefData {
    /// The animation skeleton.
    Skeleton m_skeleton;

    /// The mesh in reference position.
    Geometry::TriangleMesh m_referenceMesh;

    /// The reference pose.
    Pose m_refPose;

    /// The matrix of skinning weights.
    WeightMatrix m_weights;

    /// The per-bone bind matrices.
    std::map<uint, Transform> m_bindMatrices;

    /// The optionnal centers of rotations for CoR skinning.
    Vector3Array m_CoR;

    /// The matrix of STBS weights.
    WeightMatrix m_weightSTBS;
};

/// Pose data for one frame. Poses are in model space.
struct SkinningFrameData {
    /// The pose at the previous frame.
    Pose m_previousPose;

    /// The pose at the current frame.
    Pose m_currentPose;

    /// The relative pose from previous to current.
    Pose m_prevToCurrentRelPose;

    /// The relative pose from reference pose to current.
    Pose m_refToCurrentRelPose;

    /// The position of the mesh vertices at the previous frame.
    Vector3Array m_previousPosition;

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
