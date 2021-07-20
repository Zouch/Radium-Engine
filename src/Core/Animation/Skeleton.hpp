#pragma once

#include <Core/Animation/HandleArray.hpp>
#include <Core/Containers/AdjacencyList.hpp>

namespace Ra {
namespace Core {
namespace Animation {

/**
 * The Skeleton class.
 *
 * A skeleton is a set of joint transforms with an associated joint hierarchy,
 * represented by a graph ( adjacency list ).
 *
 * Skeleton bones represent a couple of joints: the proximal joint and the distal joint,
 * the former being the parent of the latter in the hierarchy.
 * For animation purposes, a bone transform refers to the proximal joint's tranform.
 *
 * During the edition of the transformation of a skeleton bone, the transformations
 * of all the bones are updated accroding to the Manipulation scheme
 * (cf Ra::Core:Animation::Skeleton::Manipulation).
 */
class RA_CORE_API Skeleton : public HandleArray
{
  public:
    /// Edition scheme for the manipulation of the skeleton.
    /// \todo also implement Inverse Kynematics.
    enum Manipulation {
        FORWARD = 0, ///< Standard edition scheme: rotation and / or translation of one bone.
        PSEUDO_IK    ///< Advanced edition scheme: translation of a bone means parent's rotation.
    };

    Skeleton();
    explicit Skeleton( const uint n );
    Skeleton( const Skeleton& skeleton ) = default;

    ~Skeleton() override = default;

    inline uint size() const override { return m_graph.size(); }
    void clear() override;
    const Pose& getPose( const SpaceType MODE ) const override;
    void setPose( const Pose& pose, const SpaceType MODE ) override;
    const Transform& getTransform( const uint i, const SpaceType MODE ) const override;

    /**
     * Set the i-th transform to \p T, which is given in the MODE space, w.r.t.
     * the current manipulation scheme and updates the whole Skeleton Pose.
     */
    void setTransform( const uint i, const Transform& T, const SpaceType MODE ) override;

    /**
     * Add a new root transform to the skeleton.
     * @param T      the joint transform associated to the new bone
     * @param label  the name for the new joint
     * @return       the index of the new joint
     */
    uint addRoot( const Transform& T = Transform::Identity(), const Label label = "" );

    /**
     * Add a new joint transform to the skeleton.
     * @param parent the index of the joint's parent in the hierarchy
     * @param T      the joint transform associated to the new bone
     * @param MODE   SpaceType of T (either SpaceType::LOCAL or SpaceType::MODEL)
     * @param label  the name for the new joint
     * @return       the index of the new joint
     */
    uint addBone( const uint parent,
                  const Transform& T   = Transform::Identity(),
                  const SpaceType MODE = SpaceType::LOCAL,
                  const Label label    = "" );

    /**
     * Get the i-th bone endpoints in Model space.
     * @param i             the bone index
     * @param[out] startOut the bone's start point
     * @param[out]  endOut   the bone's end point
     *
     * \note If the i-th bone is a leaf, then \p startOut == \p endOut.
     */
    void getBonePoints( uint i, Vector3& startOut, Vector3& endOut ) const;

    /// Projects point \p pos, given in Model Space, onto the bone with index \p boneIdx.
    Vector3 projectOnBone( uint boneIdx, const Vector3& pos ) const;

    /// Stream insertion operator.
    friend std::ostream& operator<<( std::ostream& os, const Skeleton& skeleton );

  protected:
    /**
     * Sets the \p i-th transform to T, given in Local space.
     * \note Updates the Model-space transform of the descendants.
     * \note This method keeps the Local-space transforms of the descendants untouched.
     */
    void setLocalTransform( uint i, const Transform& T );

    /**
     * Sets the \p i-th transform to T, given in Model space.
     * \note Updates the Local-space transform of the descendants.
     * \note This method keeps the Model-space transforms of the descendants untouched.
     */
    void setModelTransform( uint i, const Transform& T );

  public:
    /// The Joint hierarchy.
    AdjacencyList m_graph;

    /// The manipulation scheme.
    Manipulation m_manipulation { FORWARD };

  protected:
    /// Skeleton pose in MODEL space.
    ModelPose m_modelSpace;
};

} // namespace Animation
} // Namespace Core
} // Namespace Ra
