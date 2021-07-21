#include <Core/Asset/Camera.hpp>
#include <Engine/Scene/CameraComponent.hpp>

#include <Core/Containers/MakeShared.hpp>
#include <Core/Math/Math.hpp>
#include <Engine/Data/Mesh.hpp>
#include <Engine/Data/PlainMaterial.hpp>
#include <Engine/Data/ShaderConfigFactory.hpp>
#include <Engine/Rendering/RenderObject.hpp>

namespace Ra {

using Core::Math::Pi;
using Core::Math::PiDiv2;
using Core::Math::PiDiv4;

namespace Engine {
namespace Scene {

CameraComponent::CameraComponent( Entity* entity,
                                  const std::string& name,
                                  Scalar height,
                                  Scalar width ) :
    Component( name, entity ) {
    m_camera = std::make_unique<Core::Asset::Camera>( height, width );
}

CameraComponent::~CameraComponent() = default;

void CameraComponent::initialize() {
    using Data::Material;
    using Data::Mesh;
    using Data::PlainMaterial;
    if ( !m_renderObjects.empty() ) return;
    // Create the render mesh for the camera

    Ra::Core::Geometry::LineMesh triMesh;
    triMesh.setVertices( { { 0_ra, 0_ra, 0_ra },
                           { -.5_ra, -.5_ra, -1_ra },
                           { -.5_ra, .5_ra, -1_ra },
                           { .5_ra, .5_ra, -1_ra },
                           { .5_ra, -.5_ra, -1_ra },
                           { -.3_ra, .5_ra, -1_ra },
                           { 0_ra, .7_ra, -1_ra },
                           { .3_ra, .5_ra, -1_ra } } );
    triMesh.setIndices( { { 0, 1 },
                          { 0, 2 },
                          { 0, 3 },
                          { 0, 4 },
                          { 1, 2 },
                          { 1, 4 },
                          { 2, 3 },
                          { 3, 4 },
                          { 5, 6 },
                          { 6, 7 } } );

    auto m = std::make_shared<Data::LineMesh>( m_name + "_mesh" );
    m->loadGeometry( std::move( triMesh ) );

    // Create the RO
    auto mat              = Core::make_shared<PlainMaterial>( m_name + "_Material" );
    mat->m_color          = { 1_ra, .5_ra, 0_ra, 1_ra };
    mat->m_perVertexColor = false;
    m_RO                  = Rendering::RenderObject::createRenderObject( m_name + "_RO",
                                                        this,
                                                        Rendering::RenderObjectType::Geometry,
                                                        m,
                                                        Rendering::RenderTechnique {} );

    m_RO->setLocalTransform( m_camera->getFrame() );

    m_RO->setMaterial( mat );
    show( false );
    m_RO->setPickable( true );
    addRenderObject( m_RO );
}

void CameraComponent::show( bool on ) {
    CORE_ASSERT( m_RO, "Camera's render object must be initialize with Camera::intialize()" );
    m_RO->setVisible( on );
}

void CameraComponent::updateTransform() {
    CORE_ASSERT( m_RO, "Camera's render object must be initialize with Camera::intialize()" );
    m_RO->setLocalTransform( m_camera->getFrame() *
                             Eigen::Scaling( 1_ra,
                                             1_ra / m_camera->getAspect(),
                                             .5_ra / std::tan( m_camera->getFOV() * .5_ra ) ) );
}

CameraComponent* CameraComponent::duplicate( Entity* cloneEntity,
                                             const std::string& cloneName ) const {
    auto cam =
        new CameraComponent { cloneEntity, cloneName, m_camera->getWidth(), m_camera->getHeight() };
    cam->getCamera()->setFrame( m_camera->getFrame() );
    cam->getCamera()->setProjMatrix( m_camera->getProjMatrix() );
    cam->getCamera()->setType( m_camera->getType() );
    cam->getCamera()->setZoomFactor( m_camera->getZoomFactor() );
    cam->getCamera()->setFOV( m_camera->getFOV() );
    cam->getCamera()->setZNear( m_camera->getZNear() );
    cam->getCamera()->setZFar( m_camera->getZFar() );
    cam->initialize();
    return cam;
}

} // namespace Scene
} // namespace Engine
} // namespace Ra
