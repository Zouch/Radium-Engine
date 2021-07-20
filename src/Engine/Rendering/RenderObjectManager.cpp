#include <Engine/Rendering/RenderObjectManager.hpp>

#include <Engine/RadiumEngine.hpp>

#include <Engine/Scene/Component.hpp>
#include <Engine/Scene/Entity.hpp>

#include <Engine/Data/Mesh.hpp>
#include <Engine/Rendering/RenderObject.hpp>

#include <Engine/Scene/SignalManager.hpp>

#include <numeric> // for reduce

namespace Ra {
namespace Engine {
namespace Rendering {

RenderObjectManager::RenderObjectManager() = default;

RenderObjectManager::~RenderObjectManager() = default;

bool RenderObjectManager::exists( const Core::Utils::Index& index ) const {
    return ( index.isValid() && m_renderObjects.contains( index ) );
}

Core::Utils::Index RenderObjectManager::addRenderObject( Rendering::RenderObject* renderObject ) {
    // Avoid data race in the std::maps
    std::lock_guard<std::mutex> lock( m_doubleBufferMutex );

    std::shared_ptr<Rendering::RenderObject> newRenderObject( renderObject );
    Core::Utils::Index index = m_renderObjects.insert( newRenderObject );

    newRenderObject->setIndex( index );

    auto type = renderObject->getType();

    m_renderObjectByType[(int)type].insert( index );

    Engine::RadiumEngine::getInstance()->getSignalManager()->fireRenderObjectAdded(
        Scene::ItemEntry(
            renderObject->getComponent()->getEntity(), renderObject->getComponent(), index ) );
    return index;
}

void RenderObjectManager::removeRenderObject( const Core::Utils::Index& index ) {
    CORE_ASSERT( exists( index ), "Trying to access a render object which doesn't exist" );

    // FIXME : Should we check if the render object is in the double buffer map ?
    std::shared_ptr<Rendering::RenderObject> renderObject = m_renderObjects.at( index );

    Engine::RadiumEngine::getInstance()->getSignalManager()->fireRenderObjectRemoved(
        Scene::ItemEntry(
            renderObject->getComponent()->getEntity(), renderObject->getComponent(), index ) );

    // Lock after signal has been fired (as this signal can cause another RO to be deleted)
    std::lock_guard<std::mutex> lock( m_doubleBufferMutex );
    m_renderObjects.remove( index );

    auto type = renderObject->getType();
    m_renderObjectByType[(int)type].erase( index );
    renderObject.reset();
}

size_t RenderObjectManager::getRenderObjectsCount() {
    return m_renderObjects.size();
}

std::shared_ptr<Rendering::RenderObject>
RenderObjectManager::getRenderObject( const Core::Utils::Index& index ) {
    CORE_ASSERT( exists( index ), "Trying to access a render object which doesn't exist" );
    return m_renderObjects.at( index );
}

const Core::Utils::IndexMap<std::shared_ptr<Rendering::RenderObject>>&
RenderObjectManager::getRenderObjects() const {
    return m_renderObjects;
}

void RenderObjectManager::getRenderObjectsByType(
    std::vector<std::shared_ptr<Rendering::RenderObject>>& objectsOut,
    const Rendering::RenderObjectType& type ) const {
    // Take the mutex
    std::lock_guard<std::mutex> lock( m_doubleBufferMutex );

    //// Copy each element in m_renderObjects
    std::transform(
        m_renderObjectByType[(int)type].begin(),
        m_renderObjectByType[(int)type].end(),
        std::back_inserter( objectsOut ),
        [this]( const Core::Utils::Index& i ) { return this->m_renderObjects.at( i ); } );
}

void RenderObjectManager::renderObjectExpired( const Core::Utils::Index& idx ) {
    std::lock_guard<std::mutex> lock( m_doubleBufferMutex );

    auto ro = m_renderObjects.at( idx );
    m_renderObjects.remove( idx );

    auto type = ro->getType();

    m_renderObjectByType[size_t( type )].erase( idx );

    ro->hasExpired();

    ro.reset();
}

size_t RenderObjectManager::getNumFaces() const {
    // todo : use reduce instead of accumulate to improve performances (since C++17)
    size_t result = std::accumulate(
        m_renderObjects.begin(),
        m_renderObjects.end(),
        size_t( 0 ),
        []( size_t a, const std::shared_ptr<Rendering::RenderObject>& ro ) -> size_t {
            if ( ro->isVisible() && ro->getType() == Rendering::RenderObjectType::Geometry ) {
                return a + ro->getMesh()->getNumFaces();
            }
            else {
                return a;
            }
        } );
    return result;
}

size_t RenderObjectManager::getNumVertices() const {
    // todo : use reduce instead of accumulate to improve performances (since C++17)
    size_t result = std::accumulate(
        m_renderObjects.begin(),
        m_renderObjects.end(),
        size_t( 0 ),
        []( size_t a, const std::shared_ptr<Rendering::RenderObject>& ro ) -> size_t {
            if ( ro->isVisible() && ro->getType() == Rendering::RenderObjectType::Geometry ) {
                return a + ro->getMesh()->getNumVertices();
            }
            else {
                return a;
            }
        } );
    return result;
}

} // namespace Rendering
} // namespace Engine
} // namespace Ra
