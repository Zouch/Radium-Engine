#include <Gui/Viewer/Gizmo/Gizmo.hpp>

#include <Core/Asset/Camera.hpp>
#include <Core/Geometry/RayCast.hpp>
#include <Engine/Data/Mesh.hpp>
#include <Engine/Rendering/RenderObject.hpp>
#include <Engine/Rendering/RenderObjectManager.hpp>
#include <Engine/Scene/CameraComponent.hpp>

#include <Core/Containers/MakeShared.hpp>
#include <Engine/Data/PlainMaterial.hpp>
#include <Engine/Data/ShaderConfigFactory.hpp>
#include <Engine/Rendering/RenderTechnique.hpp>

namespace Ra {
namespace Gui {

std::array<std::shared_ptr<Ra::Engine::Data::PlainMaterial>, 3> Gizmo::s_material;

Gizmo::Gizmo( Engine::Scene::Component* c,
              const Core::Transform& worldTo,
              const Core::Transform& t,
              Mode mode ) :
    m_worldTo( worldTo ), m_transform( t ), m_comp( c ), m_mode( mode ) {
    using namespace Core::Utils;
    if ( !s_material[0] )
    {
        auto mat      = Core::make_shared<Engine::Data::PlainMaterial>( "GizmoRedMaterial" );
        mat->m_color  = Color::Red();
        s_material[0] = mat;
        mat           = Core::make_shared<Engine::Data::PlainMaterial>( "GizmoGreenMaterial" );
        mat->m_color  = Color::Green();
        s_material[1] = mat;
        mat           = Core::make_shared<Engine::Data::PlainMaterial>( "GizmoBlueMaterial" );
        mat->m_color  = Color::Blue();
        s_material[2] = mat;
    }
}

Gizmo::~Gizmo() {
    for ( auto ro : m_ros )
    { m_comp->removeRenderObject( ro->getIndex() ); }
}

void Gizmo::show( bool on ) {
    for ( auto ro : m_ros )
    { ro->setVisible( on ); }
}

bool Gizmo::findPointOnAxis( const Core::Asset::Camera& cam,
                             const Core::Vector3& origin,
                             const Core::Vector3& axis,
                             const Core::Vector2& pix,
                             Core::Vector3& pointOut,
                             std::vector<Scalar>& hits ) {
    // Taken from Rodolphe's View engine gizmos -- see slide_axis().
    // Find a plane containing axis and as parallel as possible to
    // the camera image plane
    const Core::Vector3 ortho = cam.getDirection().cross( axis );
    const Core::Vector3 normal =
        ( ortho.squaredNorm() > 0 ) ? axis.cross( ortho ) : axis.cross( cam.getUpVector() );

    const auto ray = cam.getRayFromScreen( pix );
    bool hasHit    = Core::Geometry::RayCastPlane( ray, origin, normal, hits );
    if ( hasHit ) { pointOut = origin + ( axis.dot( ray.pointAt( hits[0] ) - origin ) ) * axis; }
    return hasHit;
}

bool Gizmo::findPointOnPlane( const Core::Asset::Camera& cam,
                              const Core::Vector3& origin,
                              const Core::Vector3& axis,
                              const Core::Vector2& pix,
                              Core::Vector3& pointOut,
                              std::vector<Scalar>& hits ) {
    // Taken from Rodolphe's View engine gizmos -- see slide_plane().
    const auto ray = cam.getRayFromScreen( pix );
    bool hasHit    = Core::Geometry::RayCastPlane( ray, origin, axis, hits );
    if ( hasHit ) { pointOut = ray.pointAt( hits[0] ); }
    return hasHit;
}

void Gizmo::addRenderObject( Engine::Rendering::RenderObject* ro ) {
    m_comp->addRenderObject( ro );
    m_ros.push_back( ro );
}

std::shared_ptr<Engine::Rendering::RenderTechnique> Gizmo::makeRenderTechnique( int color ) {
    auto rt       = Core::make_shared<Engine::Rendering::RenderTechnique>();
    auto plaincfg = Ra::Engine::Data::ShaderConfigurationFactory::getConfiguration( "Plain" );
    auto provider = new Gizmo::UiSelectionControler( s_material[color] );
    rt->setConfiguration( *plaincfg );
    rt->setParametersProvider( std::shared_ptr<Engine::Data::ShaderParameterProvider>( provider ) );
    return rt;
}

Gizmo::UiSelectionControler* Gizmo::getControler( int ro ) {
    // TODO : can these casts be replaced by a improved design ?
    auto c = dynamic_cast<const Gizmo::UiSelectionControler*>(
        m_ros[ro]->getRenderTechnique()->getParametersProvider() );
    CORE_ASSERT( c != nullptr, "Gizmo not set (or with wrong state controler?" );
    return const_cast<Gizmo::UiSelectionControler*>( c );
}

Gizmo::UiSelectionControler::UiSelectionControler(
    std::shared_ptr<Ra::Engine::Data::PlainMaterial>& material,
    const Core::Utils::Color& selectedColor ) :
    ShaderParameterProvider(), m_associatedMaterial( material ), m_selectedColor( selectedColor ) {}

void Gizmo::UiSelectionControler::updateGL() {
    m_associatedMaterial->updateGL();
    m_renderParameters = m_associatedMaterial->getParameters();
    if ( m_selected ) { m_renderParameters.addParameter( "material.color", m_selectedColor ); }
}

void Gizmo::UiSelectionControler::toggleState() {
    m_selected = !m_selected;
}

void Gizmo::UiSelectionControler::setState() {
    m_selected = true;
}

void Gizmo::UiSelectionControler::clearState() {
    m_selected = false;
}

} // namespace Gui
} // namespace Ra
