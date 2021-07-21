#pragma once

#include <Core/Asset/AssetData.hpp>
#include <Core/Types.hpp>
#include <Core/Utils/Color.hpp>

namespace Ra {
namespace Core {
namespace Geometry {
class AbstractVolume;
} // namespace Geometry

namespace Asset {

/**
 * Participating media defined by a grid density
 */
struct VolumeData : public AssetData {
    inline VolumeData( const std::string& name = "" ) : AssetData( name ) {}
    /// The underlaying density matrix
    Geometry::AbstractVolume* volume { nullptr };
    /// Absortion coefficient of the volume (default is Air)
    Utils::Color sigma_a { 0.0011_ra, 0.0024_ra, 0.014_ra };
    /// Scattering coefficient of the volume (default is Air)
    Utils::Color sigma_s { 2.55_ra, 3.21_ra, 3.77_ra };

    /// The bounding box of the volume
    Aabb boundingBox { Vector3 { 0_ra, 0_ra, 0_ra }, Vector3 { 1_ra, 1_ra, 1_ra } };
    /// Transformation matrix to go from the associated geometry frame to the
    /// canonical [0, 1]^3 density domain
    Transform densityToModel { Transform::Identity() };
    /// Transformation matrix of the object
    Transform modelToWorld { Transform::Identity() };
};

} // namespace Asset
} // namespace Core
} // namespace Ra
