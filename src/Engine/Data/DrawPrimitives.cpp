#include <Engine/Data/DrawPrimitives.hpp>

#include <Core/Geometry/MeshPrimitives.hpp>
#include <Core/Utils/Color.hpp>

#include <Engine/Data/Mesh.hpp>
#include <Engine/Data/ShaderConfigFactory.hpp>
#include <Engine/Rendering/RenderObject.hpp>
#include <Engine/Rendering/RenderTechnique.hpp>

#include <Core/Containers/MakeShared.hpp>
#include <Engine/Data/PlainMaterial.hpp>

#include <algorithm>
#include <numeric>

namespace Ra {

using namespace Core;
using namespace Core::Geometry;

namespace Engine {
namespace Data {
namespace DrawPrimitives {

Rendering::RenderObject* Primitive( Scene::Component* component, const MeshPtr& mesh ) {
    return Primitive( component, std::dynamic_pointer_cast<Data::AttribArrayDisplayable>( mesh ) );
}

Rendering::RenderObject* Primitive( Scene::Component* component, const LineMeshPtr& mesh ) {
    return Primitive( component, std::dynamic_pointer_cast<Data::AttribArrayDisplayable>( mesh ) );
}

Rendering::RenderObject* Primitive( Scene::Component* component,
                                    const AttribArrayDisplayablePtr& mesh ) {
    Rendering::RenderTechnique rt;
    auto builder = Rendering::EngineRenderTechniques::getDefaultTechnique( "Plain" );
    builder.second( rt, false );
    auto roMaterial = Core::make_shared<PlainMaterial>( "Default material" );
    roMaterial->m_perVertexColor =
        mesh->getAttribArrayGeometry().hasAttrib( Mesh::getAttribName( Mesh::VERTEX_COLOR ) );
    rt.setParametersProvider( roMaterial );

    auto ro = Rendering::RenderObject::createRenderObject(
        mesh->getName(), component, Rendering::RenderObjectType::Debug, mesh, rt );
    ro->setMaterial( roMaterial );
    return ro;
}

LineMeshPtr Point( const Core::Vector3& point, const Core::Utils::Color& color, Scalar scale ) {

    Geometry::LineMesh geom;
    geom.setVertices( { ( point + ( scale * Core::Vector3::UnitX() ) ),
                        ( point - ( scale * Core::Vector3::UnitX() ) ),
                        ( point + ( scale * Core::Vector3::UnitY() ) ),
                        ( point - ( scale * Core::Vector3::UnitY() ) ),
                        ( point + ( scale * Core::Vector3::UnitZ() ) ),
                        ( point - ( scale * Core::Vector3::UnitZ() ) ) } );
    geom.setIndices( { { 0, 1 }, { 2, 3 }, { 4, 5 } } );
    geom.addAttrib( Mesh::getAttribName( Mesh::VERTEX_COLOR ),
                    Core::Vector4Array { geom.vertices().size(), color } );

    return make_shared<LineMesh>( "Point Primitive", std::move( geom ) );
}

LineMeshPtr
Line( const Core::Vector3& a, const Core::Vector3& b, const Core::Utils::Color& color ) {
    Geometry::LineMesh geom;
    geom.setVertices( { a, b } );
    geom.setIndices( { { 0, 1 } } );
    geom.addAttrib( Mesh::getAttribName( Mesh::VERTEX_COLOR ),
                    Core::Vector4Array { geom.vertices().size(), color } );

    return make_shared<LineMesh>( "Line Primitive", std::move( geom ) );
}

LineMeshPtr
Vector( const Core::Vector3& start, const Core::Vector3& v, const Core::Utils::Color& color ) {
    Core::Vector3 end = start + v;
    Core::Vector3 a, b;
    Core::Math::getOrthogonalVectors( v.normalized(), a, b );
    a.normalize();
    Scalar l = v.norm();

    const Scalar arrowFract = 0.1f;

    Geometry::LineMesh geom;
    geom.setVertices( { start,
                        end,
                        start + ( ( 1.f - arrowFract ) * v ) + ( ( arrowFract * l ) * a ),
                        start + ( ( 1.f - arrowFract ) * v ) - ( ( arrowFract * l ) * a ) } );
    geom.setIndices( { { 0, 1 }, { 1, 2 }, { 1, 3 } } );
    geom.addAttrib( Mesh::getAttribName( Mesh::VERTEX_COLOR ),
                    Core::Vector4Array { geom.vertices().size(), color } );

    return make_shared<LineMesh>( "Vector Primitive", std::move( geom ) );
}

LineMeshPtr Ray( const Core::Ray& ray, const Core::Utils::Color& color, Scalar len ) {
    Geometry::LineMesh geom;
    Core::Vector3 end = ray.pointAt( len );
    geom.setVertices( { ray.origin(), end } );
    geom.setIndices( { { 0, 1 } } );
    geom.addAttrib( Mesh::getAttribName( Mesh::VERTEX_COLOR ),
                    Core::Vector4Array { geom.vertices().size(), color } );
    return make_shared<LineMesh>( "Ray Primitive", std::move( geom ) );
}

AttribArrayDisplayablePtr Triangle( const Core::Vector3& a,
                                    const Core::Vector3& b,
                                    const Core::Vector3& c,
                                    const Core::Utils::Color& color,
                                    bool fill ) {
    if ( fill )
    {
        Geometry::TriangleMesh geom;
        geom.setVertices( { a, b, c } );
        geom.setIndices( { { 0, 1, 2 } } );
        geom.addAttrib( Mesh::getAttribName( Mesh::VERTEX_COLOR ),
                        Core::Vector4Array { geom.vertices().size(), color } );
        return make_shared<Mesh>( "Triangle Primitive", std::move( geom ) );
    }
    else
    {
        Geometry::LineMesh geom;
        geom.setVertices( { a, b, c } );
        geom.setIndices( { { 0, 1 }, { 1, 2 }, { 2, 0 } } );
        geom.addAttrib( Mesh::getAttribName( Mesh::VERTEX_COLOR ),
                        Core::Vector4Array { geom.vertices().size(), color } );
        return make_shared<LineMesh>( "Triangle Primitive", std::move( geom ) );
    }
}

/// \todo continue to convert mesh creation and remove call to deprecated Mesh::loadGeometry
MeshPtr QuadStrip( const Core::Vector3& a,
                   const Core::Vector3& x,
                   const Core::Vector3& y,
                   uint quads,
                   const Core::Utils::Color& color ) {
    Core::Vector3Array vertices( quads * 2 + 2 );
    std::vector<uint> indices( quads * 2 + 2 );

    Core::Vector3 B = a;
    vertices[0]     = B;
    vertices[1]     = B + x;
    indices[0]      = 0;
    indices[1]      = 1;
    for ( uint i = 0; i < quads; ++i )
    {
        B += y;
        vertices[2 * i + 2] = B;
        vertices[2 * i + 3] = B + x;
        indices[2 * i + 2]  = 2 * i + 2;
        indices[2 * i + 3]  = 2 * i + 3;
    }

    Core::Vector4Array colors( vertices.size(), color );

    MeshPtr mesh( new Mesh( "Quad Strip Primitive", Mesh::RM_TRIANGLE_STRIP ) );
    mesh->loadGeometry( vertices, indices );
    mesh->getCoreGeometry().addAttrib( Mesh::getAttribName( Mesh::VERTEX_COLOR ), colors );
    return mesh;
}

LineMeshPtr Circle( const Core::Vector3& center,
                    const Core::Vector3& normal,
                    Scalar radius,
                    uint segments,
                    const Core::Utils::Color& color ) {
    CORE_ASSERT( segments >= 2, "Cannot draw a circle with less than 3 points" );

    Geometry::LineMesh geom;
    ///\todo refer to class typedef instaed of core types/
    Core::Vector3Array vertices( segments + 1 );
    Geometry::LineMesh::IndexContainerType indices;
    indices.resize( segments );

    Core::Vector3 xPlane, yPlane;
    Core::Math::getOrthogonalVectors( normal, xPlane, yPlane );
    xPlane.normalize();
    yPlane.normalize();

    Scalar thetaInc( Core::Math::PiMul2 / Scalar( segments ) );
    Scalar theta( 0.0 );
    vertices[0] = center + radius * ( std::cos( theta ) * xPlane + std::sin( theta ) * yPlane );
    theta += thetaInc;

    for ( uint i = 1; i <= segments; ++i )
    {
        vertices[i] = center + radius * ( std::cos( theta ) * xPlane + std::sin( theta ) * yPlane );
        indices[i - 1] = { i - 1, i };
        theta += thetaInc;
    }

    geom.setVertices( std::move( vertices ) );
    geom.setIndices( std::move( indices ) );
    geom.addAttrib( Mesh::getAttribName( Mesh::VERTEX_COLOR ),
                    Core::Vector4Array { geom.vertices().size(), color } );

    return make_shared<LineMesh>( "Circle Primitive", std::move( geom ) );
}

LineMeshPtr CircleArc( const Core::Vector3& center,
                       const Core::Vector3& normal,
                       Scalar radius,
                       Scalar angle,
                       uint segments,
                       const Core::Utils::Color& color ) {

    Geometry::LineMesh geom;
    Core::Vector3Array vertices( segments + 1 );
    Geometry::LineMesh::IndexContainerType indices;
    indices.resize( segments );

    Core::Vector3 xPlane, yPlane;
    Core::Math::getOrthogonalVectors( normal, xPlane, yPlane );
    xPlane.normalize();
    yPlane.normalize();

    Scalar thetaInc( 2 * angle / Scalar( segments ) );
    Scalar theta( 0.0 );
    vertices[0] = center + radius * ( std::cos( theta ) * xPlane + std::sin( theta ) * yPlane );
    theta += thetaInc;

    for ( uint i = 1; i <= segments; ++i )
    {
        vertices[i] = center + radius * ( std::cos( theta ) * xPlane + std::sin( theta ) * yPlane );
        indices[i - 1] = { i - 1, i };

        theta += thetaInc;
    }

    geom.setVertices( std::move( vertices ) );
    geom.setIndices( std::move( indices ) );
    geom.addAttrib( Mesh::getAttribName( Mesh::VERTEX_COLOR ),
                    Core::Vector4Array { geom.vertices().size(), color } );

    return make_shared<LineMesh>( "Arc Circle Primitive", std::move( geom ) );
}

MeshPtr Sphere( const Core::Vector3& center, Scalar radius, const Core::Utils::Color& color ) {
    auto geom      = makeGeodesicSphere( radius, 2 );
    auto handle    = geom.getAttribHandle<TriangleMesh::Point>( "in_position" );
    auto& vertices = geom.getAttrib<TriangleMesh::Point>( handle );
    auto& data     = vertices.getDataWithLock();

    std::for_each( data.begin(), data.end(), [center]( Core::Vector3& v ) { v += center; } );
    vertices.unlock();

    geom.addAttrib( Mesh::getAttribName( Mesh::VERTEX_COLOR ),
                    Core::Vector4Array { geom.vertices().size(), color } );

    return make_shared<Mesh>( "Sphere Primitive", std::move( geom ) );
}

MeshPtr Capsule( const Core::Vector3& p1,
                 const Core::Vector3& p2,
                 Scalar radius,
                 const Core::Utils::Color& color ) {
    const Scalar l = ( p2 - p1 ).norm();

    TriangleMesh geom = makeCapsule( l, radius );

    // Compute the transform so that
    // (0,0,-l/2) maps to p1 and (0,0,l/2) maps to p2

    const Core::Vector3 trans = 0.5f * ( p2 + p1 );
    Core::Quaternion rot =
        Core::Quaternion::FromTwoVectors( Core::Vector3 { 0, 0, l / 2 }, 0.5f * ( p1 - p2 ) );

    Core::Transform t = Core::Transform::Identity();
    t.rotate( rot );
    t.pretranslate( trans );
    Matrix3 normalMatrix = t.linear().inverse().transpose();

    auto vertHandle  = geom.getAttribHandle<TriangleMesh::Point>( "in_position" );
    auto& vertAttrib = geom.getAttrib<TriangleMesh::Point>( vertHandle );
    auto& vertData   = vertAttrib.getDataWithLock();
    std::for_each( vertData.begin(), vertData.end(), [t]( Core::Vector3& v ) { v = t * v; } );
    vertAttrib.unlock();

    auto normalHandle  = geom.getAttribHandle<TriangleMesh::Point>( "in_normal" );
    auto& normalAttrib = geom.getAttrib<TriangleMesh::Point>( normalHandle );
    auto& normalData   = normalAttrib.getDataWithLock();
    std::for_each( normalData.begin(), normalData.end(), [normalMatrix]( Core::Vector3& n ) {
        n = normalMatrix * n;
    } );
    normalAttrib.unlock();

    geom.addAttrib( Mesh::getAttribName( Mesh::VERTEX_COLOR ),
                    Core::Vector4Array { geom.vertices().size(), color } );

    return make_shared<Mesh>( "Capsule Primitive", std::move( geom ) );
}

MeshPtr Disk( const Core::Vector3& center,
              const Core::Vector3& normal,
              Scalar radius,
              uint segments,
              const Core::Utils::Color& color ) {
    CORE_ASSERT( segments > 2, "Cannot draw a circle with less than 3 points" );

    uint seg = segments + 1;
    Core::Vector3Array vertices( seg );
    std::vector<uint> indices( seg + 1 );

    Core::Vector3 xPlane, yPlane;
    Core::Math::getOrthogonalVectors( normal, xPlane, yPlane );
    xPlane.normalize();
    yPlane.normalize();

    Scalar thetaInc( Core::Math::PiMul2 / Scalar( segments ) );
    Scalar theta( 0.0 );

    vertices[0] = center;
    indices[0]  = 0;
    for ( uint i = 1; i < seg; ++i )
    {
        vertices[i] = center + radius * ( std::cos( theta ) * xPlane + std::sin( theta ) * yPlane );
        indices[i]  = i;

        theta += thetaInc;
    }
    indices[seg] = 1;

    Core::Vector4Array colors( vertices.size(), color );

    MeshPtr mesh( new Mesh( "Disk Primitive", Mesh::RM_TRIANGLE_FAN ) );
    mesh->loadGeometry( vertices, indices );
    mesh->getCoreGeometry().addAttrib( Mesh::getAttribName( Mesh::VERTEX_COLOR ), colors );

    return mesh;
}

LineMeshPtr Normal( const Core::Vector3& point,
                    const Core::Vector3& normal,
                    const Core::Utils::Color& color,
                    Scalar scale ) {
    // Display an arrow (just like the Vector() function)
    // plus the normal plane.
    Core::Vector3 a, b;
    Core::Vector3 n = normal.normalized();
    Core::Math::getOrthogonalVectors( n, a, b );

    n                 = scale * n;
    Core::Vector3 end = point + n;
    a.normalize();
    b.normalize();

    const Scalar arrowFract = 0.1f;

    Geometry::LineMesh geom;

    Core::Vector3Array vertices = {
        point,
        end,
        point + ( ( 1.f - arrowFract ) * n ) + ( ( arrowFract * scale ) * a ),
        point + ( ( 1.f - arrowFract ) * n ) - ( ( arrowFract * scale ) * a ),

        point + ( scale * a ),
        point + ( scale * b ),
        point - ( scale * a ),
        point - ( scale * b ),
    };

    geom.setVertices( vertices );
    geom.setIndices( { { 0, 1 },
                       { 1, 2 },
                       { 1, 3 },
                       { 4, 5 },
                       { 5, 6 },
                       { 6, 7 },
                       { 7, 4 },
                       { 4, 6 },
                       { 5, 7 } } );
    geom.addAttrib( Mesh::getAttribName( Mesh::VERTEX_COLOR ),
                    Core::Vector4Array { geom.vertices().size(), color } );

    return make_shared<LineMesh>( "Normal Primitive", std::move( geom ) );
}

MeshPtr Frame( const Core::Transform& frameFromEntity, Scalar scale ) {
    // Frame is a bit different from the others
    // since there are 3 lines of different colors.
    Core::Vector3 pos = frameFromEntity.translation();
    Core::Vector3 x   = frameFromEntity.linear() * Core::Vector3::UnitX();
    Core::Vector3 y   = frameFromEntity.linear() * Core::Vector3::UnitY();
    Core::Vector3 z   = frameFromEntity.linear() * Core::Vector3::UnitZ();

    Core::Vector3Array vertices = {
        pos, pos + scale * x, pos, pos + scale * y, pos, pos + scale * z };

    Core::Vector4Array colors = {
        Core::Utils::Color::Red(),
        Core::Utils::Color::Red(),
        Core::Utils::Color::Green(),
        Core::Utils::Color::Green(),
        Core::Utils::Color::Blue(),
        Core::Utils::Color::Blue(),
    };

    std::vector<uint> indices = { 0, 1, 2, 3, 4, 5 };
    MeshPtr mesh( new Mesh( "Frame Primitive", Mesh::RM_LINES ) );

    mesh->loadGeometry( vertices, indices );
    mesh->getCoreGeometry().addAttrib( Mesh::getAttribName( Mesh::VERTEX_COLOR ), colors );

    return mesh;
}

MeshPtr Grid( const Core::Vector3& center,
              const Core::Vector3& x,
              const Core::Vector3& y,
              const Core::Utils::Color& color,
              Scalar cellSize,
              uint res ) {

    CORE_ASSERT( res > 1, "Grid has to be at least a 2x2 grid." );
    Core::Vector3Array vertices;
    std::vector<uint> indices;

    const Scalar halfWidth { ( cellSize * res ) / 2.f };
    const Core::Vector3 deltaPosX { cellSize * x };
    const Core::Vector3 startPosX { center - halfWidth * x };
    const Core::Vector3 endPosX { center + halfWidth * x };
    const Core::Vector3 deltaPosY { cellSize * y };
    const Core::Vector3 startPosY { center - halfWidth * y };
    const Core::Vector3 endPosY { center + halfWidth * y };
    Core::Vector3 currentPosX { startPosX };
    for ( uint i = 0; i < res + 1; ++i )
    {
        vertices.push_back( startPosY + currentPosX );
        vertices.push_back( endPosY + currentPosX );
        indices.push_back( uint( vertices.size() ) - 2 );
        indices.push_back( uint( vertices.size() ) - 1 );
        currentPosX += deltaPosX;
    }

    Core::Vector3 currentPosY = startPosY;
    for ( uint i = 0; i < res + 1; ++i )
    {
        vertices.push_back( startPosX + currentPosY );
        vertices.push_back( endPosX + currentPosY );
        indices.push_back( uint( vertices.size() ) - 2 );
        indices.push_back( uint( vertices.size() ) - 1 );
        currentPosY += deltaPosY;
    }

    Core::Vector4Array colors( vertices.size(), color );

    MeshPtr mesh( new Mesh( "GridPrimitive", Mesh::RM_LINES ) );
    mesh->loadGeometry( vertices, indices );
    mesh->getCoreGeometry().addAttrib( Mesh::getAttribName( Mesh::VERTEX_COLOR ), colors );

    return mesh;
}

MeshPtr AABB( const Core::Aabb& aabb, const Core::Utils::Color& color ) {
    Core::Vector3Array vertices( 8 );

    for ( uint i = 0; i < 8; ++i )
    { vertices[i] = aabb.corner( static_cast<Core::Aabb::CornerType>( i ) ); }

    std::vector<uint> indices = {
        0, 1, 1, 3, 3, 2, 2, 0, // Floor
        0, 4, 1, 5, 2, 6, 3, 7, // Links
        4, 5, 5, 7, 7, 6, 6, 4, // Ceil
    };

    Core::Vector4Array colors( vertices.size(), color );

    MeshPtr mesh( new Mesh( "AABB Primitive", Mesh::RM_LINES ) );
    mesh->loadGeometry( vertices, indices );
    mesh->getCoreGeometry().addAttrib( Mesh::getAttribName( Mesh::VERTEX_COLOR ), colors );

    return mesh;
}

MeshPtr OBB( const Obb& obb, const Core::Utils::Color& color ) {
    Core::Vector3Array vertices( 8 );

    for ( uint i = 0; i < 8; ++i )
    { vertices[i] = obb.worldCorner( i ); }

    std::vector<uint> indices = {
        0, 1, 1, 3, 3, 2, 2, 0, // Floor
        4, 5, 5, 7, 7, 6, 6, 4, // Ceil
        0, 4, 1, 5, 2, 6, 3, 7, // Links
    };

    Core::Vector4Array colors( vertices.size(), color );

    MeshPtr mesh( new Mesh( "OBB Primitive", Mesh::RM_LINES ) );
    mesh->loadGeometry( vertices, indices );
    mesh->getCoreGeometry().addAttrib( Mesh::getAttribName( Mesh::VERTEX_COLOR ), colors );

    return mesh;
}

MeshPtr Spline( const Core::Geometry::Spline<3, 3>& spline,
                uint pointCount,
                const Core::Utils::Color& color,
                Scalar /*scale*/ ) {
    Core::Vector3Array vertices;
    vertices.reserve( pointCount );

    std::vector<uint> indices;
    indices.reserve( pointCount * 2 - 2 );

    Scalar dt = Scalar( 1 ) / Scalar( pointCount - 1 );
    for ( uint i = 0; i < pointCount; ++i )
    {
        Scalar t = dt * i;
        vertices.push_back( spline.f( t ) );
    }

    for ( uint i = 0; i < pointCount - 1; ++i )
    {
        indices.push_back( i );
        indices.push_back( i + 1 );
    }

    Core::Vector4Array colors( vertices.size(), color );

    MeshPtr mesh( new Mesh( "Spline Primitive", Mesh::RM_LINES ) );
    mesh->loadGeometry( vertices, indices );
    mesh->getCoreGeometry().addAttrib( Mesh::getAttribName( Mesh::VERTEX_COLOR ), colors );

    return mesh;
}

MeshPtr LineStrip( const Core::Vector3Array& vertices, const Core::Vector4Array& colors ) {

    std::vector<uint> indices( vertices.size() );
    std::iota( indices.begin(), indices.end(), 0 );
    auto r = ( vertices.size() % 3 );
    if ( r != 0 )
    {
        for ( ; r < 3; ++r )
        { indices.push_back( vertices.size() - 1 ); }
    }
    MeshPtr mesh( new Mesh( "Line Strip Primitive", Mesh::RM_LINE_STRIP ) );
    mesh->loadGeometry( vertices, indices );
    if ( colors.size() > 0 )
    { mesh->getCoreGeometry().addAttrib( Mesh::getAttribName( Mesh::VERTEX_COLOR ), colors ); }
    return mesh;
}
} // namespace DrawPrimitives
} // namespace Data
} // namespace Engine
} // namespace Ra
