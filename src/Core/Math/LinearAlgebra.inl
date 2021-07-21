#pragma once
#include <Core/Math/LinearAlgebra.hpp>

namespace Ra {
namespace Core {
namespace Math {
inline void print( const MatrixN& matrix ) {
    // Taken straight from :
    // http://eigen.tuxfamily.org/dox/structEigen_1_1IOFormat.html

    std::cout << "Matrix rows : " << matrix.rows() << std::endl;
    std::cout << "Matrix cols : " << matrix.cols() << std::endl;
    Eigen::IOFormat cleanFormat( 4, 0, ", ", "\n", "[", "]" );
    std::cout << matrix.format( cleanFormat ) << std::endl;
}

//
// Vector functions.
//

template <typename Vector_>
inline Vector_ floor( const Vector_& v ) {
    using Scalar_ = typename Vector_::Scalar;
    return v.unaryExpr(
        std::function<Scalar_( Scalar_ )>( static_cast<Scalar_ ( & )( Scalar_ )>( std::floor ) ) );
}

template <typename Vector_>
inline Vector_ ceil( const Vector_& v ) {
    using Scalar_ = typename Vector_::Scalar;
    return v.unaryExpr(
        std::function<Scalar_( Scalar_ )>( static_cast<Scalar_ ( & )( Scalar_ )>( std::ceil ) ) );
}

template <typename Vector_>
inline Vector_ trunc( const Vector_& v ) {
    using Scalar_ = typename Vector_::Scalar;
    return v.unaryExpr(
        std::function<Scalar_( Scalar_ )>( static_cast<Scalar_ ( & )( Scalar_ )>( std::trunc ) ) );
}

template <typename Derived, typename DerivedA, typename DerivedB>
inline typename Derived::PlainMatrix clamp( const Eigen::MatrixBase<Derived>& v,
                                            const Eigen::MatrixBase<DerivedA>& min,
                                            const Eigen::MatrixBase<DerivedB>& max ) {
    return v.cwiseMin( max ).cwiseMax( min );
}

template <typename Derived>
inline typename Derived::PlainMatrix
clamp( const Eigen::MatrixBase<Derived>& v, const Scalar& min, const Scalar& max ) {
    return v.unaryExpr( [min, max]( Scalar x ) { return std::clamp( x, min, max ); } );
}

template <typename Vector_>
inline Scalar angle( const Vector_& v1, const Vector_& v2 ) {
    return std::atan2( v1.cross( v2 ).norm(), v1.dot( v2 ) );
}

template <typename Vector_>
Scalar cotan( const Vector_& v1, const Vector_& v2 ) {
    return v1.dot( v2 ) / v1.cross( v2 ).norm();
}

template <typename Vector_>
Scalar cos( const Vector_& v1, const Vector_& v2 ) {
    return ( v1.dot( v2 ) ) / std::sqrt( v1.normSquared() * v2.normSquared() );
}

template <typename Vector_>
Scalar getNormAndNormalize( Vector_& v ) {
    const Scalar norm = v.norm();
    v /= norm;
    return norm;
}

template <typename Vector_>
Scalar getNormAndNormalizeSafe( Vector_& v ) {
    const Scalar norm = v.norm();
    if ( norm > 0 ) { v /= norm; }
    return norm;
}

template <typename Scalar>
Eigen::ParametrizedLine<Scalar, 3>
transformRay( const Eigen::Transform<Scalar, 3, Eigen::Affine>& t,
              const Eigen::ParametrizedLine<Scalar, 3>& r ) {
    return { t * r.origin(), t.linear() * r.direction() };
}

//
// Quaternion functions.
//

inline Quaternion add( const Quaternion& q1, const Quaternion& q2 ) {
    return Quaternion( q1.coeffs() + q2.coeffs() );
}

inline Quaternion addQlerp( const Quaternion& q1, const Quaternion& q2 ) {
    const Scalar sign = Ra::Core::Math::signNZ( q1.dot( q2 ) );
    return Quaternion( q1.coeffs() + ( sign * q2.coeffs() ) );
}

inline Quaternion scale( const Quaternion& q, Scalar k ) {
    return Quaternion( k * q.coeffs() );
}

inline void
getOrthogonalVectors( const Vector3& fx, Eigen::Ref<Vector3> fy, Eigen::Ref<Vector3> fz ) {
    // taken from [Duff et al. 17] Building An Orthonormal Basis, Revisited. JCGT. 2017
    Scalar sign    = std::copysign( Scalar( 1.0 ), fx( 2 ) );
    const Scalar a = Scalar( -1.0 ) / ( sign + fx( 2 ) );
    const Scalar b = fx( 0 ) * fx( 1 ) * a;
    fy             = Ra::Core::Vector3(
        Scalar( 1.0 ) + sign * fx( 0 ) * fx( 0 ) * a, sign * b, -sign * fx( 0 ) );
    fz = Ra::Core::Vector3( b, sign + fx( 1 ) * fx( 1 ) * a, -fx( 1 ) );
}

inline Vector3
projectOnPlane( const Vector3& planePos, const Vector3& planeNormal, const Vector3& point ) {
    return point + planeNormal * ( planePos - point ).dot( planeNormal );
}

template <typename Vector_>
Vector_ slerp( const Vector_& v1, const Vector_& v2, Scalar t ) {
    const Scalar dot          = v1.dot( v2 );
    const Scalar theta        = t * angle( v1, v2 );
    const Vector_ relativeVec = ( v2 - v1 * dot ).normalized();
    return ( ( v1 * std::cos( theta ) ) + ( relativeVec * std::sin( theta ) ) );
}

// Reference : Paolo Baerlocher, Inverse Kinematics techniques for interactive posture control
// of articulated figures (PhD Thesis), p.138. EPFL, 2001.
// http://www0.cs.ucl.ac.uk/research/equator/papers/Documents2002/Paolo%20Baerlocher_Thesis_2001.pdf
inline void getSwingTwist( const Quaternion& in, Quaternion& swingOut, Quaternion& twistOut ) {
    // singular case.
    if ( UNLIKELY( in.z() == 0 && in.w() == 0 ) ) {
        twistOut = in;
        swingOut.setIdentity();
    }
    else {
        const Scalar gamma = std::atan2( in.z(), in.w() );
        // beta = atan2 ( sqrt ( in.x^2 + in.y^2), sqrt ( in.z^2 + in.w^2))
        const Scalar beta =
            std::atan2( in.coeffs().head<2>().norm(), in.coeffs().tail<2>().norm() );

        // k = 2 / sinc(beta) = 2 / ( sin(beta)/ beta))
        const Scalar k = 2.f * beta / std::sin( beta );

        // The parentheses here are important. because otherwise k * rotation2D would
        // be evaluated first (which would yield the rotation of angle k * gamma).
        const Vector2 sxy = k * ( Eigen::Rotation2D<Scalar>( gamma ) * in.coeffs().head<2>() );

        AngleAxis twist( 2 * gamma, Vector3::UnitZ() );
        twistOut = twist;

        Vector3 swingAxis( Vector3::Zero() );
        swingAxis.head<2>() = sxy.normalized();

        AngleAxis swing( sxy.norm(), swingAxis );
        swingOut = swing;
    }
}

// http://stackoverflow.com/a/13786235/4717805
inline Matrix4 lookAt( const Vector3& position, const Vector3& target, const Vector3& up ) {
    Matrix3 R;
    R.col( 2 ) = ( position - target ).normalized();
    R.col( 0 ) = up.cross( R.col( 2 ) ).normalized();
    R.col( 1 ) = R.col( 2 ).cross( R.col( 0 ) );

    Matrix4 result                = Matrix4::Zero();
    result.topLeftCorner<3, 3>()  = R.transpose();
    result.topRightCorner<3, 1>() = -R.transpose() * position;
    result( 3, 3 )                = 1.0;

    return result;
}

inline Matrix4 perspective( Scalar fovy, Scalar aspect, Scalar znear, Scalar zfar ) {
    Scalar theta  = fovy * 0.5f;
    Scalar range  = zfar - znear;
    Scalar invtan = 1.f / std::tan( theta );

    Matrix4 result = Matrix4::Zero();
    result( 0, 0 ) = invtan / aspect;
    result( 1, 1 ) = invtan;
    result( 2, 2 ) = -( znear + zfar ) / range;
    result( 3, 2 ) = -1.f;
    result( 2, 3 ) = -2 * znear * zfar / range;
    result( 3, 3 ) = 0.0;

    return result;
}

inline Matrix4 orthographic( Scalar l, Scalar r, Scalar b, Scalar t, Scalar n, Scalar f ) {
    Matrix4 result = Matrix4::Zero();

    result( 0, 0 ) = 2.f / ( r - l );
    result( 1, 1 ) = 2.f / ( t - b );
    result( 2, 2 ) = -2.f / ( f - n );
    result( 3, 3 ) = 1.f;

    result( 0, 3 ) = -( r + l ) / ( r - l );
    result( 1, 3 ) = -( t + b ) / ( t - b );
    result( 2, 3 ) = -( f + b ) / ( f - n );

    return result;
}

template <typename Matrix_>
bool checkInvalidNumbers( Eigen::Ref<const Matrix_> matrix, const bool FAIL_ON_ASSERT ) {
    bool ok = false;
    matrix
        .unaryExpr( [&ok, FAIL_ON_ASSERT]( Scalar x ) {
            ok |= !std::isnormal( x );
            if ( !ok ) {
                if ( FAIL_ON_ASSERT ) { CORE_ASSERT( false, "At least an element is nan" ); }
            }
            return 1;
        } )
        .eval();

    return !ok;
}

} // namespace Math
} // namespace Core
} // namespace Ra

// Insert quaternions operator in Eigen namespace to allow functions outside of
// Ra::Core to use them without explicit calls. This is syntactic sugar,
// but allows to write expressions such as q = ( a*q1 + b*q2 ).normalized();
// anywhere in the code.
// See

namespace Eigen {
inline Quaternion<Scalar> operator*( Scalar k, const Quaternion<Scalar>& q ) {
    return Ra::Core::Math::scale( q, k );
}

inline Quaternion<Scalar> operator*( const Quaternion<Scalar>& q, Scalar k ) {
    return Ra::Core::Math::scale( q, k );
}

inline Quaternion<Scalar> operator+( const Quaternion<Scalar>& q1, const Quaternion<Scalar>& q2 ) {
    return Ra::Core::Math::add( q1, q2 );
}

inline Quaternion<Scalar> operator/( const Quaternion<Scalar>& q, Scalar k ) {
    return Ra::Core::Math::scale( q, Scalar( 1 ) / k );
}
} // namespace Eigen
