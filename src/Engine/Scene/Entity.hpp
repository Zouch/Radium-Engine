#pragma once

#include <Engine/RaEngine.hpp>

#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <Core/Types.hpp>
#include <Core/Utils/IndexedObject.hpp>

namespace Ra {
namespace Engine {
namespace Scene {

class Component;
class System;

/// An entity is an scene element. It ties together components with a transform.
class RA_ENGINE_API Entity : public Core::Utils::IndexedObject
{
  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    explicit Entity( const std::string& name = "" );

    // Entities are not copyable.
    Entity( const Entity& entity ) = delete;
    Entity& operator=( const Entity& ) = delete;

    ~Entity() override;

    // Name
    inline const std::string& getName() const;
    inline void rename( const std::string& name );

    // Transform
    inline void setTransform( const Core::Transform& transform );
    inline void setTransform( const Core::Matrix4& transform );
    const Core::Transform& getTransform() const;
    const Core::Matrix4& getTransformAsMatrix() const;

    void swapTransformBuffers();

    // Components
    /// Add a component to the given entity. Component ownership is transfered to the entity.
    void addComponent( Component* component );

    /// Deletes a component with a given name.
    void removeComponent( const std::string& name );

    /// Get component with a given name.
    Component* getComponent( const std::string& name );
    const Component* getComponent( const std::string& name ) const;

    const std::vector<std::unique_ptr<Component>>& getComponents() const;

    /// Get component belonging to a given system.
    // Component* getComponent( const System& system);

    inline uint getNumComponents() const;

    virtual Core::Aabb computeAabb();

    void invalidateAabb();

  private:
    Core::Transform m_transform;
    Core::Transform m_doubleBufferedTransform;
    mutable std::mutex m_transformMutex;

    std::vector<std::unique_ptr<Component>> m_components;

    std::string m_name {};
    bool m_transformChanged { false };

    bool m_isAabbValid { false };
    Core::Aabb m_aabb;

    ///\todo add an index;
};

} // namespace Scene
} // namespace Engine
} // namespace Ra

#include <Engine/Scene/Entity.inl>
