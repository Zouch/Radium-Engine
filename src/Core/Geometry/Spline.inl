#pragma once
#include "Spline.hpp"

#include <Core/Math/Math.hpp>

#include <algorithm>

namespace Ra {
namespace Core {
namespace Geometry {
template <uint D, uint K>
inline Spline<D, K>::Spline( typename Spline<D, K>::Type type ) : m_type( type ) {
    static_assert( K >= 2, "Order must be at least two" );
    m_points.resize( K, Vector::Zero() );
    m_vecs.resize( K - 1, Vector::Zero() );
    m_node.resize( K + K, 0.f );
    assertSplines();
}

// -----------------------------------------------------------------------------

template <uint D, uint K>
inline void
Spline<D, K>::setCtrlPoints( const Core::VectorArray<typename Spline<D, K>::Vector>& points ) {
    m_points = points;
    m_vecs.resize( points.size() - 1, Vector::Zero() );
    for ( uint i = 0; i < m_vecs.size(); ++i ) {
        m_vecs[i] = m_points[i + 1] - m_points[i];
    }
    setNodalVector();
    assertSplines();

    for ( uint i = 0; i < m_vecs.size(); ++i ) {
        m_vecs[i] /= m_node[K + i] - m_node[i + 1];
    }
}

// -----------------------------------------------------------------------------

template <uint D, uint K>
inline const Core::VectorArray<typename Spline<D, K>::Vector>& Spline<D, K>::getCtrlPoints() const {
    return m_points;
}

// -----------------------------------------------------------------------------

/// The the nodal vector type
template <uint D, uint K>
inline void Spline<D, K>::setType( Type type ) {
    m_type = type;
    setNodalVector();
    assertSplines();
}

// -----------------------------------------------------------------------------

template <uint D, uint K>
inline typename Spline<D, K>::Vector Spline<D, K>::f( Scalar u ) const {
    u = std::clamp( u, Scalar( 0 ), Scalar( 1 ) );
    return eval( u, m_points, m_node, K );
}

// -----------------------------------------------------------------------------

template <uint D, uint K>
inline typename Spline<D, K>::Vector Spline<D, K>::df( Scalar u ) const {
    u = std::clamp( u, Scalar( 0 ), Scalar( 1 ) );
    return eval( u, m_vecs, m_node, K - 1, 1 ) * Scalar( K - 1 );
}

// -----------------------------------------------------------------------------

template <uint D, uint K>
inline void Spline<D, K>::assertSplines() const {
    CORE_ASSERT( m_points.size() >= K, "Not enough points" );
    CORE_ASSERT( m_node.size() == ( K + m_points.size() ), "Wrong nodal vector size" );
    CORE_ASSERT( m_points.size() == ( m_vecs.size() + 1 ), "Wrong point / diffs size" );
}

// -----------------------------------------------------------------------------

template <uint D, uint K>
inline void Spline<D, K>::setNodalVector() {
    switch ( m_type ) {
    case OPEN_UNIFORM:
        setNodeToOpenUniform();
        break;
    case UNIFORM:
        setNodeToUniform();
        break;
    }
}

// -----------------------------------------------------------------------------

template <uint D, uint K>
inline void Spline<D, K>::setNodeToUniform() {
    const uint n = m_points.size() - 1;
    m_node.resize( K + n + 1 );

    Scalar step = 1.f / Scalar( n - K + 2 );
    for ( uint i = 0; i < m_node.size(); ++i ) {
        m_node[i] = Scalar( i ) * step - step * ( Scalar )( K - 1 );
    }
}

// -----------------------------------------------------------------------------

template <uint D, uint K>
inline void Spline<D, K>::setNodeToOpenUniform() {
    m_node.resize( K + m_points.size() );

    uint acc = 1;
    for ( uint i = 0; i < m_node.size(); ++i ) {
        if ( i < K ) { m_node[i] = 0.f; }
        else if ( i >= ( m_points.size() + 1 ) ) {
            m_node[i] = 1.f;
        }
        else {
            m_node[i] = Scalar( acc ) / Scalar( m_points.size() + 1 - K );
            acc++;
        }
    }
}

// -----------------------------------------------------------------------------

template <uint D, uint K>
inline typename Spline<D, K>::Vector Spline<D, K>::eval( Scalar u,
                                                         const Core::VectorArray<Vector>& points,
                                                         const std::vector<Scalar>& node,
                                                         uint k,
                                                         int off ) {
    CORE_ASSERT( k >= 2, "K must be at least 2" );
    CORE_ASSERT( points.size() >= k, "Not enough points" );
    uint dec = 0;
    // TODO: better search with dichotomy ?
    // TODO: check for overflow
    while ( u > node[dec + k + off] ) {
        dec++;
    }

    // TODO: use buffers in attributes for better performances ?
    Core::VectorArray<Vector> pOut( k, Vector::Zero() );
    for ( uint i = dec, j = 0; i < ( dec + k ); ++i, ++j ) {
        pOut[j] = points[i];
    }

    std::vector<Scalar> nodeOut( k + k - 2, Scalar( 0 ) );
    for ( uint i = ( dec + 1 ), j = 0; i < ( dec + k + k - 1 ); ++i, ++j ) {
        nodeOut[j] = node[i + off];
    }
    return evalRec( u, pOut, nodeOut, k );
}

// -----------------------------------------------------------------------------

template <uint D, uint K>
inline typename Spline<D, K>::Vector Spline<D, K>::evalRec( Scalar u,
                                                            const Core::VectorArray<Vector>& points,
                                                            const std::vector<Scalar>& node,
                                                            uint k ) {
    if ( points.size() == 1 ) { return points[0]; }

    // TODO: use buffers in attributes for better performances ?
    Core::VectorArray<Vector> pOut( k - 1, Vector::Zero() );

    for ( uint i = 0; i < ( k - 1 ); ++i ) {
        const Scalar n0 = node[i + k - 1];
        const Scalar n1 = node[i];
        const Scalar f0 = ( n0 - u ) / ( n0 - n1 );
        const Scalar f1 = ( u - n1 ) / ( n0 - n1 );

        pOut[i] = points[i] * f0 + points[i + 1] * f1;
    }

    std::vector<Scalar> nodeOut( node.size() - 2 );

    for ( uint i = 1; i < node.size() - 1; ++i ) {
        nodeOut[i - 1] = node[i];
    }
    return evalRec( u, pOut, nodeOut, k - 1 );
}
} // namespace Geometry
} // namespace Core
} // namespace Ra
