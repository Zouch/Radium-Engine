#pragma once
#include <Core/Containers/AdjacencyList.hpp>

namespace Ra {
namespace Core {

/// SIZE
inline uint AdjacencyList::size() const {
    CORE_ASSERT( m_parent.size() == m_child.size(), "List size inconsistency" );
    return m_parent.size();
}

inline void AdjacencyList::clear() {
    m_child.clear();
    m_parent.clear();
}

/// QUERY
inline bool AdjacencyList::isEmpty() const {
    return ( size() == 0 );
}

inline bool AdjacencyList::isRoot( const uint i ) const {
    CORE_ASSERT( i < size(), " Index i out of bounds " );
    return ( m_parent[i] == -1 );
}

inline bool AdjacencyList::isLeaf( const uint i ) const {
    CORE_ASSERT( i < size(), " Index i out of bounds " );
    return ( m_child[i].size() == 0 );
}

inline bool AdjacencyList::isBranch( const uint i ) const {
    CORE_ASSERT( i < size(), " Index i out of bounds " );
    return ( m_child[i].size() > 1 );
}

inline bool AdjacencyList::isJoint( const uint i ) const {
    CORE_ASSERT( i < size(), " Index i out of bounds " );
    return ( m_child[i].size() == 1 );
}

inline bool AdjacencyList::isEdge( const uint i, const uint j ) const {
    CORE_ASSERT( i < size(), " Index i out of bounds " );
    CORE_ASSERT( j < size(), " Index j out of bounds " );
    for ( const auto& item : m_child[i] ) {
        if ( item == j ) return true;
    }
    return false;
}

inline const Adjacency& AdjacencyList::children() const {
    return m_child;
}

inline const ParentList& AdjacencyList::parents() const {
    return m_parent;
}

} // namespace Core
} // namespace Ra
