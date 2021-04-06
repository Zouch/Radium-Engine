#include <Core/Asset/AnimationData.hpp>

namespace Ra {
namespace Core {
namespace Asset {

HandleAnimation::HandleAnimation( const std::string& name ) :
    m_name( name ), m_anim( -1, Transform::Identity() ) {}

AnimationData::AnimationData( const std::string& name ) :
    AssetData( name ), m_time(), m_dt( 0.0 ), m_keyFrame() {}

AnimationData::~AnimationData() {}

} // namespace Asset
} // namespace Core
} // namespace Ra
