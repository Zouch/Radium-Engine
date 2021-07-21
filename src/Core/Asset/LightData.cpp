#include <Core/Asset/LightData.hpp>

namespace Ra {
namespace Core {
namespace Asset {

/// CONSTRUCTOR
LightData::LightData( const std::string& name, const LightType& type ) :
    AssetData( name ), m_frame( Eigen::Matrix<Scalar, 4, 4>::Identity() ), m_type( type ) {}

LightData::LightData( const LightData& data ) :
    AssetData( data ), m_frame( data.m_frame ), m_type( data.m_type ), m_color( data.m_color ) {

    switch ( m_type ) {
    case POINT_LIGHT:
        m_pointlight = data.m_pointlight;
        break;
    case SPOT_LIGHT:
        m_spotlight = data.m_spotlight;
        break;
    case DIRECTIONAL_LIGHT:
        m_dirlight = data.m_dirlight;
        break;
    case AREA_LIGHT:
        m_arealight = data.m_arealight;
        break;
    default:
        break;
    }
}

/// DESTRUCTOR
LightData::~LightData() {}

} // namespace Asset
} // namespace Core
} // namespace Ra
