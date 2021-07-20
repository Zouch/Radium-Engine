#pragma once
#include <Core/Geometry/PolyLine.hpp>
#include <Core/Math/LinearAlgebra.hpp> // cotan, saturate (from Math.hpp)

namespace Ra {
namespace Core {
namespace Geometry {

const Vector3Array& PolyLine::getPoints() const {
    return m_pts;
}

Scalar PolyLine::length() const {
    return m_lengths.back();
}

Aabb PolyLine::aabb() const {
    Aabb aabb;
    for ( const auto& v : m_pts ) {
        aabb.extend( v );
    }
    return aabb;
}

Scalar PolyLine::getLineParameter( uint segment, Scalar tSegment ) const {
    CORE_ASSERT( segment < m_ptsDiff.size(), "invalid segment index" );
    const Scalar lprev    = segment > 0 ? m_lengths[segment - 1] : 0;
    const Scalar lSegment = m_lengths[segment] - lprev;
    return ( ( lSegment * tSegment ) + lprev ) / length();
}

void PolyLine::getSegment( uint segment, Vector3& aOut, Vector3& abOut ) const {
    CORE_ASSERT( segment < m_ptsDiff.size(), "Invalid segment index." );
    aOut  = m_pts[segment];
    abOut = m_ptsDiff[segment];
}

const Vector3Array& PolyLine::getSegmentVectors() const {
    return m_ptsDiff;
}

uint PolyLine::getSegmentIndex( Scalar t ) const {
    // This could be optimized by dichotomy
    Scalar param = length() * Math::saturate( t );
    uint i       = 0;
    while ( m_lengths[i] < param ) {
        ++i;
    }
    return i;
}

} // namespace Geometry
} // namespace Core
} // namespace Ra
