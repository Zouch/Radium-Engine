#include <Engine/Scene/EntityManager.hpp>

#include <Core/Utils/Log.hpp>

#include <Engine/RadiumEngine.hpp>
#include <Engine/Scene/SignalManager.hpp>
#include <Engine/Scene/SystemDisplay.hpp>

namespace Ra {
namespace Engine {
namespace Scene {

using namespace Core::Utils; // log

EntityManager::EntityManager() {
    auto idx  = m_entities.emplace( SystemEntity::createInstance() );
    auto& ent = m_entities[idx];
    ent->setIndex( idx );
    CORE_ASSERT( ent.get() == SystemEntity::getInstance(), "Invalid singleton instanciation" );
    m_entitiesName.insert( { ent->getName(), ent->getIndex() } );
    RadiumEngine::getInstance()->getSignalManager()->fireEntityCreated(
        ItemEntry( SystemEntity::getInstance() ) );
}

EntityManager::~EntityManager() {
    auto& ent = m_entities[0];
    ent.release();
    SystemEntity::destroyInstance();
}

Entity* EntityManager::createEntity( const std::string& name ) {
    auto idx  = m_entities.emplace( new Entity( name ) );
    auto& ent = m_entities[idx];
    ent->setIndex( idx );

    std::string entityName = name;
    if ( name.empty() )
    {
        entityName = "Entity_" + std::to_string( idx.getValue() );
        ent->rename( entityName );
    }
    else
    {
        int i           = 1;
        bool mustRename = false;
        while ( entityExists( entityName ) )
        {
            LOG( logWARNING ) << "Entity `" << entityName << "` already exists";
            entityName = name + "_" + std::to_string( i++ );
            mustRename = true;
        }
        if ( mustRename ) { ent->rename( entityName ); }
    }

    m_entitiesName.insert( { ent->getName(), idx } );
    RadiumEngine::getInstance()->getSignalManager()->fireEntityCreated( ItemEntry( ent.get() ) );
    return ent.get();
}

bool EntityManager::entityExists( const std::string& name ) const {
    return m_entitiesName.find( name ) != m_entitiesName.end();
}

void EntityManager::removeEntity( Core::Utils::Index idx ) {
    CORE_ASSERT( idx.isValid() && m_entities.contains( idx ),
                 "Trying to remove an entity that has not been added to the manager." );

    auto& ent        = m_entities[idx];
    std::string name = ent->getName();
    m_entities.remove( idx );
    m_entitiesName.erase( name );
}

void EntityManager::removeEntity( Entity* entity ) {
    removeEntity( entity->getIndex() );
}

Entity* EntityManager::getEntity( Core::Utils::Index idx ) const {
    CORE_ASSERT( idx.isValid(), "Trying to access an invalid component." );

    Entity* ent = nullptr;

    if ( m_entities.contains( idx ) ) { ent = m_entities[idx].get(); }

    return ent;
}

std::vector<Entity*> EntityManager::getEntities() const {
    std::vector<Entity*> entities;
    entities.resize( m_entities.size() );
    std::transform( m_entities.begin(), m_entities.end(), entities.begin(), []( const auto& e ) {
        return e.get();
    } );

    return entities;
}

Entity* EntityManager::getEntity( const std::string& name ) const {
    auto idx = m_entitiesName.find( name );
    if ( idx == m_entitiesName.end() )
    {
        LOG( logDEBUG ) << "Trying to access an invalid entity (named: " + name + ")";
        return nullptr;
    }
    return m_entities.at( idx->second ).get();
}

void EntityManager::swapBuffers() {
    for ( auto& e : m_entities )
    { e->swapTransformBuffers(); }
}

void EntityManager::deleteEntities() {
    std::vector<Core::Utils::Index> indices;
    indices.reserve( m_entities.size() - 1 );
    for ( size_t i = 1; i < m_entities.size(); ++i )
    { indices.push_back( m_entities.index( i ) ); }
    for ( const auto& idx : indices )
    { removeEntity( idx ); }
}
} // namespace Scene
} // namespace Engine
} // namespace Ra
