#pragma once
#include <Core/Utils/IndexMap.hpp>

namespace Ra {
namespace Core {
namespace Utils {

// ===============================================================================
// CONSTRUCTOR
// ===============================================================================
template <typename T>
IndexMap<T>::IndexMap() : m_data(), m_index(), m_free( 1, Index( 0 ) ) {}

template <typename T>
IndexMap<T>::IndexMap( const IndexMap& id_map ) :
    m_data( id_map.m_data ), m_index( id_map.m_index ), m_free( id_map.m_free ) {}

// ===============================================================================
// DESTRUCTOR
// ===============================================================================
template <typename T>
IndexMap<T>::~IndexMap() {}

// ===============================================================================
// INSERT
// ===============================================================================
template <typename T>
inline Index IndexMap<T>::insert( const T& obj ) {
    Index idx;
    if ( pop_free_index( idx ) ) {
        typename std::deque<Index>::iterator it =
            std::lower_bound( m_index.begin(), m_index.end(), idx );
        if ( it == m_index.end() ) { m_data.insert( m_data.end(), obj ); }
        else {
            m_data.insert( citfromIndex( it ), obj );
        }
        m_index.insert( it, idx );
    }
    return idx;
}

template <typename T>
template <typename... Args>
Index IndexMap<T>::emplace( const Args&&... args ) {
    Index idx;
    if ( pop_free_index( idx ) ) {
        typename std::deque<Index>::iterator it =
            std::lower_bound( m_index.begin(), m_index.end(), idx );
        if ( it == m_index.end() ) { m_data.emplace( m_data.end(), args... ); }
        else {
            m_data.emplace( citfromIndex( it ), args... );
        }
        m_index.insert( it, idx );
    }
    return idx;
}

// ===============================================================================
// REMOVE
// ===============================================================================
template <typename T>
inline bool IndexMap<T>::remove( const Index& idx ) {
    typename std::deque<Index>::iterator it = std::find( m_index.begin(), m_index.end(), idx );
    if ( it == m_index.end() ) { return false; }
    m_data.erase( itfromIndex( it ) );
    m_index.erase( it );
    push_free_index( idx );
    return true;
}

// ===============================================================================
// ACCESS
// ===============================================================================
template <typename T>
inline const T& IndexMap<T>::at( const Index& idx ) const {
    typename std::deque<Index>::const_iterator it =
        std::find( m_index.begin(), m_index.end(), idx );
    CORE_ASSERT( it != m_index.end(), "Index not found" );
    return m_data.at( idxfromIndex( it ) );
}

template <typename T>
inline T& IndexMap<T>::access( const Index& idx ) {
    typename std::deque<Index>::iterator it = std::find( m_index.begin(), m_index.end(), idx );
    CORE_ASSERT( ( it != m_index.end() ), "Index not found" );
    return *itfromIndex( it );
}

// ===============================================================================
// SIZE
// ===============================================================================
template <typename T>
inline size_t IndexMap<T>::size() const {
    return m_data.size();
}

template <typename T>
inline void IndexMap<T>::clear() {
    m_index.clear();
    m_data.clear();
    m_free.clear();
    m_free.push_back( Index( 0 ) );
}

// ===============================================================================
// QUERY
// ===============================================================================
template <typename T>
inline bool IndexMap<T>::empty() const {
    return m_data.empty();
}

template <typename T>
inline bool IndexMap<T>::full() const {
    return m_free.empty();
}

template <typename T>
inline bool IndexMap<T>::contains( const Index& idx ) const {
    typename std::deque<Index>::const_iterator it =
        std::find( m_index.begin(), m_index.end(), idx );
    return !( it == m_index.end() );
}

template <typename T>
inline Index IndexMap<T>::index( const uint i ) const {
    if ( i >= m_index.size() ) { return Index::Invalid(); }
    return m_index.at( i );
}

// ===============================================================================
// OPERATOR
// ===============================================================================
template <typename T>
inline T& IndexMap<T>::operator[]( const Index& idx ) {
    return access( idx );
}

template <typename T>
inline const T& IndexMap<T>::operator[]( const Index& idx ) const {
    return at( idx );
}

// ===============================================================================
// INDEX ITERATOR
// ===============================================================================
template <typename T>
inline typename IndexMap<T>::ConstIndexIterator IndexMap<T>::cbegin_index() const {
    return m_index.cbegin();
}

template <typename T>
inline typename IndexMap<T>::ConstIndexIterator IndexMap<T>::cend_index() const {
    return m_index.cend();
}

// ===============================================================================
// DATA ITERATOR
// ===============================================================================
template <typename T>
inline typename IndexMap<T>::Iterator IndexMap<T>::begin() {
    return m_data.begin();
}

template <typename T>
inline typename IndexMap<T>::Iterator IndexMap<T>::end() {
    return m_data.end();
}

template <typename T>
inline typename IndexMap<T>::ConstIterator IndexMap<T>::begin() const {
    return m_data.begin();
}

template <typename T>
inline typename IndexMap<T>::ConstIterator IndexMap<T>::end() const {
    return m_data.end();
}

template <typename T>
inline typename IndexMap<T>::ConstIterator IndexMap<T>::cbegin() const {
    return m_data.cbegin();
}

template <typename T>
inline typename IndexMap<T>::ConstIterator IndexMap<T>::cend() const {
    return m_data.cend();
}

// ===============================================================================
// FREE LIST
// ===============================================================================
template <typename T>
inline void IndexMap<T>::push_free_index( const Index& idx ) {
    std::deque<Index>::iterator free_it = std::lower_bound( m_free.begin(), m_free.end(), idx );
    m_free.insert( free_it, idx );
    if ( m_data.empty() ) {
        m_free.clear();
        m_free.push_back( Index( 0 ) );
    }
}

template <typename T>
inline bool IndexMap<T>::pop_free_index( Index& idx ) {
    if ( m_free.empty() ) {
        idx = Index::Invalid();
        return false;
    }
    idx = m_free.front();
    m_free.pop_front();
    if ( m_free.empty() ) {
        Index next = idx + 1;
        if ( next.isValid() ) {
            if ( uint( next.getValue() ) > m_data.size() ) { m_free.push_back( next ); }
        }
    }
    return true;
}

// ===============================================================================
// HELPER FUNCTIONS
// ===============================================================================

template <typename T>
typename IndexMap<T>::ConstIterator
IndexMap<T>::citfromIndex( const typename IndexMap<T>::ConstIndexIterator& it ) const {
    auto dataIt = m_data.begin();
    std::advance( dataIt, std::distance( m_index.cbegin(), it ) );
    return dataIt;
}

template <typename T>
typename IndexMap<T>::Iterator
IndexMap<T>::itfromIndex( const typename IndexMap<T>::ConstIndexIterator& it ) {
    auto dataIt = m_data.begin();
    std::advance( dataIt, std::distance( m_index.cbegin(), it ) );
    return dataIt;
}

template <typename T>
size_t IndexMap<T>::idxfromIndex( const typename IndexMap<T>::ConstIndexIterator& it ) const {
    return std::distance( m_index.cbegin(), it );
}

} // namespace Utils
} // namespace Core
} // namespace Ra
