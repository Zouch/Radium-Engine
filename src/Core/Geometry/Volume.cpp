#include <Core/Geometry/Volume.hpp>

#include <Core/Utils/Log.hpp>

namespace Ra {
namespace Core {
namespace Geometry {

AbstractVolume::AbstractVolume( const VolumeStorageType& type ) :
    AbstractGeometry(), m_type( type ) {}

bool AbstractVolume::isParametric() const {
    return ( m_type == PARAMETRIC );
}

bool AbstractVolume::isDiscrete() const {
    return ( m_type == DISCRETE_DENSE ) || ( m_type == DISCRETE_SPARSE );
}

bool AbstractVolume::isDense() const {
    return ( m_type == DISCRETE_DENSE );
}

bool AbstractVolume::isSparse() const {
    return ( m_type == DISCRETE_SPARSE );
}

void AbstractVolume::displayInfo() const {
    using namespace Core::Utils; // log
    std::string type;
    switch ( m_type )
    {
    case UNKNOWN:
        type = "UNKNOWN";
        break;
    case PARAMETRIC:
        type = "PARAMETRIC";
        break;
    case DISCRETE_DENSE:
        type = "DISCRETE (DENSE)";
        break;
    case DISCRETE_SPARSE:
        type = "DISCRETE (SPARSE)";
        break;
    }
    LOG( logINFO ) << "======== MESH INFO ========";
    LOG( logINFO ) << " Type           : " << type;
}

void AbstractDiscreteVolume::clear() {
    setBinSize( Vector3::Zero() );
    setSize( Vector3i::Zero() );
    invalidateAabb();
}

Aabb AbstractDiscreteVolume::computeAabb() const {
    if ( !isAabbValid() )
    { setAabb( Aabb( Vector3::Zero(), m_binSize.cwiseProduct( m_size.cast<Scalar>() ) ) ); }
    return getAabb();
}

VolumeGrid::ValueType VolumeGrid::sample( const IndexType& i ) {
    IndexType idx {
        std::clamp( i.x(), 0, size().x() ),
        std::clamp( i.y(), 0, size().y() ),
        std::clamp( i.z(), 0, size().z() ),
    };
    return m_data[*linearIndex( idx )];
}

void VolumeGrid::computeGradients() {
    m_gradient.resize( m_data.size() );

#pragma omp parallel for
    for ( int k = 0; k < size().z(); ++k )
    {
        for ( int j = 0; j < size().y(); ++j )
        {
            for ( int i = 0; i < size().x(); ++i )
            {
                Vector3 s1;
                Vector3 s2;
                s1( 0 ) = sample( {i - 1, j, k} );
                s2( 0 ) = sample( {i + 1, j, k} );
                s1( 1 ) = sample( {i, j - 1, k} );
                s2( 1 ) = sample( {i, j + 1, k} );
                s1( 2 ) = sample( {i, j, k - 1} );
                s2( 2 ) = sample( {i, j, k + 1} );
                IndexType idx {i, j, k};
                m_gradient[*linearIndex( idx )] = s2 - s1;
            }
        }
    }
}

} // namespace Geometry
} // namespace Core
} // namespace Ra
