#include <Engine/Scene/DefaultLightManager.hpp>

#include <Engine/RadiumEngine.hpp>

namespace Ra {
namespace Engine {
namespace Scene {

DefaultLightManager::DefaultLightManager() {
    m_data = std::make_unique<DefaultLightStorage>();
}

const Scene::Light* DefaultLightManager::getLight( size_t li ) const {
    return ( *m_data )[li];
}

void DefaultLightManager::addLight( const Scene::Light* li ) {
    m_data->add( li );
}

DefaultLightStorage::DefaultLightStorage() = default;

void DefaultLightStorage::upload() const {}

void DefaultLightStorage::add( const Scene::Light* li ) {
    m_lights.emplace( li->getType(), li );
}

void DefaultLightStorage::remove( const Scene::Light* li ) {
    auto range = m_lights.equal_range( li->getType() );
    for ( auto i = range.first; i != range.second; ++i ) {
        if ( i->second == li ) {
            m_lights.erase( i );
            break;
        }
    }
}

size_t DefaultLightStorage::size() const {
    return m_lights.size();
}

void DefaultLightStorage::clear() {
    m_lights.clear();
}

const Scene::Light* DefaultLightStorage::operator[]( unsigned int n ) {
    auto iterator = m_lights.begin();
    std::advance( iterator, n );
    return iterator->second;
}
} // namespace Scene
} // namespace Engine
} // namespace Ra
