#pragma once

#include <Core/RaCore.hpp>

#include <Core/Containers/VectorArray.hpp>

namespace Ra {
namespace Core {
namespace Animation {

struct SkinningRefData;
struct SkinningFrameData;

/** \name Center-of-Rotation Skinning
 * Formula from: "Real-time Skeletal Skinning with Optimized Centers of Rotation"
 * 2016 - Le & Hodgins.
 */
/// \{

/**
 * \brief Computes the per-vertex optimal center of rotations.
 *
 * The optimal center of rotations are computed as:
 * $\mathbf{p}_i^*$ = \frac{\sum_{t}\varsigma(\mathbf{w}_i, \mathbf{w}_t)\mathcal{A}_t\mathbf{v}_t}
 *                         {\sum_{t}\varsigma(\mathbf{w}_i, \mathbf{w}_t)\mathcal{A}_t}$
 * , where $\mathbf{w}_i=\begin{bmatrix}\omega_{i0}&\omega_{i1}&\cdots&\omega_{in}\end{bmatrix}$
 * , $\mathbf{w}_t = \frac{1}{3}(\mathbf{w}_{t_0}+\mathbf{w}_{t_1}+\mathbf{w}_{t_2})$
 * and $\mathbf{v}_t = \frac{1}{3}(\mathbf{p}_{t_0}+\mathbf{p}_{t_1}+\mathbf{p}_{t_2})$
 * , $t_j$ being the $j$-th vertex of triangle $t$ and $\mathcal{A}_t$ its area.
 */
void RA_CORE_API computeCoR( SkinningRefData& dataInOut,
                             Scalar sigma         = 0.1_ra,
                             Scalar weightEpsilon = 0.1_ra );

/**
 * \brief Applies Center-of-Rotation skinning to the current frame.
 *
 * Center-of-Rotation skinning deforms the mesh geometry for frame $t$ as follows:
 * $\mathbf{p}_i^t = \sum_{s\in S} \omega_{is}\mathbf{M}_s\mathbf{p}_i^*
 *                 + \mathbf{R}(\mathbf{p}_i^0-\mathbf{p}_i^*)$
 * , where $\mathbf{R}$ is the rotation matrix given by the quaternion
 * $\mathbf{q} = \frac{\sum_{s\in S}^\oplus \omega_{is}\mathbf{q}_s}
 *                    {\|\sum_{s\in S}^\oplus \omega_{is}\mathbf{q}_s\|}$
 * and $\mathbf{p}_i^*$ is the Center-of-Rotation for vertex $\mathbf{p}_i$, computed as:
 * $\mathbf{p}_i^*$ = \frac{\sum_{t}\varsigma(\mathbf{w}_i, \mathbf{w}_t)\mathcal{A}_t\mathbf{v}_t}
 *                         {\sum_{t}\varsigma(\mathbf{w}_i, \mathbf{w}_t)\mathcal{A}_t}$
 * , where $\mathbf{w}_i=\begin{bmatrix}\omega_{i0}&\omega_{i1}&\cdots&\omega_{in}\end{bmatrix}$
 * , $\mathbf{w}_t = \frac{1}{3}(\mathbf{w}_{t_0}+\mathbf{w}_{t_1}+\mathbf{w}_{t_2})$
 * and $\mathbf{v}_t = \frac{1}{3}(\mathbf{p}_{t_0}+\mathbf{p}_{t_1}+\mathbf{p}_{t_2})$
 * , $t_j$ being the $j$-th vertex of triangle $t$ and $\mathcal{A}_t$ its area.
 *
 * The skinning of the normal, tangent and bitangent vectors is approximated as:
 * $\mathbf{v}_i^t = \mathbf{R}\mathbf{v}_i^0$
 *
 * \note Considers frameData is well sized.
 */
void RA_CORE_API centerOfRotationSkinning( const SkinningRefData& refData,
                                           const Vector3Array& tangents,
                                           const Vector3Array& bitangents,
                                           SkinningFrameData& frameData );
/// \}

} // namespace Animation
} // namespace Core
} // namespace Ra
