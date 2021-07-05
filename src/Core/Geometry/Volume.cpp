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

} // namespace Geometry
} // namespace Core
} // namespace Ra
