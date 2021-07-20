#include <Core/Geometry/VertexDistance.hpp>

namespace Ra {
namespace Core {
namespace Geometry {

void vertexDistance( const VectorArray<Vector3>& v0,
                     const VectorArray<Vector3>& v1,
                     std::vector<Scalar>& sqrDist,
                     Scalar& sqrMin,
                     Scalar& sqrMax,
                     Scalar& sqrAvg ) {
    const uint n = v0.size();
    sqrMin       = std::numeric_limits<Scalar>::max();
    sqrMax       = 0.0;
    sqrAvg       = 0.0;
    sqrDist.resize( n );
    for ( uint i = 0; i < n; ++i )
    {
        sqrDist[i] = ( v0[i] - v1[i] ).squaredNorm();
        sqrMax     = ( sqrDist[i] > sqrMax ) ? sqrDist[i] : sqrMax;
        sqrMin     = ( sqrDist[i] < sqrMin ) ? sqrDist[i] : sqrMin;
    }
    sqrAvg = ( sqrMax + sqrMin ) * 0.5;
}

void vertexDistance( const VectorArray<Vector3>& v0,
                     const VectorArray<Vector3>& v1,
                     Scalar& sqrMin,
                     Scalar& sqrMax,
                     Scalar& sqrAvg ) {
    const uint n = v0.size();
    sqrMax       = 0.0;
    sqrMin       = std::numeric_limits<Scalar>::max();
    for ( uint i = 0; i < n; ++i )
    {
        Scalar sqrDist = ( v0[i] - v1[i] ).squaredNorm();
        sqrMax         = ( sqrDist > sqrMax ) ? sqrDist : sqrMax;
        sqrMin         = ( sqrDist < sqrMin ) ? sqrDist : sqrMin;
    }
    sqrAvg = ( sqrMax + sqrMin ) * 0.5;
}

Scalar vertexDistance( const VectorArray<Vector3>& v0, const VectorArray<Vector3>& v1 ) {
    const uint n  = v0.size();
    Scalar sqrAvg = 0.0;
    for ( uint i = 0; i < n; ++i )
    { sqrAvg += ( v0[i] - v1[i] ).squaredNorm(); }
    return ( sqrAvg / (Scalar)n );
}

} // namespace Geometry
} // namespace Core
} // namespace Ra
