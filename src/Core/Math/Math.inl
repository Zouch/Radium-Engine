#pragma once
#include <Core/Math/Math.hpp>

namespace Ra {
namespace Core {
namespace Math {

inline constexpr Scalar toRadians( Scalar a ) {
    return toRad * a;
}

inline constexpr Scalar toDegrees( Scalar a ) {
    return toDeg * a;
}

template <typename Vector_Or_Scalar>
inline bool checkRange( const Vector_Or_Scalar& v, const Scalar& min, const Scalar& max ) {
    using std::clamp; // by default, use clamp from the std library
    return clamp( v, min, max ) == v;
}

template <typename T>
inline T ipow( const T& x, uint exp ) {
    if ( exp == 0 ) { return T( 1 ); }
    if ( exp == 1 ) { return x; }
    T p = ipow( x, exp / 2 );
    if ( ( exp % 2 ) == 0 ) { return p * p; }
    else {
        return p * p * x;
    }
}

/// This helper class is needed because C++ doesn't support function template
/// partial specialization.
namespace {
template <typename T, uint N>
struct IpowHelper {
    static inline constexpr T pow( const T& x ) {
        return ( N % 2 == 0 ) ? IpowHelper<T, N / 2>::pow( x ) * IpowHelper<T, N / 2>::pow( x )
                              : IpowHelper<T, N / 2>::pow( x ) * IpowHelper<T, N / 2>::pow( x ) * x;
    }
};

template <typename T>
struct IpowHelper<T, 1> {
    static inline constexpr T pow( const T& x ) { return x; }
};

template <typename T>
struct IpowHelper<T, 0> {
    static inline constexpr T pow( const T& /*x*/ ) { return T( 1 ); }
};

} // namespace

// Nb : T is last for automatic template argument deduction.
template <uint N, typename T>
inline constexpr T ipow( const T& x ) {
    return IpowHelper<T, N>::pow( x );
}

// Signum implementation that works for unsigned types.
template <typename T>
inline constexpr int signum( T x, std::false_type /*is_signed*/ ) {
    return T( 0 ) < x;
}

template <typename T>
inline constexpr int signum( T x, std::true_type /*is_signed*/ ) {
    return ( T( 0 ) < x ) - ( x < T( 0 ) );
}

template <typename T>
inline constexpr int sign( const T& val ) {
    return signum( val, std::is_signed<T>() );
}

template <typename T>
inline constexpr T signNZ( const T& val ) {
    return T( std::copysign( T( 1 ), val ) );
}

template <typename T>
inline constexpr T saturate( T v ) {
    return std::clamp( v, static_cast<T>( 0 ), static_cast<T>( 1 ) );
}

/// Implementation inspired from https://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
template <class T>
typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type
areApproxEqual( T x, T y, T espilonBoostFactor ) {
    return std::abs( x - y ) <= std::numeric_limits<T>::epsilon() * espilonBoostFactor
           // unless the result is subnormal
           || std::abs( x - y ) < std::numeric_limits<T>::min();
}

template <typename T>
constexpr T lerp( const T& a, const T& b, Scalar t ) {
    return ( 1 - t ) * a + t * b;
}
} // namespace Math
} // namespace Core
} // namespace Ra
