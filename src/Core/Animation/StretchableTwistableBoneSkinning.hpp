#pragma once

#include <Core/Animation/HandleWeight.hpp>
#include <Core/Animation/Pose.hpp>
#include <Core/Containers/AlignedStdVector.hpp>
#include <Core/Containers/VectorArray.hpp>
#include <Core/Math/DualQuaternion.hpp>

namespace Ra {
namespace Core {
namespace Animation {

class Skeleton;
struct SkinningRefData;
struct SkinningFrameData;

using DQList = AlignedStdVector<DualQuaternion>;

/**
 * \name Stretchable Twistable Bone Skinning
 * Formula from: "Stretchable, Twistable Bones For Skeletal Shape Deformation" 2011 - Jacobson &
 * Sorkine.
 */
/// \{

// clang-format off
/**
 * \brief Computes the weights for the STBS using simple projection on the bone.
 *
 * Weights are computed as follows:
 * \f$e_{is} = \frac{\|\mathbf{q}_i-\mathbf{s_a}\|}{\|\mathbf{s_b}-\mathbf{s_a}\|}\f$
 * , where \f$\mathbf{q}_i\f$ is the projection of point \f$\mathbf{p}_i\f$ on bone \f$s\f$.
 */
// clang-format on
WeightMatrix RA_CORE_API computeSTBS_weights( const Vector3Array& inMesh, const Skeleton& skel );

// clang-format off
/**
 * \brief Applies the LBS version of STBS to the current frame.
 *
 * The LBS version of STBS deforms the mesh geometry for frame \f$t\f$ as follows:
 * \f$\mathbf{p}_i^t = \sum_{s\in S} \omega_{is}[\mathbf{s_a}'+ \mathbf{R}_s\mathbf{K}_{is}
 *                                   (e_{is}\mathbf{e}_s+(-\mathbf{s_a}+\mathbf{p}_i^0))]\f$
 * , where \f$\mathbf{e}_s = (\frac{\|\mathbf{s_b}'-\mathbf{s_a}'\|}
 *                                 {\|\mathbf{s_b}-\mathbf{s_a}\|} - 1)
 *                           (\mathbf{s_b}-\mathbf{s_a})\f$
 * , \f$\mathbf{s_*}'=\mathbf{M}_s\mathbf{s_*}\f$ and \f$\mathbf{K}_{is}\f$ is the
 * twisting rotation, function of the STBS weight \f$e_{is}\f$.
 *
 * The skinning of the normal, tangent and bitangent vectors is approximated as:
 * \f$\mathbf{v}_i^t = \sum_{s\in S}\omega_{is}\mathbf{R}_s\mathbf{K}_{is}\mathbf{v}_i^0\f$
 *
 * \note Assumes frameData is well sized.
 * \note Parallelized loop inside (using openmp).
 */
// clang-format on
void RA_CORE_API linearBlendSkinningSTBS( const SkinningRefData& refData,
                                          const Vector3Array& tangents,
                                          const Vector3Array& bitangents,
                                          SkinningFrameData& frameData );

// clang-format off
/**
 * \brief  Computes the per-vertex STBS dual quaternion.
 *
 * Such a dual quaternion is computed as:
 * \f$\mathbf{Q}_i = \frac{\sum_{s\in S} \omega_{is}\mathbf{Q}_{is}}
 *                        {\|\sum_{s\in S} \omega_{is}\mathbf{Q}_{is}\|}\f$
 * , where \f$\mathbf{Q}_{is}\f$ is the STBS dual quaternion w.r.t. bone \f$s\f$,
 * computed from the STBS translation and rotation:
 * \f$\mathcal{T}_{is} = \mathbf{s_a}'+
 *                       \mathbf{R}_s\mathbf{K}_{is}(e_{is}\mathbf{e}_s-\mathbf{s_a})\f$
 * and \f$\mathcal{R}_{is} = \mathbf{R}_s\mathbf{K}_{is}\f$
 *
 * \note Parallelized loop inside (using openmp).
 */
// clang-format on
DQList RA_CORE_API computeDQSTBS( const SkinningRefData& refData, const Skeleton& poseSkel );

// clang-format off
/**
 * \brief Applies the DQS version of STBS to the current frame.
 *
 * The DQS version of STBS deforms the mesh geometry for frame \f$t\f$ as follows:
 * \f$\mathbf{p}_i^t = \mathbf{Q}_i(\mathbf{p}_i^0)\f$
 * , where \f$\mathbf{Q}_i = \frac{\sum_{s\in S} \omega_{is}\mathbf{Q}_{is}}
 *                              {\|\sum_{s\in S} \omega_{is}\mathbf{Q}_{is}\|}\f$
 * , where \f$\mathbf{Q}_{is}\f$ is the STBS dual quaternion w.r.t. bone \f$s\f$,
 * computed from the STBS translation and rotation:
 * \f$\mathcal{T}_{is} = \mathbf{s_a}'+
 *                       \mathbf{R}_s\mathbf{K}_{is}(e_{is}\mathbf{e}_s-\mathbf{s_a})\f$
 * and \f$\mathcal{R}_{is} = \mathbf{R}_s\mathbf{K}_{is}\f$
 *
 * The skinning of the normal, tangent and bitangent vectors is approximated as:
 * \f$\mathbf{v}_i^t = \mathbf{Q}_i(\mathbf{v}_i^0)\f$
 *
 * \note Assumes frameData is well sized.
 * \note Parallelized loop inside (using openmp).
 */
// clang-format on
void RA_CORE_API dualQuaternionSkinningSTBS( const SkinningRefData& refData,
                                             const Vector3Array& tangents,
                                             const Vector3Array& bitangents,
                                             SkinningFrameData& frameData );
/// \}

} // namespace Animation
} // namespace Core
} // namespace Ra
