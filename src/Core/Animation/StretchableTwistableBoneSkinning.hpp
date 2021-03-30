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

/** \name Stretchable Twistable Bone Skinning
 * Formula from: "Stretchable, Twistable Bones For Skeletal Shape Deformation" 2011 - Jacobson &
 * Sorkine.
 */
/// \{

/**
 * \brief Computes the weights for the STBS using simple projection on the bone.
 *
 * Weights are computed as follows:
 * $e_{is} = \frac{\|\mathbf{q}_i-\mathbf{s_a}\|}{\|\mathbf{s_b}-\mathbf{s_a}\|}$
 * , where $\mathbf{q}_i$ is the projection of point $\mathbf{p}_i$ on bone $s$.
 */
WeightMatrix RA_CORE_API computeSTBS_weights( const Vector3Array& inMesh,
                                              const Skeleton& skel );

/**
 * \brief Applies the LBS version of STBS to the current frame.
 *
 * The LBS version of STBS deforms the mesh geometry for frame $t$ as follows:
 * $\mathbf{p}_i^t = \sum_{s\in S} \omega_{is}[\mathbf{s_a}'+ \mathbf{R}_s\mathbf{K}_{is}
 *                                    (e_{is}\mathbf{e}_s+(-\mathbf{s_a}+\mathbf{p}_i^0))]$
 * , where $\mathbf{e}_s = (\frac{\|\mathbf{s_b}'-\mathbf{s_a}'\|}
 *                               {\|\mathbf{s_b}-\mathbf{s_a}\|} - 1)
 *                         (\mathbf{s_b}-\mathbf{s_a})$
 * , $\mathbf{s_*}'=\mathbf{M}_s\mathbf{s_*}$ and $\pt{K}_{is}$ is the twisting
 * rotation, function of the STBS weight $e_{is}$.
 *
 * The skinning of the normal, tangent and bitangent vectors is approximated as:
 * $\mathbf{v}_i^t = \sum_{s\in S}\omega_{is}\mathbf{R}_s\mathbf{K}_{is}\mathbf{v}_i^0$
 *
 * \note Assumes frameData is well sized.
 */
void RA_CORE_API linearBlendSkinningSTBS( const SkinningRefData& refData,
                                          const Vector3Array& tangents,
                                          const Vector3Array& bitangents,
                                          SkinningFrameData& frameData );

/**
 * \brief  Computes the per-vertex STBS dual quaternion.
 *
 * Such a dual quaternion is computed as:
 * $\mathbf{Q}_i = \frac{\sum_{s\in S} \omega_{is}\mathbf{Q}_{is}}
 *                      {\|\sum_{s\in S} \omega_{is}\mathbf{Q}_{is}\|}$
 * , where $\mathbf{Q}_{is}$ is the STBS dual quaternion w.r.t. bone $s$,
 * computed from the STBS translation and rotation:
 * $\mathcal{T}_{is} = \mathbf{s_a}'+
 *                     \mathbf{R}_s\mathbf{K}_{is}(e_{is}\mathbf{e}_s-\mathbf{s_a})$
 * $\mathcal{R}_{is} = \mathbf{R}_s\mathbf{K}_{is}]$
 *
 * \note The current pose must be given relatively to the reference pose.
 * \see R::Core::Animation::relativePose.
 */
DQList RA_CORE_API computeDQSTBS( const Pose& relPose,
                                  const Skeleton& poseSkel,
                                  const Skeleton& restSkel,
                                  const WeightMatrix& weight,
                                  const WeightMatrix& weightSTBS );

/**
 * \brief Applies the DQS version of STBS to the current frame.
 *
 * The DQS version of STBS deforms the mesh geometry for frame $t$ as follows:
 * $\mathbf{p}_i^t = \mathbf{Q}(\mathbf{p}_i^0)$
 * , where $\mathbf{Q}_s = \frac{\sum_{s\in S} \omega_{is}\mathbf{Q}_{is}}
 *                              {\|\sum_{s\in S} \omega_{is}\mathbf{Q}_{is}\|}$
 * , where $\mathbf{Q}_{is}$ is the STBS dual quaternion w.r.t. bone $s$,
 * computed from the STBS translation and rotation:
 * $\mathcal{T}_{is} = \mathbf{s_a}'+
 *                     \mathbf{R}_s\mathbf{K}_{is}(e_{is}\mathbf{e}_s-\mathbf{s_a})$
 * $\mathcal{R}_{is} = \mathbf{R}_s\mathbf{K}_{is}]$
 *
 * The skinning of the normal, tangent and bitangent vectors is approximated as:
 * $\mathbf{v}_i^t = \mathbf{Q}_i\mathbf{v}_i^0$
 *
 * \note Assumes frameData is well sized.
 */
void RA_CORE_API dualQuaternionSkinningSTBS( const SkinningRefData& refData,
                                             const Vector3Array& tangents,
                                             const Vector3Array& bitangents,
                                             SkinningFrameData& frameData );
/// \}

} // namespace Animation
} // namespace Core
} // namespace Ra
