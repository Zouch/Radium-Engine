#include <Gui/Viewer/Gizmo/TranslateGizmo.hpp>

#include <Core/Containers/VectorArray.hpp>
#include <Core/Geometry/MeshPrimitives.hpp>
#include <Core/Utils/Color.hpp>

#include <Core/Asset/Camera.hpp>
#include <Engine/Data/Mesh.hpp>
#include <Engine/Rendering/RenderObject.hpp>
#include <Engine/Rendering/RenderTechnique.hpp>
#include <Engine/Scene/CameraComponent.hpp>

namespace Ra {
namespace Gui {

TranslateGizmo::TranslateGizmo( Engine::Scene::Component* c,
                                const Core::Transform& worldTo,
                                const Core::Transform& t,
                                Mode mode ) :
    Gizmo( c, worldTo, t, mode ),
    m_startPoint( Core::Vector3::Zero() ),
    m_initialPix( Core::Vector2::Zero() ),
    m_selectedAxis( -1 ),
    m_selectedPlane( -1 ) {
    constexpr Scalar arrowScale = .1_ra;
    constexpr Scalar axisWidth  = .05_ra;
    constexpr Scalar arrowFrac  = .15_ra;

    for ( uint i = 0; i < 3; ++i ) {
        Core::Vector3 cylinderEnd             = Core::Vector3::Zero();
        cylinderEnd[i]                        = ( 1_ra - arrowFrac );
        Core::Vector3 arrowEnd                = Core::Vector3::Zero();
        arrowEnd[i]                           = 1_ra;
        Core::Geometry::TriangleMesh cylinder = Core::Geometry::makeCylinder(
            Core::Vector3::Zero(), arrowScale * cylinderEnd, arrowScale * axisWidth / 2_ra, 32 );
        Core::Geometry::TriangleMesh cone = Core::Geometry::makeCone(
            arrowScale * cylinderEnd, arrowScale * arrowEnd, arrowScale * arrowFrac / 2_ra, 32 );
        cylinder.append( cone );

        std::shared_ptr<Engine::Data::Mesh> mesh(
            new Engine::Data::Mesh( "Translate Gizmo Arrow" ) );
        mesh->loadGeometry( std::move( cylinder ) );

        Engine::Rendering::RenderObject* arrowDrawable = new Engine::Rendering::RenderObject(
            "Translate Gizmo Arrow", m_comp, Engine::Rendering::RenderObjectType::UI );
        auto rt = std::shared_ptr<Engine::Rendering::RenderTechnique>( makeRenderTechnique( i ) );
        arrowDrawable->setRenderTechnique( rt );
        arrowDrawable->setMesh( mesh );
        addRenderObject( arrowDrawable );
    }

    for ( uint i = 0; i < 3; ++i ) {
        Core::Vector3 axis                        = Core::Vector3::Zero();
        axis[( i == 0 ? 1 : ( i == 1 ? 0 : 2 ) )] = 1;
        Core::Transform T                         = Core::Transform::Identity();
        T.rotate( Core::AngleAxis( Core::Math::PiDiv2, axis ) );
        T.translation()[( i + 1 ) % 3] += arrowScale / 8_ra * 3_ra;
        T.translation()[( i + 2 ) % 3] += arrowScale / 8_ra * 3_ra;

        Core::Geometry::TriangleMesh plane = Core::Geometry::makePlaneGrid(
            1, 1, Core::Vector2( arrowScale / 8_ra, arrowScale / 8_ra ), T );

        // \FIXME this hack is here to be sure the plane will be selected (see shader)

        auto& normals = plane.normalsWithLock();
        normals.getMap().fill( 0_ra );
        plane.normalsUnlock();

        std::shared_ptr<Engine::Data::Mesh> mesh( new Engine::Data::Mesh( "Gizmo Plane" ) );
        mesh->loadGeometry( std::move( plane ) );

        Engine::Rendering::RenderObject* planeDrawable = new Engine::Rendering::RenderObject(
            "Gizmo Plane", m_comp, Engine::Rendering::RenderObjectType::UI );
        auto rt = std::shared_ptr<Engine::Rendering::RenderTechnique>( makeRenderTechnique( i ) );
        planeDrawable->setRenderTechnique( rt );
        planeDrawable->setMesh( mesh );
        addRenderObject( planeDrawable );
    }

    updateTransform( mode, m_worldTo, m_transform );
}

void TranslateGizmo::updateTransform( Gizmo::Mode mode,
                                      const Core::Transform& worldTo,
                                      const Core::Transform& t ) {
    m_mode                           = mode;
    m_worldTo                        = worldTo;
    m_transform                      = t;
    Core::Transform displayTransform = Core::Transform::Identity();
    displayTransform.translate( m_transform.translation() );
    if ( m_mode == LOCAL ) {
        Core::Matrix3 R = m_transform.rotation();
        R.col( 0 ).normalize();
        R.col( 1 ).normalize();
        R.col( 2 ).normalize();
        displayTransform.rotate( R );
    }

    for ( auto ro : ros() ) {
        ro->setLocalTransform( m_worldTo * displayTransform );
    }
}

void TranslateGizmo::selectConstraint( int drawableIdx ) {
    // deselect previously selected axis
    if ( m_selectedAxis != -1 ) {
        getControler( m_selectedAxis )->clearState();
        m_selectedAxis = -1;
    }
    // deselect previously selected plane
    if ( m_selectedPlane != -1 ) {
        getControler( m_selectedPlane + 3 )->clearState();
        getControler( ( m_selectedPlane + 1 ) % 3 )->clearState();
        getControler( ( m_selectedPlane + 2 ) % 3 )->clearState();
        m_selectedPlane = -1;
    }

    // Return if no component is selected
    if ( drawableIdx < 0 ) { return; }

    // update the state of the selected component
    auto found = std::find_if( ros().cbegin(), ros().cend(), [drawableIdx]( const auto& ro ) {
        return ro->getIndex() == Core::Utils::Index( drawableIdx );
    } );
    if ( found != ros().cend() ) {
        int i = int( std::distance( ros().cbegin(), found ) );
        getControler( i )->setState();
        ( i < 3 ) ? m_selectedAxis = i : m_selectedPlane = i - 3;
        // Activate the axes correponding to the selected plane
        if ( m_selectedPlane != -1 ) {
            getControler( ( m_selectedPlane + 1 ) % 3 )->setState();
            getControler( ( m_selectedPlane + 2 ) % 3 )->setState();
        }
    }
}

Core::Transform TranslateGizmo::mouseMove( const Core::Asset::Camera& cam,
                                           const Core::Vector2& nextXY,
                                           bool stepped,
                                           bool whole ) {
    CORE_UNUSED( whole );
    static const Scalar step = .2_ra;

    if ( m_selectedAxis == -1 && m_selectedPlane == -1 ) return m_transform;

    // Get gizmo center and translation axis / plane normal
    std::vector<Scalar> hits;
    int axis                   = std::max( m_selectedAxis, m_selectedPlane );
    const Core::Vector3 origin = m_transform.translation();
    const Core::Vector3 translateDir =
        m_mode == LOCAL ? Core::Vector3( m_transform.rotation() * Core::Vector3::Unit( axis ) )
                        : Core::Vector3::Unit( axis );

    // Project the clicked points against the axis defined by the translation axis,
    // or the planes defined by the translation plane.
    Ra::Core::Vector3 endPoint;
    bool found = false;
    if ( m_selectedAxis > -1 ) {
        found = findPointOnAxis( cam, origin, translateDir, m_initialPix + nextXY, endPoint, hits );
    }
    else if ( m_selectedPlane > -1 ) {
        found =
            findPointOnPlane( cam, origin, translateDir, m_initialPix + nextXY, endPoint, hits );
    }

    if ( found ) {
        // Initialize translation
        if ( !m_start ) {
            m_start        = true;
            m_startPoint   = endPoint;
            m_initialTrans = origin;
        }

        // Apply translation
        Ra::Core::Vector3 tr = endPoint - m_startPoint;
        if ( stepped ) { tr = int( tr.norm() / step ) * step * tr.normalized(); }
        m_transform.translation() = m_initialTrans + tr;
    }

    return m_transform;
}

void TranslateGizmo::setInitialState( const Core::Asset::Camera& /*cam*/,
                                      const Core::Vector2& /*initialXY*/ ) {
    m_initialPix = Core::Vector2::Zero();
    m_start      = false;
}

} // namespace Gui
} // namespace Ra
