#include <Core/Geometry/PolyLine.hpp>

namespace Ra {
namespace Core {
namespace Geometry {

void PolyLine::update() {
    m_ptsDiff.clear();
    m_lengths.clear();
    CORE_ASSERT( m_pts.size() > 1, "Line must have at least two points" );
    m_ptsDiff.reserve( m_pts.size() - 1 );
    m_lengths.reserve( m_pts.size() - 1 );
    Scalar len = 0;
    for ( uint i = 0; i < m_pts.size() - 1; ++i ) {
        m_ptsDiff.push_back( m_pts[i + 1] - m_pts[i] );
        len += m_ptsDiff.back().norm();
        m_lengths.push_back( len );
    }
}

PolyLine::PolyLine( const Vector3Array& pts ) : m_pts( pts ) {
    update();
}

void PolyLine::setPoints( const Vector3Array& pts ) {
    m_pts = pts;
    update();
}

Scalar PolyLine::squaredDistance( const Vector3& p ) const {
    CORE_ASSERT( m_pts.size() > 1, "Line must have at least two points" );
    Scalar sqDist = std::numeric_limits<Scalar>::max();
    for ( uint i = 0; i < m_ptsDiff.size(); ++i ) {
        sqDist = std::min( Geometry::pointToSegmentSq( p, m_pts[i], m_ptsDiff[i] ), sqDist );
    }
    return sqDist;
}

Scalar PolyLine::distance( const Vector3& p ) const {
    return std::sqrt( squaredDistance( p ) );
}

Scalar PolyLine::projectOnSegment( const Vector3& p, uint segment ) const {
    CORE_ASSERT( segment < m_ptsDiff.size(), "invalid segment index" );
    const Scalar tSegment = Geometry::projectOnSegment( p, m_pts[segment], m_ptsDiff[segment] );
    return getLineParameter( segment, tSegment );
}

Scalar PolyLine::project( const Vector3& p ) const {
    CORE_ASSERT( m_pts.size() > 1, "Line must have at least two points" );
    Scalar sqDist = std::numeric_limits<Scalar>::max();
    uint segment  = 0;
    std::vector<Scalar> ts;
    std::vector<Scalar> ds;
    ts.reserve( m_ptsDiff.size() );
    ds.reserve( m_ptsDiff.size() );

    for ( uint i = 0; i < m_ptsDiff.size(); ++i ) {
        Scalar proj = Geometry::projectOnSegment( p, m_pts[i], m_ptsDiff[i] );
        Scalar d    = ( p - ( m_pts[i] + proj * ( m_ptsDiff[i] ) ) ).squaredNorm();
        ds.push_back( d );
        ts.push_back( proj );
        if ( d < sqDist ) {
            sqDist  = d;
            segment = i;
        }
    }

    CORE_ASSERT( segment < m_ptsDiff.size(), "Invalid index" );
    Scalar t = ts[segment];
    if ( t > 0 && t < 1 ) {
        bool prev = segment > 0 && ts[segment - 1] > 0 && ts[segment - 1] < 1;
        bool next = segment < m_ptsDiff.size() - 1 && ts[segment + 1] > 0 && ts[segment + 1] < 1;
        if ( prev || next ) {
            if ( prev && next ) { prev = ds[segment - 1] < ds[segment + 1]; }
            uint i     = prev ? segment - 1 : segment;
            Vector3 ba = -m_ptsDiff[i];
            Vector3 bc = m_ptsDiff[i + 1];
            Vector3 bp = p - m_pts[i + 1];
            Scalar c1  = Math::cotan( ba, bp );
            Scalar c2  = Math::cotan( bp, bc );

            Scalar t1 = getLineParameter( i, ts[i] );
            Scalar t2 = getLineParameter( i + 1, ts[i + 1] );
            return ( c1 * t1 + c2 * t2 ) / ( c1 + c2 );
        }
    }
    return getLineParameter( segment, t );
}

Vector3 PolyLine::f( Scalar t ) const {
    // Clamp the parameter between 0 and 1 and scale it.
    const Scalar param = length() * Math::saturate( t );

    // Try to locate the segment section where f(t) belongs.
    uint i       = 0;
    Scalar lprev = 0.f;
    while ( m_lengths[i] < param ) {
        lprev = m_lengths[i];
        ++i;
    }

    CORE_ASSERT( i < m_ptsDiff.size(), "Invalid index" );

    // Now we know point f(t) is between point Pi and Pi+1;
    return m_pts[i] + ( ( param - lprev ) / ( m_lengths[i] - lprev ) ) * m_ptsDiff[i];
}

uint PolyLine::getNearestSegment( const Vector3& p ) const {
    CORE_ASSERT( m_pts.size() > 1, "Line must have at least two points" );
    Scalar sqDist = std::numeric_limits<Scalar>::max();
    uint segment  = 0;
    for ( uint i = 0; i < m_ptsDiff.size(); ++i ) {
        Scalar proj = Geometry::projectOnSegment( p, m_pts[i], m_ptsDiff[i] );
        Scalar d    = ( p - ( m_pts[i] + proj * ( m_ptsDiff[i] ) ) ).squaredNorm();
        if ( d < sqDist ) {
            sqDist  = d;
            segment = i;
        }
    }

    CORE_ASSERT( segment < m_ptsDiff.size(), "Invalid index" );
    return segment;
}

} // namespace Geometry
} // namespace Core
} // namespace Ra
