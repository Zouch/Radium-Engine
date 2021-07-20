#pragma once

#include <Core/Utils/Index.hpp>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Engine/RaEngine.hpp>

#include <vector>

#include <Core/Types.hpp>

namespace Ra {

namespace Engine {

namespace Rendering {
class RenderObject;
class RenderObjectManager;
} // namespace Rendering

namespace Scene {
class System;
class Entity;

/**
 * @brief A component is an element that can be updated by a system.
 * It is also linked to some other components in an entity.
 * Each component share a transform through their entity.
 */
class RA_ENGINE_API Component
{
  public:
    /// CONSTRUCTOR
    Component( const std::string& name, Entity* entity );

    /// DESTRUCTOR
    virtual ~Component();

    /**
     * @brief Pure virtual method to be overridden by any component.
     * When this method is called you are guaranteed that all other startup systems
     * have been loaded.
     */
    virtual void initialize() = 0;
    /**
     * @brief Set entity the component is part of.
     * This method is called by the entity.
     * @param entity The entity the component is part of.
     */
    virtual void setEntity( Entity* entity );

    /// Return the entity the component belongs to
    virtual Entity* getEntity() const;

    /// Return the component's name
    virtual const std::string& getName() const;

    /// Set the system to which the  component belongs.
    virtual void setSystem( System* system );

    /// Returns the system to which the component belongs.
    virtual System* getSystem() const;

    /// Add a new render object to the component. This adds the RO to the manager for drawing.
    Core::Utils::Index addRenderObject( Rendering::RenderObject* renderObject );

    /// Remove the render object from the component.
    void removeRenderObject( const Core::Utils::Index& roIdx );

    // Editable transform interface.
    // This allow to edit the data in the component with a render object
    // as a key. An invalid RO index can be passed, meaning no specific RO is
    // queried.

    /// Returns true if a transform can be edited with the render object index given as a key.
    virtual bool canEdit( const Core::Utils::Index& /*roIdx*/ ) const { return false; }

    /// Get the transform associated with the given RO index key.
    virtual Core::Transform getTransform( const Core::Utils::Index& /*roIdx*/ ) const {
        return Core::Transform::Identity();
    }

    /// Set the new transform associated with the RO index key.
    virtual void setTransform( const Core::Utils::Index& /*roIdx*/,
                               const Core::Transform& /*transform*/ ) {}

    void notifyRenderObjectExpired( const Core::Utils::Index& idx );

    // return the aabb of the union of visible RenderObjects aabb
    virtual Core::Aabb computeAabb();

    void invalidateAabb();

    const std::vector<Core::Utils::Index>& getRenderObjects() { return m_renderObjects; }

  protected:
    /// Shortcut to access the render object manager.
    static Rendering::RenderObjectManager* getRoMgr();

  public:
    std::vector<Core::Utils::Index> m_renderObjects;

  protected:
    std::string m_name {};
    Entity* m_entity { nullptr };
    System* m_system { nullptr };

  private:
    bool m_isAabbValid { false };
    Core::Aabb m_aabb;
};

} // namespace Scene
} // namespace Engine
} // namespace Ra

#include <Engine/Scene/Component.inl>
