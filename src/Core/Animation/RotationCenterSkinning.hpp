#pragma once

#include <Core/RaCore.hpp>

#include <Core/Containers/VectorArray.hpp>

namespace Ra {
namespace Core {
namespace Animation {

struct SkinningRefData;
struct SkinningFrameData;

/**
 * \name Center-of-Rotation Skinning
 * Formula from: "Real-time Skeletal Skinning with Optimized Centers of Rotation"
 * 2016 - Le & Hodgins.
 */
/// \{

// clang-format off
/**
 * \brief Computes the per-vertex optimal center of rotations.
 *
 *
 * The optimal center of rotations are computed as:
 * \f$\mathbf{p}_i^* = \frac{\sum_{t}\varsigma(\mathbf{w}_i, \mathbf{w}_t)\mathcal{A}_t\mathbf{v}_t}
 *                          {\sum_{t}\varsigma(\mathbf{w}_i, \mathbf{w}_t)\mathcal{A}_t}\f$
 * , where \f$\mathbf{w}_i=\begin{bmatrix}\omega_{i0}&\omega_{i1}&\cdots&\omega_{in}\end{bmatrix}\f$
 * , \f$\mathbf{w}_t = \frac{1}{3}(\mathbf{w}_{t_0}+\mathbf{w}_{t_1}+\mathbf{w}_{t_2})\f$
 * and \f$\mathbf{v}_t = \frac{1}{3}(\mathbf{p}_{t_0}+\mathbf{p}_{t_1}+\mathbf{p}_{t_2})\f$
 * , \f$t_j\f$ being the \f$j\f$-th vertex of triangle \f$t\f$ and \f$\mathcal{A}_t\f$ its area.
 *
 * \note Parallelized loop inside (using openmp).
 */
// clang-format on
void RA_CORE_API computeCoR( SkinningRefData& dataInOut,
                             Scalar sigma         = 0.1_ra,
                             Scalar weightEpsilon = 0.1_ra );

// clang-format off
/**
 * \brief Applies Center-of-Rotation skinning to the current frame.
 *
 *
 * Center-of-Rotation skinning deforms the mesh geometry for frame \f$t\f$ as follows:
 * \f$\mathbf{p}_i^t = \sum_{s\in S} \omega_{is}\mathbf{M}_s\mathbf{p}_i^*
 *                   + \mathbf{R}(\mathbf{p}_i^0-\mathbf{p}_i^*)\f$
 * , where \f$\mathbf{R}\f$ is the rotation matrix given by the quaternion
 * \f$\mathbf{q} = \frac{\sum_{s\in S}^\oplus \omega_{is}\mathbf{q}_s}
 *                      {\|\sum_{s\in S}^\oplus \omega_{is}\mathbf{q}_s\|}\f$
 * and \f$\mathbf{p}_i^*\f$ is the Center-of-Rotation for vertex \f$\mathbf{p}_i\f$,
 * computed as:
 * \f$\mathbf{p}_i^* = \frac{\sum_{t}\varsigma(\mathbf{w}_i, \mathbf{w}_t)\mathcal{A}_t\mathbf{v}_t}
 *                          {\sum_{t}\varsigma(\mathbf{w}_i, \mathbf{w}_t)\mathcal{A}_t}\f$
 * , where \f$\mathbf{w}_i=\begin{bmatrix}\omega_{i0}&\omega_{i1}&\cdots&\omega_{in}\end{bmatrix}\f$
 * , \f$\mathbf{w}_t = \frac{1}{3}(\mathbf{w}_{t_0}+\mathbf{w}_{t_1}+\mathbf{w}_{t_2})\f$
 * and \f$\mathbf{v}_t = \frac{1}{3}(\mathbf{p}_{t_0}+\mathbf{p}_{t_1}+\mathbf{p}_{t_2})\f$
 * , \f$t_j\f$ being the \f$j\f$-th vertex of triangle \f$t\f$ and \f$\mathcal{A}_t\f$ its area.
 *
 * The skinning of the normal, tangent and bitangent vectors is approximated as:
 * \f$\mathbf{v}_i^t = \mathbf{R}\mathbf{v}_i^0\f$
 *
 *
 * \note Considers frameData is well sized.
 * \note Parallelized loop inside (using openmp).
 */
// clang-format on
void RA_CORE_API centerOfRotationSkinning( const SkinningRefData& refData,
                                           const Vector3Array& tangents,
                                           const Vector3Array& bitangents,
                                           SkinningFrameData& frameData );
/// \}

} // namespace Animation
} // namespace Core
} // namespace Ra
