#include <Core/Asset/FileData.hpp>
#include <Core/Tasks/Task.hpp>
#include <Core/Tasks/TaskQueue.hpp>
#include <Engine/FrameInfo.hpp>
#include <Engine/RadiumEngine.hpp>
#include <Engine/Scene/ComponentMessenger.hpp>
#include <Engine/Scene/DirLight.hpp>
#include <Engine/Scene/Entity.hpp>
#include <Engine/Scene/LightManager.hpp>
#include <Engine/Scene/PointLight.hpp>
#include <Engine/Scene/SpotLight.hpp>
#include <Engine/Scene/SystemDisplay.hpp>

namespace Ra {

namespace Engine {
namespace Scene {

using namespace Core::Utils; // log
using namespace Core::Asset;

LightManager::~LightManager() = default;

//
// Renderer pre/post calls
//

size_t LightManager::count() const {
    return m_data->size();
}

//
// System
//

void LightManager::generateTasks( Core::TaskQueue* /*taskQueue*/,
                                  const Engine::FrameInfo& /*frameInfo*/ ) {
    // do nothing as this system only manage light related asset loading
}

void LightManager::handleAssetLoading( Entity* entity, const FileData* filedata ) {
    std::vector<LightData*> lightData = filedata->getLightData();
    uint id                           = 0;

    // If thereis some lights already in the manager, just remove from the manager the lights that
    // belong to the system entity (e.g. the headlight) from the list of managed lights. Beware to
    // not destroy the headlight component, that do not belong to this system, so that it could be
    // added again
    for ( size_t i = 0; i < m_data->size(); ) {
        auto l = ( *m_data )[i];
        if ( l->getEntity() == SystemEntity::getInstance() ) { m_data->remove( l ); }
        else {
            ++i;
        }
    }

    for ( const auto& data : lightData ) {
        std::string componentName =
            "LIGHT_" + data->getName() + " (" + std::to_string( id++ ) + ")";
        Scene::Light* comp = nullptr;

        switch ( data->getType() ) {
        case LightData::DIRECTIONAL_LIGHT: {

            auto thelight = new DirectionalLight( entity, componentName );
            thelight->setColor( data->m_color );
            thelight->setDirection( data->m_dirlight.direction );
            comp = thelight;
            break;
        }
        case LightData::POINT_LIGHT: {
            auto thelight = new PointLight( entity, componentName );
            thelight->setColor( data->m_color );
            thelight->setPosition( data->m_pointlight.position );
            thelight->setAttenuation( data->m_pointlight.attenuation.constant,
                                      data->m_pointlight.attenuation.linear,
                                      data->m_pointlight.attenuation.quadratic );
            comp = thelight;
            break;
        }
        case LightData::SPOT_LIGHT: {
            auto thelight = new SpotLight( entity, componentName );
            thelight->setColor( data->m_color );
            thelight->setPosition( data->m_spotlight.position );
            thelight->setDirection( data->m_spotlight.direction );
            thelight->setAttenuation( data->m_spotlight.attenuation.constant,
                                      data->m_spotlight.attenuation.linear,
                                      data->m_spotlight.attenuation.quadratic );
            thelight->setInnerAngleInRadians( data->m_spotlight.innerAngle );
            thelight->setOuterAngleInRadians( data->m_spotlight.outerAngle );
            comp = thelight;
            break;
        }
        case LightData::AREA_LIGHT: {
            // Radium-V2 : manage real area light. For the moment, transform them in point light
            // using given position
            auto thelight = new PointLight( entity, componentName );
            thelight->setColor( data->m_color );
            thelight->setPosition( data->m_arealight.position );
            thelight->setAttenuation( data->m_arealight.attenuation.constant,
                                      data->m_arealight.attenuation.linear,
                                      data->m_arealight.attenuation.quadratic );
            comp = thelight;
            break;
        }
        default:
            comp = nullptr;
        }

        // comp should be allocated in LightStorage (well, not sure ...)
        if ( !comp ) continue;

        registerComponent( entity, comp );
    }
    LOG( logINFO ) << "LightManager : loaded " << count() << " lights.";
}

void LightManager::registerComponent( const Entity* entity, Component* component ) {
    System::registerComponent( entity, component );
    m_data->add( reinterpret_cast<Scene::Light*>( component ) );
}

void LightManager::unregisterComponent( const Entity* entity, Component* component ) {
    System::unregisterComponent( entity, component );
    m_data->remove( reinterpret_cast<Scene::Light*>( component ) );
}

void LightManager::unregisterAllComponents( const Entity* entity ) {
    for ( const auto& comp : this->m_components ) {
        if ( comp.first == entity ) {
            m_data->remove( reinterpret_cast<Scene::Light*>( comp.second ) );
        }
    }
    System::unregisterAllComponents( entity );
}

} // namespace Scene
} // namespace Engine
} // namespace Ra
