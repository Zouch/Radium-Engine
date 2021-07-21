#include <Core/Animation/PoseOperation.hpp>
#include <Core/Math/Interpolation.hpp>

#include <Eigen/Geometry>

namespace Ra {
namespace Core {
namespace Animation {

bool compatible( const Pose& p0, const Pose& p1 ) {
    return ( p0.size() == p1.size() );
}

Pose relativePose( const Pose& modelPose, const RestPose& restPose ) {
    CORE_ASSERT( compatible( modelPose, restPose ), " Poses with different size " );
    Pose T( restPose.size() );
#pragma omp parallel for
    for ( int i = 0; i < int( T.size() ); ++i ) {
        T[i] = modelPose[i] * restPose[i].inverse( Eigen::Affine );
    }
    return T;
}

Pose applyTransformation( const Pose& pose, const AlignedStdVector<Transform>& transform ) {
    Pose T( std::min( pose.size(), transform.size() ) );
#pragma omp parallel for
    for ( int i = 0; i < int( T.size() ); ++i ) {
        T[i] = transform[i] * pose[i];
    }
    return T;
}

Pose applyTransformation( const Pose& pose, const Transform& transform ) {
    Pose T( pose.size() );
#pragma omp parallel for
    for ( int i = 0; i < int( T.size() ); ++i ) {
        T[i] = transform * pose[i];
    }
    return T;
}

bool areEqual( const Pose& p0, const Pose& p1 ) {
    CORE_ASSERT( compatible( p0, p1 ), " Poses with different size " );
    const uint n = p0.size();
    for ( uint i = 0; i < n; ++i ) {
        if ( !p0[i].isApprox( p1[i] ) ) { return false; }
    }
    return true;
}

Pose interpolatePoses( const Pose& a, const Pose& b, const Scalar t ) {
    CORE_ASSERT( ( a.size() == b.size() ), "Poses are wrong" );
    CORE_ASSERT( ( ( t >= Scalar( 0. ) ) && ( t <= Scalar( 1. ) ) ), "T is wrong" );

    const uint size = a.size();
    Pose interpolatedPose( size );

#pragma omp parallel for
    for ( int i = 0; i < int( size ); ++i ) {
        interpolatedPose[i] = Math::linearInterpolate( a[i], b[i], t );
    }

    return interpolatedPose;
}

} // namespace Animation
} // namespace Core
} // namespace Ra
