#include <Engine/Scene/System.hpp>

#include <Engine/Scene/Component.hpp>
#include <Engine/Scene/Entity.hpp>

namespace Ra {
namespace Engine {
namespace Scene {

void System::registerComponent( const Entity* ent, Component* component ) {
    // Perform checks on debug
#if defined( DEBUG )
    CORE_ASSERT( component->getEntity() == ent, "Component does not belong to entity" );
    for ( const auto& pair : m_components )
    {
        CORE_ASSERT( pair.second != component, "Component already registered" );
        if ( pair.first == ent )
        {
            CORE_ASSERT( pair.second->getName() != component->getName(),
                         "A component with the same name is already associated with this entity" );
        }
    }
#endif // DEBUG
    m_components.emplace_back( ent, component );
    component->setSystem( this );
}

void System::unregisterComponent( const Entity* ent, Component* component ) {
    CORE_ASSERT( component->getEntity() == ent, "Component does not belong to entity" );
    CORE_UNUSED( ent );
    const auto& pos =
        std::find_if( m_components.begin(), m_components.end(), [component]( const auto& pair ) {
            return pair.second == component;
        } );

    CORE_ASSERT( pos != m_components.end(), "Component is not registered." );
    CORE_ASSERT( pos->first == ent, "Component belongs to a different entity" );
    component->setSystem( nullptr );
    m_components.erase( pos );
}

void System::unregisterAllComponents( const Entity* entity ) {
    std::vector<std::pair<const Entity*, Component*>>::iterator pos;
    while ( ( pos = std::find_if(
                  m_components.begin(), m_components.end(), [entity]( const auto& pair ) {
                      return pair.first == entity;
                  } ) ) != m_components.end() )
    { m_components.erase( pos ); }
}

std::vector<Component*> System::getEntityComponents( const Entity* entity ) {
    std::vector<Component*> comps;
    for ( const auto& ec : m_components )
    {
        if ( ec.first == entity ) { comps.push_back( ec.second ); }
    }
    return comps;
}

} // namespace Scene
} // namespace Engine
} // namespace Ra
