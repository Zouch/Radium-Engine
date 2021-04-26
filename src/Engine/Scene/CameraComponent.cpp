#include <Core/Utils/Camera.hpp>
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
    m_camera = std::make_unique<Core::Utils::Camera>( height, width );
}

CameraComponent::~CameraComponent() = default;

void CameraComponent::initialize() {
    using Data::Material;
    using Data::Mesh;
    using Data::PlainMaterial;
    if ( !m_renderObjects.empty() ) return;
    // Create the render mesh for the camera
    auto m = std::make_shared<Mesh>( m_name + "_mesh" );
    Ra::Core::Geometry::TriangleMesh triMesh;
    triMesh.setVertices( {{0_ra, 0_ra, 0_ra},
                          {-.5_ra, -.5_ra, -1_ra},
                          {-.5_ra, .5_ra, -1_ra},
                          {.5_ra, .5_ra, -1_ra},
                          {.5_ra, -.5_ra, -1_ra},
                          {-.3_ra, .5_ra, -1_ra},
                          {0_ra, .7_ra, -1_ra},
                          {.3_ra, .5_ra, -1_ra}} );
    triMesh.setIndices( {{0, 1, 2}, {0, 2, 3}, {0, 3, 4}, {0, 4, 1}, {5, 6, 7}} );
    Core::Vector4Array c( 8, {.2_ra, .2_ra, .2_ra, 1_ra} );
    triMesh.addAttrib( Mesh::getAttribName( Mesh::VERTEX_COLOR ), c );

    m->loadGeometry( std::move( triMesh ) );

    // Create the RO
    auto mat = Core::make_shared<PlainMaterial>( m_name + "_Material" );
    /// \todo switch to "mat->m_color          = {.2_ra, .2_ra, .2_ra, 1_ra};"
    mat->m_perVertexColor = true;
    Rendering::RenderTechnique rt;
    auto cfg = Data::ShaderConfigurationFactory::getConfiguration( "Plain" );
    rt.setConfiguration( *cfg );
    rt.setParametersProvider( mat );
    m_RO = Rendering::RenderObject::createRenderObject(
        m_name + "_RO", this, Rendering::RenderObjectType::Debug, m, rt );
    m_RO->setLocalTransform( m_camera->getFrame() );
    m_RO->setMaterial( mat );
    show( false );
    addRenderObject( m_RO );
}

void CameraComponent::show( bool on ) {
    CORE_ASSERT( m_RO, "Camera's render object must be initialize with Camera::intialize()" );
    m_RO->setVisible( on );
}

void CameraComponent::applyTransform( const Core::Transform& T ) {
    CORE_ASSERT( m_RO, "Camera's render object must be initialize with Camera::intialize()" );
    m_camera->applyTransform( T );
    m_RO->setLocalTransform( m_camera->getFrame() );
}

void CameraComponent::updateProjMatrix() {
    m_camera->updateProjMatrix();
}

Core::Ray CameraComponent::getRayFromScreen( const Core::Vector2& pix ) const {
    // Ray starts from the camera's current position.
    return m_camera->getRayFromScreen( pix );
}

CameraComponent* CameraComponent::duplicate( Entity* cloneEntity,
                                             const std::string& cloneName ) const {
    auto cam =
        new CameraComponent {cloneEntity, cloneName, m_camera->getWidth(), m_camera->getHeight()};
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

void CameraComponent::fitZRange( const Core::Aabb& aabb ) {
    m_camera->fitZRange( aabb );
}

} // namespace Scene
} // namespace Engine
} // namespace Ra