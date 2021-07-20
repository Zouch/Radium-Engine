#include <Gui/Viewer/Gizmo/ScaleGizmo.hpp>

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

using namespace Ra::Core::Utils;

ScaleGizmo::ScaleGizmo( Engine::Scene::Component* c,
                        const Core::Transform& worldTo,
                        const Core::Transform& t,
                        Mode mode ) :
    Gizmo( c, worldTo, t, mode ),
    m_prevScale( Core::Vector3::Ones() ),
    m_startPoint( Core::Vector3::Zero() ),
    m_initialPix( Core::Vector2::Zero() ),
    m_selectedAxis( -1 ),
    m_selectedPlane( -1 ) {
    constexpr Scalar arrowScale = .1_ra;
    constexpr Scalar axisWidth  = .05_ra;
    constexpr Scalar arrowFrac  = .125_ra;
    constexpr Scalar radius     = arrowScale * axisWidth / 2_ra;

    for ( uint i = 0; i < 3; ++i )
    {
        Core::Vector3 cylinderEnd             = Core::Vector3::Zero();
        cylinderEnd[i]                        = ( 1_ra - arrowFrac );
        Core::Geometry::TriangleMesh cylinder = Core::Geometry::makeCylinder(
            Core::Vector3::Zero(), arrowScale * cylinderEnd, radius, 32 );
        Core::Aabb boxBounds;
        boxBounds.extend( Core::Vector3( -radius * 2_ra, -radius * 2_ra, -radius * 2_ra ) );
        boxBounds.extend( Core::Vector3( radius * 2_ra, radius * 2_ra, radius * 2_ra ) );
        boxBounds.translate( arrowScale * cylinderEnd );
        Core::Geometry::TriangleMesh box = Core::Geometry::makeSharpBox( boxBounds );
        cylinder.append( box );

        std::shared_ptr<Engine::Data::Mesh> mesh( new Engine::Data::Mesh( "Scale Gizmo Arrow" ) );
        mesh->loadGeometry( std::move( cylinder ) );

        Engine::Rendering::RenderObject* arrowDrawable = new Engine::Rendering::RenderObject(
            "Scale Gizmo Arrow", m_comp, Engine::Rendering::RenderObjectType::UI );
        auto rt = std::shared_ptr<Engine::Rendering::RenderTechnique>( makeRenderTechnique( i ) );
        arrowDrawable->setRenderTechnique( rt );
        arrowDrawable->setMesh( mesh );
        addRenderObject( arrowDrawable );
    }

    for ( uint i = 0; i < 3; ++i )
    {
        Core::Vector3 axis                        = Core::Vector3::Zero();
        axis[( i == 0 ? 1 : ( i == 1 ? 0 : 2 ) )] = 1;
        Core::Transform T                         = Core::Transform::Identity();
        T.rotate( Core::AngleAxis( Core::Math::PiDiv2, axis ) );
        T.translation()[( i + 1 ) % 3] += arrowFrac * arrowScale * 3;
        T.translation()[( i + 2 ) % 3] += arrowFrac * arrowScale * 3;

        Core::Geometry::TriangleMesh plane = Core::Geometry::makePlaneGrid(
            1, 1, Core::Vector2( arrowFrac * arrowScale, arrowFrac * arrowScale ), T );

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

void ScaleGizmo::updateTransform( Gizmo::Mode mode,
                                  const Core::Transform& worldTo,
                                  const Core::Transform& t ) {
    m_mode                           = mode;
    m_worldTo                        = worldTo;
    m_transform                      = t;
    Core::Transform displayTransform = Core::Transform::Identity();
    displayTransform.translate( m_transform.translation() );
    // always scale in LOCAL frame
    {
        Core::Matrix3 R = m_transform.rotation();
        R.col( 0 ).normalize();
        R.col( 1 ).normalize();
        R.col( 2 ).normalize();
        displayTransform.rotate( R );
    }

    for ( const auto& ro : ros() )
    { ro->setLocalTransform( m_worldTo * displayTransform ); }
}

void ScaleGizmo::selectConstraint( int drawableIdx ) {
    // deselect previously selected axis. Due to "whole" selection clear all states
    for ( uint i = 0; i < 6; ++i )
    { getControler( i )->clearState(); }
    // prepare selection
    m_selectedAxis  = -1;
    m_selectedPlane = -1;
    // Return if no component is selected
    if ( drawableIdx < 0 ) { return; }

    // update the state of the selected component
    auto found = std::find_if( ros().cbegin(), ros().cend(), [drawableIdx]( const auto& ro ) {
        return ro->getIndex() == Core::Utils::Index( drawableIdx );
    } );
    if ( found != ros().cend() )
    {
        int i = int( std::distance( ros().cbegin(), found ) );
        getControler( i )->setState();
        ( i < 3 ) ? m_selectedAxis = i : m_selectedPlane = i - 3;
        // Activate the axes correponding to the selected plane
        if ( m_selectedPlane != -1 )
        {
            getControler( ( m_selectedPlane + 1 ) % 3 )->setState();
            getControler( ( m_selectedPlane + 2 ) % 3 )->setState();
        }
    }
}

Core::Transform ScaleGizmo::mouseMove( const Core::Asset::Camera& cam,
                                       const Core::Vector2& nextXY,
                                       bool stepped,
                                       bool whole ) {
    static const Scalar step = .2_ra;

    // Recolor gizmo
    // TODO : this appearance management has nothing to do in this method.
    //  Find how to move this on the selectConstraint method.
    if ( whole )
    {
        for ( uint i = 0; i < 6; ++i )
        { getControler( i )->setState(); }
    }
    else
    {
        for ( uint i = 0; i < 6; ++i )
        { getControler( i )->clearState(); }
        if ( m_selectedAxis > -1 ) { getControler( m_selectedAxis )->setState(); }

        if ( m_selectedPlane > -1 )
        {
            getControler( m_selectedPlane + 3 )->setState();
            // Activate the axes correponding to the selected plane
            getControler( ( m_selectedPlane + 1 ) % 3 )->setState();
            getControler( ( m_selectedPlane + 2 ) % 3 )->setState();
        }
    }

    // T
    if ( m_selectedAxis == -1 && m_selectedPlane == -1 && !whole ) { return m_transform; }

    // Get gizmo center and translation axis / plane normal
    int dir                    = whole ? 0 : std::max( m_selectedAxis, m_selectedPlane );
    const Core::Vector3 origin = m_transform.translation();
    Core::Vector3 scaleDir = Core::Vector3( m_transform.rotation() * Core::Vector3::Unit( dir ) );

    // Project the clicked points against the axis defined by the scale axis,
    // or the planes defined by the scale plane.
    std::vector<Scalar> hits;
    bool found = false;
    Core::Vector3 endPoint;
    if ( whole )
    { found = findPointOnPlane( cam, origin, scaleDir, m_initialPix + nextXY, endPoint, hits ); }
    else if ( m_selectedAxis > -1 )
    { found = findPointOnAxis( cam, origin, scaleDir, m_initialPix + nextXY, endPoint, hits ); }
    else if ( m_selectedPlane >= 0 )
    { found = findPointOnPlane( cam, origin, scaleDir, m_initialPix + nextXY, endPoint, hits ); }
    if ( !found ) { return m_transform; }

    // Initialize scale
    if ( !m_start )
    {
        m_start      = true;
        m_startPos   = m_transform.translation();
        m_prevScale  = Core::Vector3::Ones();
        m_startPoint = endPoint;
    }

    // Prevent scale == 0
    const Scalar a = ( endPoint - m_startPos ).squaredNorm();
    if ( a < 1e-3_ra ) { return m_transform; }

    // Get scale value
    const Core::Vector3 b = m_startPoint - m_startPos;
    Scalar scale          = std::sqrt( a ) / b.norm();
    if ( stepped ) { scale = int( scale / step + 1 ) * step; }

    // Apply scale
    m_transform.scale( m_prevScale );
    if ( whole ) { m_prevScale = scale * Core::Vector3::Ones(); }
    else if ( m_selectedAxis > -1 )
    {
        m_prevScale                 = Core::Vector3::Ones();
        m_prevScale[m_selectedAxis] = scale;
    }
    else if ( m_selectedPlane >= 0 )
    {
        m_prevScale                              = Core::Vector3::Ones();
        m_prevScale[( m_selectedPlane + 1 ) % 3] = scale;
        m_prevScale[( m_selectedPlane + 2 ) % 3] = scale;
    }
    m_transform.scale( m_prevScale );
    m_prevScale = m_prevScale.cwiseInverse();

    return m_transform;
}

void ScaleGizmo::setInitialState( const Core::Asset::Camera& /*cam*/,
                                  const Core::Vector2& /*initialXY*/ ) {
    m_initialPix = Core::Vector2::Zero();
    m_start      = false;
}

} // namespace Gui
} // namespace Ra
