#pragma once
#include <Core/Containers/Tex.hpp>
#include <Core/Math/LinearAlgebra.hpp>
#include <Core/Types.hpp>

namespace Ra {
namespace Core {

// Helper functions
namespace {
// This is a helper class for the texture fetch implementation. This interpolate linear
// interpolation from values at a corner of a grid.
template <uint N>
struct NLinearInterpolator {
    template <typename T>
    static T interpolate(
        const Grid<T, N>& grid,                    // grid from which values are read
        const typename Tex<T, N>::Vector& fact,    // factors of the interpolation (between 0 and 1)
        const typename Tex<T, N>::IdxVector& size, // size of the  dual grid
        const typename Tex<T, N>::IdxVector& clamped_nearest ) // base indices of the cell
    {
        CORE_ERROR( "N-linear interpolation not implemented for N= " << N );
        return T();
    }
};

template <>
struct NLinearInterpolator<2> {
    // bilinear interpolation in a quad cell
    template <typename T>
    static T interpolate( const Grid<T, 2>& grid,
                          const Vector2& fact,
                          const Vector2ui& size,
                          const Vector2ui& clamped_nearest ) {
        const uint i0 = clamped_nearest[0];
        const uint j0 = clamped_nearest[1];

        const uint i1 = i0 < size[0] ? i0 + 1 : i0;
        const uint j1 = j0 < size[1] ? j0 + 1 : j0;

        const T v00 = grid.at( { i0, j0 } );
        const T v01 = grid.at( { i0, j1 } );
        const T v10 = grid.at( { i1, j0 } );
        const T v11 = grid.at( { i1, j1 } );

        const T c0 = v00 * ( 1.0 - fact[0] ) + v10 * fact[0];
        const T c1 = v01 * ( 1.0 - fact[0] ) + v11 * fact[0];

        return c0 * ( 1.0 - fact[1] ) + c1 * fact[1];
    }
};

template <>
struct NLinearInterpolator<3> {
    // tri-linear interpolation in a cubic cell
    template <typename T>
    static T interpolate( const Grid<T, 3>& grid,
                          const Vector3& fact,
                          const Vector3ui& size,
                          const Vector3ui& clamped_nearest ) {
        const uint i0 = clamped_nearest[0];
        const uint j0 = clamped_nearest[1];
        const uint k0 = clamped_nearest[2];
        const uint i1 = i0 < size[0] ? i0 + 1 : i0;
        const uint j1 = j0 < size[1] ? j0 + 1 : j0;
        const uint k1 = k0 < size[2] ? k0 + 1 : k0;

        const T v000 = grid.at( { i0, j0, k0 } );
        const T v001 = grid.at( { i0, j0, k1 } );
        const T v010 = grid.at( { i0, j1, k0 } );
        const T v011 = grid.at( { i0, j1, k1 } );
        const T v100 = grid.at( { i1, j0, k0 } );
        const T v101 = grid.at( { i1, j0, k1 } );
        const T v110 = grid.at( { i1, j1, k0 } );
        const T v111 = grid.at( { i1, j1, k1 } );

        const T c00 = v000 * ( 1.0 - fact[0] ) + v100 * fact[0];
        const T c10 = v010 * ( 1.0 - fact[0] ) + v110 * fact[0];
        const T c01 = v001 * ( 1.0 - fact[0] ) + v101 * fact[0];
        const T c11 = v011 * ( 1.0 - fact[0] ) + v111 * fact[0];

        const T c0 = c00 * ( 1.0 - fact[1] ) + c10 * fact[1];
        const T c1 = c01 * ( 1.0 - fact[1] ) + c11 * fact[1];

        return c0 * ( 1.0 - fact[2] ) + c1 * fact[2];
    }
};
} // namespace

template <typename T, uint N>
Tex<T, N>::Tex( const IdxVector& resolution, const Vector& start, const Vector& end ) :
    Grid<T, N>( resolution ), m_aabb( start, end ) {
    const Vector quotient = ( resolution - IdxVector::Ones() ).template cast<Scalar>();
    m_cellSize            = m_aabb.sizes().cwiseQuotient( quotient );
}

template <typename T, uint N>
Tex<T, N>::Tex( const IdxVector& resolution, const AabbND& aabb ) :
    Grid<T, N>( resolution ), m_aabb( aabb ) {
    const Vector quotient = ( resolution - IdxVector::Ones() ).template cast<Scalar>();
    m_cellSize            = m_aabb.sizes().cwiseQuotient( quotient );
}

template <typename T, uint N>
inline const typename Tex<T, N>::AabbND& Tex<T, N>::getAabb() const {
    return m_aabb;
}

template <typename T, uint N>
inline T Tex<T, N>::fetch( const Vector& v ) const {
    Vector scaled_coords( ( v - m_aabb.min() ).cwiseQuotient( m_cellSize ) );
    // Sometimes due to float imprecision, a value of 0 is passed as -1e7
    // which floors incorrectly rounds down to -1, hence the use of trunc().
    Vector tmp = Ra::Core::Math::trunc( scaled_coords );
    CORE_ASSERT( !( ( tmp.array() < Vector::Zero().array() ).any() ), "Cannot cast to uint" );
    IdxVector nearest = tmp.template cast<uint>();
    Vector fact       = scaled_coords - tmp;

    // TODO: Give other texture behaviour (such as wrapping) ?

    IdxVector size =
        this->sizeVector() - IdxVector::Ones(); // TODO check this code on borders of the grid
    IdxVector clamped_nearest =
        Ra::Core::Math::clamp<IdxVector>( nearest, IdxVector::Zero(), size );

    return NLinearInterpolator<N>::interpolate( *this, fact, size, clamped_nearest );
}
} // namespace Core
} // namespace Ra
