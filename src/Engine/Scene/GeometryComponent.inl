#pragma once

#include <Engine/Scene/GeometryComponent.hpp>

#include <Core/Containers/MakeShared.hpp>

#include <Engine/Data/BlinnPhongMaterial.hpp>
#include <Engine/Data/MaterialConverters.hpp>
#include <Engine/Data/RenderParameters.hpp>
#include <Engine/Rendering/RenderObject.hpp>
#include <Engine/Scene/ComponentMessenger.hpp>
#include <Engine/Scene/Entity.hpp>

namespace Ra {
namespace Engine {
namespace Scene {

template <typename CoreMeshType>
SurfaceMeshComponent<CoreMeshType>::SurfaceMeshComponent(
    const std::string& name,
    Entity* entity,
    const Ra::Core::Asset::GeometryData* data ) :
    GeometryComponent( name, entity ), m_displayMesh( nullptr ) {
    generateMesh( data );
}

template <typename CoreMeshType>
SurfaceMeshComponent<CoreMeshType>::SurfaceMeshComponent( const std::string& name,
                                                          Entity* entity,
                                                          CoreMeshType&& mesh,
                                                          Core::Asset::MaterialData* mat ) :
    GeometryComponent( name, entity ), m_displayMesh( new Data::Mesh( name, std::move( mesh ) ) ) {
    setContentName( name );
    finalizeROFromGeometry( mat, Core::Transform::Identity() );
}

template <typename CoreMeshType>
void SurfaceMeshComponent<CoreMeshType>::generateMesh( const Ra::Core::Asset::GeometryData* data ) {
    m_contentName     = data->getName();
    m_displayMesh     = Ra::Core::make_shared<RenderMeshType>( m_contentName );
    CoreMeshType mesh = Data::createCoreMeshFromGeometryData<CoreMeshType>( data );

    m_displayMesh->loadGeometry( std::move( mesh ) );

    finalizeROFromGeometry( data->hasMaterial() ? &( data->getMaterial() ) : nullptr,
                            data->getFrame() );
}

template <typename CoreMeshType>
void SurfaceMeshComponent<CoreMeshType>::finalizeROFromGeometry(
    const Core::Asset::MaterialData* data,
    Core::Transform transform ) {
    // The technique for rendering this component
    std::shared_ptr<Data::Material> roMaterial;
    // First extract the material from asset or create a default one
    if ( data != nullptr ) {
        auto converter = Data::EngineMaterialConverters::getMaterialConverter( data->getType() );
        auto mat       = converter.second( data );
        roMaterial.reset( mat );
    }
    else {
        auto mat             = new Data::BlinnPhongMaterial( m_contentName + "_DefaultBPMaterial" );
        mat->m_renderAsSplat = m_displayMesh->getNumFaces() == 0;
        mat->m_perVertexColor = m_displayMesh->getCoreGeometry().hasAttrib(
            Data::Mesh::getAttribName( Data::Mesh::VERTEX_COLOR ) );
        roMaterial.reset( mat );
    }
    // initialize with a default rendertechique that draws nothing
    std::string roName( m_name + "_" + m_contentName + "_RO" );
    auto ro = Rendering::RenderObject::createRenderObject( roName,
                                                           this,
                                                           Rendering::RenderObjectType::Geometry,
                                                           m_displayMesh,
                                                           Rendering::RenderTechnique {} );
    ro->setTransparent( roMaterial->isTransparent() );
    ro->setMaterial( roMaterial );
    setupIO( m_contentName );
    ro->setLocalTransform( transform );
    m_roIndex = addRenderObject( ro );
}

#ifndef CHECK_MESH_NOT_NULL
#    define CHECK_MESH_NOT_NULL                \
        CORE_ASSERT( m_displayMesh != nullptr, \
                     "DisplayMesh should exist while component is alive" );
#    define CHECK_MESH_NOT_NULL_UNDEF
#endif

template <typename CoreMeshType>
const CoreMeshType& SurfaceMeshComponent<CoreMeshType>::getCoreGeometry() const {
    CHECK_MESH_NOT_NULL;
    return m_displayMesh->getCoreGeometry();
}

template <typename CoreMeshType>
typename SurfaceMeshComponent<CoreMeshType>::RenderMeshType*
SurfaceMeshComponent<CoreMeshType>::getDisplayable() {
    CHECK_MESH_NOT_NULL;
    return m_displayMesh.get();
}

template <typename CoreMeshType>
void SurfaceMeshComponent<CoreMeshType>::setupIO( const std::string& id ) {
    CHECK_MESH_NOT_NULL;

    const auto& cm = ComponentMessenger::getInstance();
    auto cbOut     = std::bind( &SurfaceMeshComponent<CoreMeshType>::getMeshOutput, this );
    auto cbRw      = std::bind( &SurfaceMeshComponent<CoreMeshType>::getMeshRw, this );

    cm->registerOutput<CoreMeshType>( getEntity(), this, id, cbOut );
    cm->registerReadWrite<CoreMeshType>( getEntity(), this, id, cbRw );

    base::setupIO( id );
}

template <typename CoreMeshType>
const CoreMeshType* SurfaceMeshComponent<CoreMeshType>::getMeshOutput() const {
    CHECK_MESH_NOT_NULL;
    return &m_displayMesh->getCoreGeometry();
}

template <typename CoreMeshType>
CoreMeshType* SurfaceMeshComponent<CoreMeshType>::getMeshRw() {
    CHECK_MESH_NOT_NULL;
    return &( m_displayMesh->getCoreGeometry() );
}

#ifdef CHECK_MESH_NOT_NULL_UNDEF
#    undef CHECK_MESH_NOT_NULL
#endif

} // namespace Scene
} // namespace Engine
} // namespace Ra
