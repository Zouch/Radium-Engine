#pragma once

#include <string>
#include <vector>

#include <Core/Animation/KeyFramedValue.hpp>
#include <Core/Asset/AnimationTime.hpp>
#include <Core/Asset/AssetData.hpp>
#include <Core/RaCore.hpp>

namespace Ra {
namespace Core {
namespace Asset {

/**
 * A HandleAnimation stores data for an animation Handle.
 */
struct RA_CORE_API HandleAnimation {
    explicit HandleAnimation( const std::string& name = "" );

    /// The Handle's name.
    std::string m_name;

    /// The list of KeyFramed transforms applied to the Handle.
    Core::Animation::KeyFramedValue<Transform> m_anim;

    /// The AnimationTime for the Handle.
    AnimationTime m_animationTime;
};

/**
 * The AnimationData class stores all the HandleAnimation related to an
 * animation of an object, one per animation Handle.
 * \note Objects can have several AnimationData, one for each animation.
 */
class RA_CORE_API AnimationData : public AssetData
{
  public:
    explicit AnimationData( const std::string& name = "" );

    ~AnimationData();

    /**
     * Sets the name of the animation.
     */
    inline void setName( const std::string& name );

    /// \name Time
    /// \{

    /**
     * \returns the AnimationTime of the animation.
     */
    inline const AnimationTime& getTime() const;

    /**
     * Sets the AnimationTime of the animation.
     */
    inline void setTime( const AnimationTime& time );

    /**
     * \returns the animation timestep.
     */
    inline AnimationTime::Time getTimeStep() const;

    /**
     * Sets the animation timestep.
    ￼ */
    inline void setTimeStep( const AnimationTime::Time& delta );
    /// \}

    /// \name Keyframes
    /// \{

    /**
     * \returns the number of HandleAnimations.
     */
    inline uint getFramesSize() const;

    /**
     * \returns the list of HandleAnimations, i.e. the whole animation frames.
     */
    inline std::vector<HandleAnimation> getHandleData() const;

    /**
     * Sets the animation frames.
     */
    inline void setHandleData( const std::vector<HandleAnimation>& frameList );
    /// \}

    /**
     * Print stat info to the Debug output.
     */
    inline void displayInfo() const;

  protected:
    /// The AnimationTime for the object.
    AnimationTime m_time;

    /// The animation timestep.
    AnimationTime::Time m_dt { 0 };

    /// The animation frames.
    std::vector<HandleAnimation> m_keyFrame;
};

} // namespace Asset
} // namespace Core
} // namespace Ra

#include <Core/Asset/AnimationData.inl>
