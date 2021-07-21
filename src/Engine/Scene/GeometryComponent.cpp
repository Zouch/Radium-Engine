#include <Engine/Scene/GeometryComponent.hpp>

#include <iostream>

#include <Core/Asset/GeometryData.hpp>
#include <Core/Containers/MakeShared.hpp>
#include <Core/Geometry/Normal.hpp>
#include <Core/Utils/Color.hpp>
#include <Core/Utils/Log.hpp>

#include <Engine/Data/BlinnPhongMaterial.hpp>
#include <Engine/Data/Material.hpp>
#include <Engine/Data/MaterialConverters.hpp>
#include <Engine/Data/Mesh.hpp>
#include <Engine/Data/VolumeObject.hpp>
#include <Engine/Data/VolumetricMaterial.hpp>
#include <Engine/Rendering/RenderObject.hpp>
#include <Engine/Rendering/RenderObjectManager.hpp>
#include <Engine/Rendering/RenderObjectTypes.hpp>
#include <Engine/Rendering/RenderTechnique.hpp>
#include <Engine/Scene/ComponentMessenger.hpp>

#define CHECK_MESH_NOT_NULL \
    CORE_ASSERT( m_displayMesh != nullptr, "DisplayMesh should exist while component is alive" );

using TriangleArray = Ra::Core::VectorArray<Ra::Core::Vector3ui>;
using namespace Ra::Core::Utils;

namespace Ra {
namespace Engine {
namespace Scene {

void GeometryComponent::setupIO( const std::string& id ) {
    const auto& cm = ComponentMessenger::getInstance();
    auto roOut     = std::bind( &GeometryComponent::roIndexRead, this );
    cm->registerOutput<Index>( getEntity(), this, id, roOut );
}

const Index* GeometryComponent::roIndexRead() const {
    return &m_roIndex;
}

/*-----------------------------------------------------------------------------------------------*/
/*---------------------------------  PointCloud Component----------------------------------------*/
/*-----------------------------------------------------------------------------------------------*/

PointCloudComponent::PointCloudComponent( const std::string& name,
                                          Entity* entity,
                                          const Ra::Core::Asset::GeometryData* data ) :
    GeometryComponent( name, entity ), m_displayMesh( nullptr ) {
    generatePointCloud( data );
}

PointCloudComponent::PointCloudComponent( const std::string& name,
                                          Entity* entity,
                                          Core::Geometry::PointCloud&& mesh,
                                          Core::Asset::MaterialData* mat ) :
    GeometryComponent( name, entity ),
    m_displayMesh( new Data::PointCloud( name, std::move( mesh ) ) ) {
    finalizeROFromGeometry( mat, Core::Transform::Identity() );
}

//////////// STORE Mesh/PointCloud here instead of an index, so don't need to request the ROMgr
/// and no problem with the Displayable -> doesn't affect the API

PointCloudComponent::~PointCloudComponent() = default;

void PointCloudComponent::initialize() {}

void PointCloudComponent::generatePointCloud( const Ra::Core::Asset::GeometryData* data ) {
    m_contentName = data->getName();
    m_displayMesh = Ra::Core::make_shared<Data::PointCloud>( m_contentName );
    m_displayMesh->setRenderMode( Data::AttribArrayDisplayable::RM_POINTS );

    Ra::Core::Geometry::PointCloud mesh;
    Ra::Core::Geometry::PointCloud::PointAttribHandle::Container vertices;
    Ra::Core::Geometry::PointCloud::NormalAttribHandle::Container normals;

    vertices.reserve( data->getVerticesSize() );
    std::copy(
        data->getVertices().begin(), data->getVertices().end(), std::back_inserter( vertices ) );

    if ( data->hasNormals() )
    {
        normals.reserve( data->getVerticesSize() );
        std::copy(
            data->getNormals().begin(), data->getNormals().end(), std::back_inserter( normals ) );
    }

    mesh.setVertices( std::move( vertices ) );
    mesh.setNormals( std::move( normals ) );

    if ( data->hasTangents() )
    {
        mesh.addAttrib( Data::Mesh::getAttribName( Data::Mesh::VERTEX_TANGENT ),
                        data->getTangents() );
    }

    if ( data->hasBiTangents() )
    {
        mesh.addAttrib( Data::Mesh::getAttribName( Data::Mesh::VERTEX_BITANGENT ),
                        data->getBiTangents() );
    }

    if ( data->hasTextureCoordinates() )
    {
        mesh.addAttrib( Data::Mesh::getAttribName( Data::Mesh::VERTEX_TEXCOORD ),
                        data->getTexCoords() );
    }

    // add custom attribs
    mesh.vertexAttribs().copyAllAttributes( data->getAttribManager() );

    // To be discussed: Should not weights be part of the geometry ?
    //        mesh->addData( Data::Mesh::VERTEX_WEIGHTS, meshData.weights );

    m_displayMesh->loadGeometry( std::move( mesh ) );

    finalizeROFromGeometry( data->hasMaterial() ? &( data->getMaterial() ) : nullptr,
                            data->getFrame() );
}

void PointCloudComponent::finalizeROFromGeometry( const Core::Asset::MaterialData* data,
                                                  Core::Transform transform ) {
    // The technique for rendering this component
    std::shared_ptr<Data::Material> roMaterial;
    // First extract the material from asset or create a default one
    if ( data != nullptr )
    {
        auto converter = Data::EngineMaterialConverters::getMaterialConverter( data->getType() );
        auto mat       = converter.second( data );
        roMaterial.reset( mat );
    }
    else
    {
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

const Ra::Core::Geometry::PointCloud& PointCloudComponent::getCoreGeometry() const {
    CHECK_MESH_NOT_NULL;
    return m_displayMesh->getCoreGeometry();
}

Data::PointCloud* PointCloudComponent::getGeometry() {
    CHECK_MESH_NOT_NULL;
    return m_displayMesh.get();
}

void PointCloudComponent::setupIO( const std::string& id ) {
    CHECK_MESH_NOT_NULL;
    auto cbOut = std::bind( &PointCloudComponent::getMeshOutput, this );
    auto cbRw  = std::bind( &PointCloudComponent::getPointCloudRw, this );

    const auto& cm = ComponentMessenger::getInstance();

    cm->registerOutput<Ra::Core::Geometry::PointCloud>( getEntity(), this, id, cbOut );
    cm->registerReadWrite<Ra::Core::Geometry::PointCloud>( getEntity(), this, id, cbRw );
    base::setupIO( id );
}

const Ra::Core::Geometry::PointCloud* PointCloudComponent::getMeshOutput() const {
    CHECK_MESH_NOT_NULL;
    return &m_displayMesh->getCoreGeometry();
}

Ra::Core::Geometry::PointCloud* PointCloudComponent::getPointCloudRw() {
    CHECK_MESH_NOT_NULL;
    return &( m_displayMesh->getCoreGeometry() );
}

/*-----------------------------------------------------------------------------------------------*/
/*---------------------------------  Volume Component  ------------------------------------------*/
/*-----------------------------------------------------------------------------------------------*/
VolumeComponent::VolumeComponent( const std::string& name,
                                  Entity* entity,
                                  const Ra::Core::Asset::VolumeData* data ) :
    Component( name, entity ), m_displayVolume {nullptr} {
    generateVolumeRender( data );
}

VolumeComponent::~VolumeComponent() = default;

void VolumeComponent::initialize() {}

void VolumeComponent::generateVolumeRender( const Ra::Core::Asset::VolumeData* data ) {
    m_contentName   = data->getName();
    m_displayVolume = Ra::Core::make_shared<Data::VolumeObject>( m_contentName );
    m_displayVolume->loadGeometry( data->volume, data->boundingBox );

    auto roMaterial =
        Ra::Core::make_shared<Data::VolumetricMaterial>( data->getName() + "_VolMat" );
    roMaterial->setTexture( const_cast<Data::Texture*>( &( m_displayVolume->getDataTexture() ) ) );
    roMaterial->m_sigma_a       = data->sigma_a;
    roMaterial->m_sigma_s       = data->sigma_s;
    roMaterial->m_modelToMedium = data->densityToModel.inverse();

    std::string roName( m_name + "_" + m_contentName + "_RO" );
    auto ro = Rendering::RenderObject::createRenderObject( roName,
                                                           this,
                                                           Rendering::RenderObjectType::Geometry,
                                                           m_displayVolume,
                                                           Rendering::RenderTechnique {} );
    ro->setTransparent( roMaterial->isTransparent() );
    ro->setMaterial( roMaterial );
    ro->setLocalTransform( data->modelToWorld );

    setupIO( m_contentName );
    m_volumeIndex = addRenderObject( ro );
}

#define CHECK_VOL_NOT_NULL                   \
    CORE_ASSERT( m_displayVolume != nullptr, \
                 "DisplayVolume should exist while component is alive" );

Data::VolumeObject* VolumeComponent::getDisplayVolume() {
    CHECK_VOL_NOT_NULL;
    return m_displayVolume.get();
}

Index VolumeComponent::getRenderObjectIndex() const {
    CHECK_VOL_NOT_NULL;
    return m_volumeIndex;
}

void VolumeComponent::setContentName( const std::string& name ) {
    this->m_contentName = name;
}

void VolumeComponent::setupIO( const std::string& id ) {
    CHECK_VOL_NOT_NULL;

    const auto& cm = ComponentMessenger::getInstance();
    auto cbOut     = std::bind( &VolumeComponent::getVolumeOutput, this );
    auto cbRw      = std::bind( &VolumeComponent::getVolumeRw, this );
    auto roOut     = std::bind( &VolumeComponent::roIndexRead, this );

    cm->registerOutput<Ra::Core::Geometry::AbstractVolume>( getEntity(), this, id, cbOut );
    cm->registerReadWrite<Ra::Core::Geometry::AbstractVolume>( getEntity(), this, id, cbRw );
    cm->registerOutput<Ra::Core::Utils::Index>( getEntity(), this, id, roOut );
}

const Index* VolumeComponent::roIndexRead() const {
    CHECK_VOL_NOT_NULL;
    return &m_volumeIndex;
}

const Ra::Core::Geometry::AbstractVolume* VolumeComponent::getVolumeOutput() const {
    CHECK_VOL_NOT_NULL;
    return &m_displayVolume->getVolume();
}

Ra::Core::Geometry::AbstractVolume* VolumeComponent::getVolumeRw() {
    CHECK_VOL_NOT_NULL;
    return &m_displayVolume->getVolume();
}

} // namespace Scene
} // namespace Engine
} // namespace Ra
