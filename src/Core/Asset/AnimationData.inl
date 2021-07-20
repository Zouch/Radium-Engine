#pragma once
#include <Core/Asset/AnimationData.hpp>

#include <Core/Utils/Log.hpp>

namespace Ra {
namespace Core {
namespace Asset {

inline void AnimationData::setName( const std::string& name ) {
    m_name = name;
}

// TIME
inline const AnimationTime& AnimationData::getTime() const {
    return m_time;
}

inline void AnimationData::setTime( const AnimationTime& time ) {
    m_time = time;
}

inline AnimationTime::Time AnimationData::getTimeStep() const {
    return m_dt;
}

inline void AnimationData::setTimeStep( const AnimationTime::Time& delta ) {
    m_dt = delta;
}

// Keyframes
inline uint AnimationData::getFramesSize() const {
    return m_keyFrame.size();
}

inline std::vector<HandleAnimation> AnimationData::getHandleData() const {
    return m_keyFrame;
}

inline void AnimationData::setHandleData( const std::vector<HandleAnimation>& frameList ) {
    const uint size = frameList.size();
    m_keyFrame.resize( size );
#pragma omp parallel for
    for ( int i = 0; i < int( size ); ++i )
    { m_keyFrame[i] = frameList[i]; }
}

inline void AnimationData::displayInfo() const {
    using namespace Core::Utils; // log
    LOG( logDEBUG ) << "======== ANIMATION INFO ========";
    LOG( logDEBUG ) << " Name              : " << m_name;
    LOG( logDEBUG ) << " Start Time        : " << m_time.getStart();
    LOG( logDEBUG ) << " End   Time        : " << m_time.getEnd();
    LOG( logDEBUG ) << " Time Step         : " << m_dt;
    LOG( logDEBUG ) << " Animated Object # : " << m_keyFrame.size();
}

} // namespace Asset
} // namespace Core
} // namespace Ra
