#pragma once

#include <Core/CoreMacros.hpp>
#include <Core/Math/Math.hpp>
#include <Eigen/Core>
#include <Eigen/Geometry> //homogeneous
#include <random>

namespace Ra {
namespace Core {
namespace Utils {

/*!
 * Colors are defined as vector4, i.e. 4 Scalars in RGBA order.
 * displayable colors should have all their coordinates between 0 and 1.
 *
 * \warning Vector arithmetics can be used to add, substract, multiply or divide colors with
 * scalars and other colors. In that case, alpha is treated as any other component. It is fine for
 * most cases, however adding two colors (e.g. Blue()+Red()) may lead to unconsistent
 * alpha: Blue()+Red() = {1, 0, 1, 2}.
 * In that case, only the rgb values need to be added:
 * \code
 * Color result = Color::fromRGB( Blue().rgb()+Red().rgb() ); // result = {1, 0, 1, 1}
 * \endcode
 * The validity of alpha can be checked using hasValidAlpha().
 */
template <typename _Scalar>
class ColorBase : public Eigen::Matrix<_Scalar, 4, 1>
{
  public:
    using VectorType = Eigen::Matrix<_Scalar, 4, 1>;

    explicit inline ColorBase() : ColorBase( _Scalar( 1. ), _Scalar( 1. ), _Scalar( 1. ) ) {}

    template <typename S2>
    inline ColorBase( S2 r, S2 g, S2 b, S2 alpha = S2( 1 ) ) :
        VectorType( _Scalar( r ), _Scalar( g ), _Scalar( b ), _Scalar( alpha ) ) {}

    /// Copy constructor
    template <typename S2>
    inline ColorBase( const ColorBase<S2>& other ) : VectorType( other.template cast<_Scalar>() ) {}

    /// Copy constructor from Eigen expressions: `ColorBase<_Scalar> c( (*this) * 255 );`
    template <typename Derived>
    inline ColorBase( const Eigen::MatrixBase<Derived>& v ) :
        VectorType( v.template cast<_Scalar>() ) {}

    /// cast operator, mandatory to use Vector arithmetic
    operator VectorType() { return *this; }

    /// convert the color expressed in sRGB color space to linear RGB
    static inline ColorBase sRGBToLinearRGB( const ColorBase& srgb ) {
        ColorBase<_Scalar> c( srgb );
        for ( auto& u : c.rgb() ) {
            if ( u < 0.04045_ra ) { u /= 12.92_ra; }
            else {
                u = std::pow( ( u + 0.055_ra ) / 1.055_ra, 2.4_ra );
            }
        }
        return c;
    }

    /// convert the color expressed in linear RGB color space to sRGB
    static inline ColorBase linearRGBTosRGB( const ColorBase& lrgb ) {
        ColorBase<_Scalar> c( lrgb );
        for ( auto& u : c.rgb() ) {
            if ( u < 0.0031308_ra ) { u *= 12.92_ra; }
            else {
                u = 1.055_ra * std::pow( u, 1_ra / 2.4_ra ) - 0.055_ra;
            }
        }
        return c;
    }

    template <typename Derived>
    static inline ColorBase fromRGB( const Eigen::MatrixBase<Derived>& rgb,
                                     Scalar alpha = Scalar( 1. ) ) {
        ColorBase c( rgb.template cast<_Scalar>().homogeneous() );
        c.alpha() = alpha;
        return c;
    }

    Eigen::Block<VectorType, 3, 1> rgb() { return ( *this ).template head<3>(); }
    const Eigen::Block<VectorType, 3, 1> rgb() const { return ( *this ).template head<3>(); }

    Scalar alpha() const { return ( *this )( 3 ); }
    Scalar& alpha() { return ( *this )( 3 ); }
    bool hasValidAlpha() const { return Math::checkRange( alpha(), 0_ra, 1_ra ); }

    static inline ColorBase<_Scalar> Alpha() {
        return ColorBase<_Scalar>( _Scalar( 0. ), _Scalar( 0. ), _Scalar( 0. ), _Scalar( 0. ) );
    }

    static inline ColorBase<_Scalar> Black() {
        return ColorBase<_Scalar>( _Scalar( 0. ), _Scalar( 0. ), _Scalar( 0. ) );
    }

    static inline ColorBase<_Scalar> Red() {
        return ColorBase<_Scalar>( _Scalar( 1. ), _Scalar( 0. ), _Scalar( 0. ) );
    }

    static inline ColorBase<_Scalar> Green() {
        return ColorBase<_Scalar>( _Scalar( 0. ), _Scalar( 1. ), _Scalar( 0. ) );
    }

    static inline ColorBase<_Scalar> Blue() {
        return ColorBase<_Scalar>( _Scalar( 0. ), _Scalar( 0. ), _Scalar( 1. ) );
    }

    static inline ColorBase<_Scalar> Yellow() {
        return ColorBase<_Scalar>( _Scalar( 1. ), _Scalar( 1. ), _Scalar( 0. ) );
    }

    static inline ColorBase<_Scalar> Magenta() {
        return ColorBase<_Scalar>( _Scalar( 1. ), _Scalar( 0. ), _Scalar( 1. ) );
    }

    static inline ColorBase<_Scalar> Cyan() {
        return ColorBase<_Scalar>( _Scalar( 0. ), _Scalar( 1. ), _Scalar( 1. ) );
    }

    static inline ColorBase<_Scalar> White() {
        return ColorBase<_Scalar>( _Scalar( 1. ), _Scalar( 1. ), _Scalar( 1. ) );
    }

    static inline ColorBase<_Scalar> Grey( _Scalar f = _Scalar( 0.5 ), _Scalar a = _Scalar( 1. ) ) {
        return ColorBase<_Scalar>( f, f, f, a );
    }

    static inline ColorBase<_Scalar> Skin() {
        return ColorBase<_Scalar>( _Scalar( 1.0 ), _Scalar( 0.87 ), _Scalar( 0.74 ) );
    }
    // Convert to/from various int formats

    static inline ColorBase<_Scalar> fromChars( uchar r, uchar g, uchar b, uchar a = 0xff ) {
        return ColorBase<_Scalar>( _Scalar( r ) / 255.0f,
                                   _Scalar( g ) / 255.0f,
                                   _Scalar( b ) / 255.0f,
                                   _Scalar( a ) / 255.0f );
    }

    static inline ColorBase<_Scalar> fromRGBA32( uint32_t rgba ) {
        uchar r = uchar( ( rgba >> 24 ) & 0xff );
        uchar g = uchar( ( rgba >> 16 ) & 0xff );
        uchar b = uchar( ( rgba >> 8 ) & 0xff );
        uchar a = uchar( ( rgba >> 0 ) & 0xff );
        return fromChars( r, g, b, a );
    }

    static inline ColorBase<_Scalar> fromARGB32( uint32_t argb ) {
        uchar a = uchar( ( argb >> 24 ) & 0xff );
        uchar r = uchar( ( argb >> 16 ) & 0xff );
        uchar g = uchar( ( argb >> 8 ) & 0xff );
        uchar b = uchar( ( argb >> 0 ) & 0xff );
        return fromChars( r, g, b, a );
    }

    static inline ColorBase<_Scalar> fromHSV( const _Scalar hue,
                                              const _Scalar saturation = 1.0,
                                              const _Scalar value      = 1.0,
                                              const _Scalar alpha      = 1.0 ) {
        ColorBase<_Scalar> c;

        if ( saturation == 0.0f ) {
            c[0] = c[1] = c[2] = value;
            c[3]               = alpha;
            return c;
        }
        _Scalar h  = ( ( hue == 1.0f ) ? 0.0f : hue ) * 6.0f;
        int i      = int( std::floor( h ) );
        _Scalar v1 = value * ( 1.0f - saturation );
        _Scalar v2 = value * ( 1.0f - ( saturation * ( h - i ) ) );
        _Scalar v3 = value * ( 1.0f - ( saturation * ( 1.0f - h - i ) ) );
        switch ( i ) {
        case 0: {
            c[0] = value;
            c[1] = v3;
            c[2] = v1;
        } break;
        case 1: {
            c[0] = v2;
            c[1] = value;
            c[2] = v1;
        } break;
        case 2: {
            c[0] = v1;
            c[1] = value;
            c[2] = v3;
        } break;
        case 3: {
            c[0] = v1;
            c[1] = v2;
            c[2] = value;
        } break;
        case 4: {
            c[0] = v3;
            c[1] = v1;
            c[2] = value;
        } break;
        default: {
            c[0] = value;
            c[1] = v1;
            c[2] = v2;
        } break;
        }
        c[3] = alpha;
        return c;
    }

    inline uint32_t toRGBA32() const {
        ColorBase<_Scalar> c( ( *this ) * 255 );
        Eigen::Matrix<int, 4, 1> scaled( c.x(), c.y(), c.z(), c.w() );
        return ( uint32_t( scaled( 0 ) ) << 24 ) | ( uint32_t( scaled( 1 ) ) << 16 ) |
               ( uint32_t( scaled( 2 ) ) << 8 ) | ( uint32_t( scaled( 3 ) ) << 0 );
    }

    inline uint32_t toARGB32() const {
        ColorBase<_Scalar> c( ( *this ) * 255 );
        Eigen::Matrix<int, 4, 1> scaled( c.x(), c.y(), c.z(), c.w() );
        return ( uint32_t( scaled( 3 ) ) << 24 ) | ( uint32_t( scaled( 0 ) ) << 16 ) |
               ( uint32_t( scaled( 1 ) ) << 8 ) | ( uint32_t( scaled( 2 ) ) << 0 );
    }

    static inline std::vector<ColorBase<_Scalar>> scatter( const uint size, const _Scalar gamma ) {
        std::vector<ColorBase<_Scalar>> color( size );
        if ( size > 1 )
            for ( uint i = 0; i < size; ++i ) {
                color[i] = fromHSV( ( _Scalar( i ) / _Scalar( size - 1 ) ) * 0.777 );
                color[i] = ( color[i] + ColorBase<_Scalar>::Constant( gamma ) ) * 0.5;
            }
        else {
            color[0] = Red();
        }
        std::shuffle( color.begin(), color.end(), std::mt19937( std::random_device()() ) );
        return color;
    }
};

using Color  = ColorBase<Scalar>;
using Colorf = ColorBase<float>;
using Colord = ColorBase<double>;

} // namespace Utils
} // namespace Core
} // namespace Ra
