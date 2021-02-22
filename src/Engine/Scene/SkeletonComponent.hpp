#pragma once

#include <Core/Animation/HandleWeight.hpp>
#include <Core/Animation/KeyFramedValue.hpp>
#include <Core/Animation/Skeleton.hpp>
#include <Core/Asset/AnimationData.hpp>
#include <Core/Asset/HandleData.hpp>

#include <Engine/Scene/Component.hpp>

namespace Ra {
namespace Engine {
namespace Data {
class BlinnPhongMaterial;
class Mesh;
} // namespace Data
namespace Rendering {
class RenderObject;
class RenderTechnique;
} // namespace Rendering

namespace Scene {

class SkeletonBoneRenderObject;
class SkeletonBasedAnimationSystem;

/** 
 * The SkeletonComponent is responsible for the management of skeleton-based
 * character animations. It stores the animation Skeleton and the animation
 * data and is responsible for drawing the skeleton.
 *
 * Regarding Component Communication, an AnimationComponent gives access to
 * the following data from the skeleton's name:
 *    - the Skeleton;
 *    - the mapping from the bones RenderObject's index to
 *      the bones indices within the Skeleton;
 *    - the reference Pose;
 *    - the current Animation;
 *    - the current animation time;
 *    - whether the animation time has been reset.
  */
class RA_ENGINE_API SkeletonComponent : public Component
{
  public:
    friend class SkeletonBasedAnimationSystem;

    /// Animations are lists of keyframed transforms (one per bone).
    using Animation = std::vector<Ra::Core::Animation::KeyFramedValue<Ra::Core::Transform>>;

    SkeletonComponent( const std::string& name, Entity* entity );
    ~SkeletonComponent() override;
    SkeletonComponent( const SkeletonComponent& ) = delete;
    SkeletonComponent& operator=( const SkeletonComponent& ) = delete;

    /// \name Component interface
    /// \{

    virtual void initialize() override {}

    virtual bool canEdit( const Ra::Core::Utils::Index& roIdx ) const override;

    virtual Ra::Core::Transform getTransform( const Ra::Core::Utils::Index& roIdx ) const override;

    virtual void setTransform( const Ra::Core::Utils::Index& roIdx,
                               const Ra::Core::Transform& transform ) override;
    /// \}

    /// \name Build from fileData
    /// \{

    /**
     * Create the skeleton from the given data.
     */
    void handleSkeletonLoading( const Ra::Core::Asset::HandleData* data );

    /**
     * Create the animations from the given data.
     */
    void handleAnimationLoading( const std::vector<Ra::Core::Asset::AnimationData*>& data );
    /// \}

    /// \name Skeleton-based animation data
    /// \{

    /**
     * Set the animation skeleton.
     */
    void setSkeleton( const Ra::Core::Animation::Skeleton& skel );

    /**
     * \returns the animation skeleton.
     */
    inline const Ra::Core::Animation::Skeleton* getSkeleton() { return &m_skel; }

    /**
     * Return the number of animations.
     */
    size_t getAnimationCount() const { return m_animations.size(); }

    /**
     * Return the \p i -th animation.
     */
    const Animation& getAnimation( const size_t i ) const { return m_animations[i]; }

    /**
     * Return the \p i -th animation.
     */
    Animation& getAnimation( const size_t i ) { return m_animations[i]; }

    /**
     * Creates a new empty animation from the current pose.
     */
    Animation& addNewAnimation();

    /**
     * Deletes the \p i-th animation.
     */
    void removeAnimation( const size_t i );

    /**
     * Set the animation to play.
     */
    void useAnimation( const size_t i );

    /**
     * Return the index of the animation to play.
     */
    size_t getAnimationId() const;
    /// \}

    /// \name Animation Process
    /// \{

    /**
     * Updates the skeleton pose as the pose corresponding to time \p time.
     */
    void update( Scalar time );

    /**
     * \returns the current time of animation.
     */
    Scalar getAnimationTime() const;

    /**
     * \returns the duration of the current animation.
     */
    Scalar getAnimationDuration() const;

    /**
     * If \p status is TRUE, then use the animation time step if available;
     * otherwise, use the application timestep.
     */
    void toggleAnimationTimeStep( const bool status );

    /**
     * Return true if using the animation time step, false otherwise.
     */
    bool usesAnimationTimeStep() const;

    /**
     * Set animation speed factor.
     */
    void setSpeed( const Scalar value );

    /**
     * Return animation speed factor.
     */
    Scalar getSpeed() const;

    /**
     * Toggle animation auto repeat.
     */
    void autoRepeat( const bool status );

    /**
     * Return true is animation auto repeat is on, false otherwise.
     */
    bool isAutoRepeat() const;

    /**
     * Toggle animation ping-pong.
     */
    void pingPong( const bool status );

    /**
     * Return true is animation ping-pong is on, false otherwise.
     */
    bool isPingPong() const;
    /// \}

    /// \name Skeleton display
    /// \{

    /**
     * Turns xray display on/off for the skeleton bones.
     */
    void setXray( bool on ) const;

    /**
     * Return true if bones are displayed in xray mode, false otherwise.
     */
    bool isXray() const;

    /**
     * Toggle skeleton bones display.
     */
    // FIXME: There is a compatibility issue between xray and display!
    void toggleSkeleton( const bool status );

    /**
     * Return true if the skeleton is displayed.
     */
    bool isShowingSkeleton() const;

    /**
     * Updates the skeleton display.
     */
    void updateDisplay();
    /// \}

    /**
     * Returns the map from RO index to bone index.
     */
    const std::map<Ra::Core::Utils::Index, uint>* getBoneRO2idx() const;

  private:
    /**
     * Internal function to create the bone display objects.
     */
    void setupSkeletonDisplay();

    /**
     * Internal Debug function to display the skeleton hierarchy.
     */
    void printSkeleton( const Ra::Core::Animation::Skeleton& skeleton );

    /// \name Component Communication (CC)
    /// \{

    /**
     * Setup CC.
     */
    void setupIO();

    /**
     * Reference Pose getter for CC.
     */
    const Ra::Core::Animation::RefPose* getRefPoseOutput() const;

    /**
     * Current Animation getter for CC.
     */
    const Animation* getAnimationOutput() const;

    /**
     * Current Animation Time for CC.
     */
    const Scalar* getTimeOutput() const;

    /**
     * Reset status getter for CC.
     */
    const bool* getWasReset() const;
    /// \}

  private:
    /// Entity name for CC.
    std::string m_skelName;

    /// The Animation Skeleton.
    Ra::Core::Animation::Skeleton m_skel;

    /// The Reference Pose in model space.
    Ra::Core::Animation::RefPose m_refPose;

    /// The animations.
    std::vector<Animation> m_animations;

    /// Bones ROs.
    std::vector<Rendering::RenderObject*> m_boneDrawables;

    /// Map from bone RO index to bone idx, for CC.
    std::map<Ra::Core::Utils::Index, uint> m_boneMap;

    /// Current animation ID.
    size_t m_animationID{0};

    /// Wheither to use the animation's timestep (if available) or the app's.
    bool m_animationTimeStep{false};

    /// Time step of each animation.
    std::vector<Scalar> m_dt;

    /// Current animation time (might be different from the app time -- see below).
    Scalar m_animationTime{0_ra};

    /// Animation Play speed.
    Scalar m_speed{1_ra};

    /// Whether animation auto repeat mode in on.
    bool m_autoRepeat{false};

    /// Whether animation ping-pong mode is on.
    bool m_pingPong{false};

    /// Was the animation reset?
    bool m_wasReset{false};

    /// Mesh for bone display.
    static std::shared_ptr<Data::Mesh> s_boneMesh;

    /// Material for bone display.
    static std::shared_ptr<Data::BlinnPhongMaterial> s_boneMaterial;

    /// RenderTechnique for bone display.
    static std::shared_ptr<Rendering::RenderTechnique> s_boneRenderTechnique;
};

} // namespace Scene
} // namespace Engine
} // namespace Ra

