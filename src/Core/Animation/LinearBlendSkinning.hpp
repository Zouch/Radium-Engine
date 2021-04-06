#pragma once

#include <Core/Containers/VectorArray.hpp>
#include <RaCore.hpp>

namespace Ra {
namespace Core {
namespace Animation {

struct SkinningRefData;
struct SkinningFrameData;

/// \name Linear Blend Skinning
/// \{

// clang-format off
/**
 * \brief Applies Linear-Blend skinning to the current frame.
 *
 * Linear-Blend skinning deforms the mesh geometry for frame \f$t\f$ as follows:
 * \f$\mathbf{p}_i^t = \sum_{s\in S}\omega_{is}\mathbf{M}_s\mathbf{p}_i^0\f$
 *
 * The skinning of the normal, tangent and bitangent vectors is approximated as:
 * \f$\mathbf{v}_i^t = \sum_{s\in S}\omega_{is}\mathbf{R}_s\mathbf{v}_i^0\f$
 *
 * \note Assumes frameData is well sized.
 */
// clang-format on
void RA_CORE_API linearBlendSkinning( const SkinningRefData& refData,
                                      const Vector3Array& tangents,
                                      const Vector3Array& bitangents,
                                      SkinningFrameData& frameData );
/// \}

} // namespace Animation
} // namespace Core
} // namespace Ra
