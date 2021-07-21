#include <Engine/Scene/Component.hpp>

#include <Core/Math/LinearAlgebra.hpp>
#include <Core/Utils/Log.hpp>
#include <Engine/Data/Mesh.hpp>
#include <Engine/RadiumEngine.hpp>
#include <Engine/Rendering/RenderObject.hpp>
#include <Engine/Rendering/RenderObjectManager.hpp>
#include <Engine/Scene/Entity.hpp>
#include <Engine/Scene/SignalManager.hpp>
#include <Engine/Scene/System.hpp>

namespace Ra {
namespace Engine {
namespace Scene {

using namespace Core::Utils; // log

Component::Component( const std::string& name, Entity* entity ) :
    m_name { name }, m_entity { entity } {
    m_entity->addComponent( this );
}

Component::~Component() {
    for ( const auto& ro : m_renderObjects ) {
        getRoMgr()->removeRenderObject( ro );
    }
    if ( m_system ) { m_system->unregisterComponent( getEntity(), this ); }
    RadiumEngine::getInstance()->getSignalManager()->fireComponentRemoved(
        ItemEntry( getEntity(), this ) );
}

Rendering::RenderObjectManager* Component::getRoMgr() {
    return RadiumEngine::getInstance()->getRenderObjectManager();
}

Core::Utils::Index Component::addRenderObject( Rendering::RenderObject* renderObject ) {
    m_renderObjects.push_back( getRoMgr()->addRenderObject( renderObject ) );
    invalidateAabb();
    return m_renderObjects.back();
}

void Component::removeRenderObject( const Core::Utils::Index& roIdx ) {
    auto found = std::find( m_renderObjects.cbegin(), m_renderObjects.cend(), roIdx );
    CORE_WARN_IF( found == m_renderObjects.cend(), " Render object not found in component" );
    if ( ( found != m_renderObjects.cend() ) && getRoMgr() ) {
        getRoMgr()->removeRenderObject( *found );
        m_renderObjects.erase( found );
    }
    invalidateAabb();
}

void Component::notifyRenderObjectExpired( const Core::Utils::Index& idx ) {
    auto found = std::find( m_renderObjects.cbegin(), m_renderObjects.cend(), idx );
    CORE_WARN_IF( found == m_renderObjects.cend(), " Render object not found in component" );
    if ( found != m_renderObjects.cend() ) { m_renderObjects.erase( found ); }
}

Core::Aabb Component::computeAabb() {
    if ( !m_isAabbValid ) {
        // component haven't any transformation, so ask render objects own aabb.
        Core::Aabb aabb;
        for ( const auto& roIndex : m_renderObjects ) {
            auto ro =
                RadiumEngine::getInstance()->getRenderObjectManager()->getRenderObject( roIndex );
            if ( ro->isVisible() ) { aabb.extend( ro->computeAabb() ); }
        }

        m_aabb        = aabb;
        m_isAabbValid = true;
    }

    return m_aabb;
}

void Component::invalidateAabb() {
    m_isAabbValid = false;
    m_entity->invalidateAabb();
}

} // namespace Scene
} // namespace Engine
} // namespace Ra
