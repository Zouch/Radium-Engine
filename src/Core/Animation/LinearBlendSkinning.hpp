#pragma once

#include <RaCore.hpp>
#include <Core/Containers/VectorArray.hpp>

namespace Ra {
namespace Core {
namespace Animation {

struct SkinningRefData;
struct SkinningFrameData;

/**
 * \brief Applies Linear-Blend skinning to the current frame.
 *
 * Linear-Blend skinning deforms the mesh geometry for frame $t$ as follows:
 * $\mathbf{p}_i^t = \sum_{s\in S}\omega_{is}\mathbf{M}_s\mathbf{p}_i^0$
 *
 * The skinning of the normal, tangent and bitangent vectors is approximated as:
 * $\mathbf{v}_i^t = \sum_{s\in S}\omega_{is}\mathbf{R}_s\mathbf{v}_i^0$
 *
 * \note Assumes frameData is well sized.
 */
void RA_CORE_API linearBlendSkinning( const SkinningRefData& refData,
                                      const Vector3Array& tangents,
                                      const Vector3Array& bitangents,
                                      SkinningFrameData& frameData );

} // namespace Animation
} // namespace Core
} // namespace Ra
