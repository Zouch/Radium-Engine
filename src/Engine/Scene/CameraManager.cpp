#include <Engine/Scene/CameraManager.hpp>

#include <Core/Asset/Camera.hpp>
#include <Core/Asset/FileData.hpp>
#include <Core/Tasks/Task.hpp>
#include <Core/Tasks/TaskQueue.hpp>
#include <Engine/FrameInfo.hpp>
#include <Engine/RadiumEngine.hpp>
#include <Engine/Rendering/RenderObject.hpp>
#include <Engine/Scene/CameraComponent.hpp>
#include <Engine/Scene/ComponentMessenger.hpp>
#include <Engine/Scene/Entity.hpp>
#include <Engine/Scene/SystemDisplay.hpp>

namespace Ra {
namespace Engine {
namespace Scene {

using namespace Core::Utils; // log
using namespace Core::Asset;

Ra::Core::Asset::Camera CameraManager::defaultCamera;

CameraManager::CameraManager() {
    defaultCamera.setFOV( 60.0_ra * Core::Math::toRad );
    defaultCamera.setZNear( 0.1_ra );
    defaultCamera.setZFar( 1000.0_ra );
    m_activeCamera = defaultCamera;
}

void CameraManager::initialize() {

    if ( count() > 0 ) {
        LOG( logDEBUG ) << "CameraManager seems to be already initialized, do nothing.";
        return;
    }
}

void CameraManager::activate( Core::Utils::Index index ) {

    if ( index.isInvalid() || index > count() ) {
        LOG( logDEBUG ) << "Try to activate camera with an invalid/out of bound index. Ignored.";
        return;
    }
    // save current size
    auto width     = m_activeCamera.getWidth();
    auto height    = m_activeCamera.getHeight();
    m_activeCamera = *getCamera( index )->getCamera();
    m_activeCamera.setViewport( width, height );
    m_activeCamera.updateProjMatrix();
}

Ra::Core::Utils::Index CameraManager::getCameraIndex( const CameraComponent* cam ) {
    for ( size_t i = 0; i < m_data->size(); ++i ) {
        if ( cam == ( *m_data )[i] ) return i;
    }
    return {};
}

size_t CameraManager::count() const {
    return m_data->size();
}

void CameraManager::generateTasks( Core::TaskQueue* taskQueue,
                                   const Engine::FrameInfo& /*frameInfo*/ ) {

    class RoUpdater : public Ra::Core::Task
    {
      public:
        void process() override { m_camera->updateTransform(); }
        std::string getName() const override { return "camera updater"; }
        CameraComponent* m_camera;
    };

    // only update visible components.
    for ( size_t i = 0; i < m_data->size(); ++i ) {
        auto comp = ( *m_data )[i];
        auto ro   = comp->getRenderObject();
        if ( ro->isVisible() ) {
            auto updater      = new RoUpdater();
            updater->m_camera = comp;
            taskQueue->registerTask( updater );
        }
    }
}

void CameraManager::handleAssetLoading( Entity* entity, const FileData* filedata ) {
    std::vector<Camera*> cameraData = filedata->getCameraData();
    uint id                         = 0;
    uint cpt                        = 0;
    for ( const auto& data : cameraData ) {
        std::string componentName = "CAMERA_" + entity->getName() + std::to_string( id++ );
        auto comp                 = new CameraComponent( entity, componentName, 100, 100 );
        *( comp->getCamera() )    = *data;
        cpt++;
        registerComponent( entity, comp );
    }
    LOG( logINFO ) << "CameraManager : loaded " << cpt << " Cameras. (now manager has " << count()
                   << " cameras).";
}

void CameraManager::registerComponent( const Entity* entity, Component* component ) {
    System::registerComponent( entity, component );
    m_data->add( reinterpret_cast<CameraComponent*>( component ) );
}

void CameraManager::unregisterComponent( const Entity* entity, Component* component ) {
    System::unregisterComponent( entity, component );
    m_data->remove( reinterpret_cast<CameraComponent*>( component ) );
}

void CameraManager::unregisterAllComponents( const Entity* entity ) {
    for ( const auto& comp : this->m_components ) {
        if ( comp.first == entity ) {
            m_data->remove( reinterpret_cast<CameraComponent*>( comp.second ) );
        }
    }
    System::unregisterAllComponents( entity );
}

} // namespace Scene
} // namespace Engine
} // namespace Ra
