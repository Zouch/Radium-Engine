#pragma once

#include <Engine/Scene/ItemEntry.hpp>
#include <Engine/Scene/System.hpp>

namespace Ra {
namespace Engine {
namespace Scene {

/**
 * The SkeletonBasedAnimationSystem manages both SkeletonComponents and SkinningComponents.
 * It is also responsible for transmitting calls to/from animation-related processes.
 */
class RA_ENGINE_API SkeletonBasedAnimationSystem : public System
{
  public:
    /// Create a new animation system
    SkeletonBasedAnimationSystem();

    SkeletonBasedAnimationSystem( const SkeletonBasedAnimationSystem& ) = delete;
    SkeletonBasedAnimationSystem& operator=( const SkeletonBasedAnimationSystem& ) = delete;

    /// \name System Interface
    /// \{

    /**
     * Creates a task for each AnimationComponent to update skeleton display.
     */
    void generateTasks( Core::TaskQueue* taskQueue, const FrameInfo& frameInfo ) override;

    /**
     * Loads Skeletons and Animations from a file data into the givn Entity.
     */
    void handleAssetLoading( Entity* entity, const Core::Asset::FileData* fileData ) override;
    /// \}

    /// \name Skeleton display
    /// \{

    /**
     * Sets bone display xray mode to \p on for all AnimationComponents.
     */
    void setXray( bool on );

    /**
     * \returns true if bone display xray mode on, false otherwise.
     */
    bool isXrayOn();

    /**
     * Toggles skeleton display for all AnimationComponents.
     */
    void toggleSkeleton( const bool status );
    /// \}

    /// Enforce Skeleton update at the next frame.
    void enforceUpdate() { m_time = -1; }

  private:
    /// True if we want to show xray-bones.
    bool m_xrayOn {false};

    /// The current animation time.
    Scalar m_time {0};
};

} // namespace Scene
} // namespace Engine
} // namespace Ra
