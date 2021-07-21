#pragma once

#include <Core/RaCore.hpp>
#include <Core/Types.hpp>
#include <Core/Utils/Observable.hpp>

namespace Ra {
namespace Core {
namespace Geometry {

/// Abstract class for Geometry objects in Radium Core
///
/// \warning: Part of the current proposal for a modular implementation of
/// displayable objects.
///
/// \note We use a struct because all members are public anyway
struct RA_CORE_API AbstractGeometry {
    /*
     * Note: Explicitly defaulted virtual destructor, copy/move constructors,
     * copy/move assignment operators
     * When a base class is intended for polymorphic use, its destructor may have
     * to be declared public and virtual. This blocks implicit moves
     * (and deprecates implicit copies), and so the special member functions
     */
    virtual ~AbstractGeometry() = default;
    AbstractGeometry()          = default;
    AbstractGeometry( const AbstractGeometry& other ) {
        m_isAabbValid = other.m_isAabbValid;
        m_aabb        = other.m_aabb;
        other.m_aabbObservable.copyObserversTo( m_aabbObservable );
    }
    AbstractGeometry& operator=( const AbstractGeometry& other ) {
        m_isAabbValid = other.m_isAabbValid;
        m_aabb        = other.m_aabb;
        other.m_aabbObservable.copyObserversTo( m_aabbObservable );
        return *this;
    }
    /// Erases all data, making the geometry empty.
    virtual void clear() = 0;

    /// Compute bounding box
    virtual Aabb computeAabb() const = 0;

    Ra::Core::Utils::ObservableVoid& getAabbObservable() { return m_aabbObservable; }

  protected:
    bool isAabbValid() const { return m_isAabbValid; }
    Core::Aabb getAabb() const { return m_aabb; }

    void invalidateAabb() const {
        m_isAabbValid = false;
        m_aabbObservable.notify();
    }
    // set a new (valid) aabb
    void setAabb( const Core::Aabb& aabb ) const {
        m_aabb        = aabb;
        m_isAabbValid = true;
    }

  private:
    mutable bool m_isAabbValid { false };
    mutable Core::Aabb m_aabb;
    Ra::Core::Utils::ObservableVoid m_aabbObservable;
};

} // namespace Geometry
} // namespace Core
} // namespace Ra
