#pragma once

#include <Core/Animation/HandleWeight.hpp>
#include <Core/Animation/Pose.hpp>
#include <Core/Containers/AlignedStdVector.hpp>
#include <Core/Containers/VectorArray.hpp>
#include <Core/Math/DualQuaternion.hpp>

namespace Ra {
namespace Core {
namespace Animation {

struct SkinningRefData;
struct SkinningFrameData;

using DQList = AlignedStdVector<DualQuaternion>;

/// \name Dual Quaternion Skinning
/// \{

// clang-format off
/**
 * \brief Computes the per-vertex dual quaternion.
 *
 * Such a dual quaternion is computed as:
 * \f$ \mathbf{Q}_i=\frac{\sum_{s\in S} \omega_{is}\mathbf{Q}_s}
 *                       {\|\sum_{s\in S} \omega_{is}\mathbf{Q}_s\|} \f$
 */
// clang-format on
DQList RA_CORE_API computeDQ( const Pose& pose, const WeightMatrix& weight );

/**
 * \brief Default non-optimized, non-parallel implementation of computeDQ.
 */
DQList RA_CORE_API computeDQ_naive( const Pose& pose, const WeightMatrix& weight );

/**
 * \brief Applies the given Dual-Quaternions to the given vertices.
 * \note Parallelized loop inside (using openmp).
 */
Vector3Array RA_CORE_API applyDualQuaternions( const DQList& DQ, Vector3Array& vertices );

// clang-format off
/**
 * \brief Applies Dual-Quaternion skinning to the current frame.
 *
 * Dual-Quaternion skinning deforms the mesh geometry for frame \f$t\f$ as follows:
 * \f$\mathbf{p}_i^t = \mathbf{Q}_i(\mathbf{p}_i^0)\f$
 * , where \f$\mathbf{Q}_i = \frac{\sum_{s\in S} \omega_{is}\mathbf{Q}_s}
 *                                {\|\sum_{s\in S} \omega_{is}\mathbf{Q}_s\|}\f$
 *
 * The skinning of the normal, tangent and bitangent vectors is approximated as:
 * \f$\mathbf{v}_i^t = \mathbf{Q}_i(\mathbf{v}_i^0)\f$
 *
 * \note Assumes frameData is well sized.
 * \note Parallelized loop inside (using openmp).
 */
// clang-format on
void RA_CORE_API dualQuaternionSkinning( const SkinningRefData& refData,
                                         const Vector3Array& tangents,
                                         const Vector3Array& bitangents,
                                         SkinningFrameData& frameData );
/// \}

} // namespace Animation
} // namespace Core
} // namespace Ra
