#pragma once

#include <Core/RaCore.hpp>
#include <Eigen/Core>
#include <Eigen/Geometry>

namespace Ra {
namespace Core {
namespace Geometry {
/// An oriented bounding box.
class Obb
{
  public:
    using Transform = Eigen::Transform<Scalar, 3, Eigen::Affine>;
    using Aabb      = Eigen::AlignedBox<Scalar, 3>;

    /// Constructors and destructor.

    /// Initializes an empty bounding box.
    inline Obb() : m_aabb(), m_transform( Transform::Identity() ) {}

    /// Initialize an OBB from an AABB and a transform.
    inline Obb( const Aabb& aabb, const Transform& tr ) : m_aabb( aabb ), m_transform( tr ) {}

    /// Default copy constructor and assignment operator.
    Obb( const Obb& other ) = default;
    Obb& operator=( const Obb& other ) = default;

    virtual inline ~Obb() {}

    /// Return the AABB enclosing this
    inline Aabb toAabb() const {
        if ( m_aabb.isEmpty() ) { return m_aabb; }
        Aabb tmp;
        for ( int i = 0; i < 8; ++i ) {
            tmp.extend( worldCorner( i ) );
        }
        return tmp;
    }

    /// Extends the OBB with an new point.
    inline void addPoint( const Eigen::Matrix<Scalar, 3, 1>& p ) { m_aabb.extend( p ); }

    /// Returns the position of the i^th corner of AABB (model space)
    inline Eigen::Matrix<Scalar, 3, 1> corner( int i ) const {
        return m_aabb.corner( static_cast<Aabb::CornerType>( i ) );
    }

    /// Returns the position of the ith corner of the OBB ( world space )
    inline Eigen::Matrix<Scalar, 3, 1> worldCorner( int i ) const {
        return m_transform * m_aabb.corner( static_cast<Aabb::CornerType>( i ) );
    }

    /// Non-const access to the obb transformation
    Transform& transform() { return m_transform; }

    /// Const access to the obb transformation
    const Transform& transform() const { return m_transform; }

  private:
    /// The untransformed AABB
    Aabb m_aabb;
    /// Orientation of the box.
    Transform m_transform;
};
} // namespace Geometry
} // namespace Core
} // namespace Ra
