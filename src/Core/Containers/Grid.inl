#pragma once
#include "Grid.hpp"

namespace Ra {
namespace Core {
// Anonymous helper functions to convert to/from multi-dimensional indices
// If we want to  make the grid row-major we just need to overload these functions
// by reversing the loops.
namespace {

template <typename T, uint D>
inline typename Grid<T, D>::IdxVector
linearToIdxVector( uint linIdx, const typename Grid<T, D>::IdxVector& size ) {
    typename Grid<T, D>::IdxVector result = Grid<T, D>::IdxVector::Zero();

    for ( uint i = 0; i < D; ++i ) {
        result[i] = linIdx % size[i];
        linIdx    = linIdx / size[i];
    }
    return result;
}

template <typename T, uint D>
inline uint idxVectorToLinear( const typename Grid<T, D>::IdxVector& vecIdx,
                               const typename Grid<T, D>::IdxVector& size ) {
    uint result  = 0;
    uint dimProd = 1;
    for ( uint i = 0; i < D; ++i ) {
        result += vecIdx[i] * dimProd;
        dimProd *= size[i];
    }
    return result;
}
} // namespace

//
// Constructors
//

template <typename T, uint D>
Grid<T, D>::Grid( const typename Grid<T, D>::IdxVector& size, const T& val ) :
    m_size( size ), m_data( size.prod(), val ) {}

template <typename T, uint D>
Grid<T, D>::Grid( const typename Grid<T, D>::IdxVector& size, const T* values ) :
    m_size( size ), m_data( size.prod(), values ) {}
//
// Vector size and data management.
//

template <typename T, uint D>
inline uint Grid<T, D>::size() const {
    CORE_ASSERT( m_data.size() == m_size.prod(), "Inconsistent grid size" );
    return m_data.size();
}

template <typename T, uint D>
inline const typename Grid<T, D>::IdxVector& Grid<T, D>::sizeVector() const {
    CORE_ASSERT( m_data.size() == m_size.prod(), "Inconsistent grid size" );
    return m_size;
}

template <typename T, uint D>
inline bool Grid<T, D>::empty() const {
    CORE_ASSERT( m_data.size() == m_size.prod(), "Inconsistent grid size" );
    return m_data.empty();
}

template <typename T, uint D>
inline void Grid<T, D>::clear() {
    m_data.clear();
    m_size = IdxVector::Zero();
    CORE_ASSERT( empty(), "Inconsistent grid" );
}

template <typename T, uint D>
inline const T* Grid<T, D>::data() const {
    return m_data.data();
}

template <typename T, uint D>
inline T* Grid<T, D>::data() {
    return m_data.data();
}

//
// Individual element access.
//

template <typename T, uint D>
inline const T& Grid<T, D>::at( const typename Grid<T, D>::IdxVector& idx ) const {
    const uint linIdx = idxVectorToLinear<T, D>( idx, sizeVector() );
    CORE_ASSERT( linIdx < m_data.size(), "Invalid vector index" );
    return m_data[linIdx];
}

template <typename T, uint D>
inline T& Grid<T, D>::at( const typename Grid<T, D>::IdxVector& idx ) {
    const uint linIdx = idxVectorToLinear<T, D>( idx, sizeVector() );
    CORE_ASSERT( linIdx < m_data.size(), "Invalid vector index" );
    return m_data[linIdx];
}

template <typename T, uint D>
inline const T& Grid<T, D>::at( uint idx ) const {
    CORE_ASSERT( idx < m_data.size(), "Invalid vector index" );
    return m_data[idx];
}

template <typename T, uint D>
inline T& Grid<T, D>::at( uint idx ) {
    CORE_ASSERT( idx < m_data.size(), "Invalid vector index" );
    return m_data[idx];
}

template <typename T, uint D>
const T& Grid<T, D>::at( const typename Grid<T, D>::Iterator& it ) const {
    CORE_ASSERT( it.getGridSize() == m_size, "Incompatible iterator" );
    return at( it.getLinear() );
}

template <typename T, uint D>
T& Grid<T, D>::at( const typename Grid<T, D>::Iterator& it ) {
    CORE_ASSERT( it.getGridSize() == m_size, "Incompatible iterator" );
    return at( it.getLinear() );
}

//
// Iterators begin / end functions.
//

template <typename T, uint D>
inline typename Grid<T, D>::Iterator Grid<T, D>::begin() {
    return Iterator( *this );
}

template <typename T, uint D>
inline typename Grid<T, D>::Iterator Grid<T, D>::begin() const {
    return Iterator( *this );
}

template <typename T, uint D>
inline typename Grid<T, D>::Iterator Grid<T, D>::end() {
    return Iterator( *this, size() );
}

template <typename T, uint D>
inline typename Grid<T, D>::Iterator Grid<T, D>::end() const {
    return Iterator( *this, size() );
}

//
// Iterators construction
//

template <typename T, uint D>
inline Grid<T, D>::Iterator::Iterator( const typename Grid<T, D>::IdxVector& size, uint startIdx ) :
    m_sizes( size ) {
    setFromLinear( startIdx );
}

template <typename T, uint D>
inline Grid<T, D>::Iterator::Iterator( const typename Grid<T, D>::IdxVector& size,
                                       const typename Grid<T, D>::IdxVector& startIdx ) :
    m_sizes( size ) {
    setFromVector( startIdx );
}

template <typename T, uint D>
inline Grid<T, D>::Iterator::Iterator( const Grid<T, D>& grid, uint startIdx ) :
    m_sizes( grid.sizeVector() ) {
    setFromLinear( startIdx );
}

template <typename T, uint D>
inline Grid<T, D>::Iterator::Iterator( const Grid<T, D>& grid,
                                       const typename Grid<T, D>::IdxVector& startIdx ) :
    m_sizes( grid.sizeVector() ) {
    setFromVector( startIdx );
}

//
// Basic Iterator get/set
//

template <typename T, uint D>
inline void Grid<T, D>::Iterator::setFromLinear( uint i ) {
    m_index = i;
}

template <typename T, uint D>
inline void Grid<T, D>::Iterator::setFromVector( const typename Grid<T, D>::IdxVector& idx ) {
    m_index = idxVectorToLinear<T, D>( idx, m_sizes );
}

template <typename T, uint D>
inline uint Grid<T, D>::Iterator::getLinear() const {
    return m_index;
}

template <typename T, uint D>
inline typename Grid<T, D>::IdxVector Grid<T, D>::Iterator::getVector() const {
    return linearToIdxVector<T, D>( m_index, m_sizes );
}

template <typename T, uint D>
inline bool Grid<T, D>::Iterator::isOut() const {
    return !isIn();
}

template <typename T, uint D>
inline bool Grid<T, D>::Iterator::isIn() const {
    return m_index < m_sizes.prod();
}

//
// Iterator increment and decrement
//

template <typename T, uint D>
typename Grid<T, D>::Iterator& Grid<T, D>::Iterator::operator++() {
    m_index++;
    return *this;
}

template <typename T, uint D>
typename Grid<T, D>::Iterator& Grid<T, D>::Iterator::operator--() {
    m_index--;
    return *this;
}

template <typename T, uint D>
typename Grid<T, D>::Iterator Grid<T, D>::Iterator::operator++( int ) {
    Iterator copy( *this );
    ++( *this );
    return copy;
}

template <typename T, uint D>
typename Grid<T, D>::Iterator Grid<T, D>::Iterator::operator--( int ) {
    Iterator copy( *this );
    --( *this );
    return copy;
}

template <typename T, uint D>
typename Grid<T, D>::Iterator& Grid<T, D>::Iterator::operator+=( uint i ) {
    m_index += i;
    return *this;
}

template <typename T, uint D>
typename Grid<T, D>::Iterator& Grid<T, D>::Iterator::operator-=( uint i ) {
    m_index -= i;
    return *this;
}

template <typename T, uint D>
typename Grid<T, D>::Iterator&
Grid<T, D>::Iterator::operator+=( const typename Grid<T, D>::IdxVector& idx ) {
    CORE_ASSERT( isValidOffset( idx.template cast<int>() ), "Invalid offset vector." );
    setFromVector( getVector() + idx );
    return *this;
}

template <typename T, uint D>
typename Grid<T, D>::Iterator&
Grid<T, D>::Iterator::operator-=( const typename Grid<T, D>::IdxVector& idx ) {
    CORE_ASSERT( isValidOffset( -( idx.template cast<int>() ) ), "Invalid offset vector." );
    setFromVector( getVector() - idx );
    return *this;
}

template <typename T, uint D>
typename Grid<T, D>::Iterator&
Grid<T, D>::Iterator::operator+=( const typename Grid<T, D>::OffsetVector& idx ) {
    CORE_ASSERT( isValidOffset( idx ), "Invalid offset vector" );
    setFromVector( ( getVector().template cast<int>() + idx ).template cast<uint>() );
    return *this;
}

template <typename T, uint D>
bool Grid<T, D>::Iterator::operator==( const typename Grid<T, D>::Iterator& other ) const {
    CORE_ASSERT( m_sizes == other.m_sizes, "Comparing unrelated grid iterators" );
    return m_index == other.m_index;
}

template <typename T, uint D>
bool Grid<T, D>::Iterator::operator<( const typename Grid<T, D>::Iterator& other ) const {
    CORE_ASSERT( m_sizes == other.m_sizes, "Comparing unrelated grid iterators" );
    return m_index < other.m_index;
}

template <typename T, uint D>
const typename Grid<T, D>::IdxVector& Grid<T, D>::Iterator::getGridSize() const {
    return m_sizes;
}

template <typename T, uint D>
template <typename T2>
typename Grid<T2, D>::Iterator Grid<T, D>::Iterator::cast() const {
    return typename Grid<T2, D>::Iterator( m_sizes, m_index );
}

template <typename T, uint D>
bool Grid<T, D>::Iterator::isValidOffset( const typename Grid<T, D>::OffsetVector& idx ) {
    OffsetVector pos = getVector().template cast<int>() + idx;
    return !( ( pos.array() < 0 ).any() ||
              ( pos.array() >= m_sizes.template cast<int>().array() ).any() );
}

} // namespace Core
} // namespace Ra
