#include <Core/Containers/Grid.hpp>
#include <Core/Geometry/MeshPrimitives.hpp>
#include <Core/Math/Math.hpp> // areApproxEqual
#include <Core/Types.hpp>

#include <array>
#include <string>

namespace Ra {
namespace Core {
namespace Geometry {

TriangleMesh makeXNormalQuad( const Vector2& halfExts,
                              const Utils::optional<Utils::Color>& color ) {
    Transform T = Transform::Identity();
    T.linear().col( 0 ).swap( T.linear().col( 1 ) );
    T.linear().col( 1 ).swap( T.linear().col( 2 ) );
    return makePlaneGrid( 1, 1, halfExts, T, color );
}

TriangleMesh makeYNormalQuad( const Vector2& halfExts,
                              const Utils::optional<Utils::Color>& color ) {
    Transform T = Transform::Identity();
    T.linear().col( 1 ).swap( T.linear().col( 2 ) );
    T.linear().col( 0 ).swap( T.linear().col( 1 ) );
    return makePlaneGrid( 1, 1, halfExts, T, color );
}

TriangleMesh makeZNormalQuad( const Vector2& halfExts,
                              const Utils::optional<Utils::Color>& color ) {
    return makePlaneGrid( 1, 1, halfExts, Transform::Identity(), color );
}

TriangleMesh makeBox( const Vector3& halfExts, const Utils::optional<Utils::Color>& color ) {
    Aabb aabb( -halfExts, halfExts );
    return makeBox( aabb, color );
}

TriangleMesh makeBox( const Aabb& aabb, const Utils::optional<Utils::Color>& color ) {
    TriangleMesh result;
    result.setVertices( { aabb.corner( Aabb::BottomLeftFloor ),
                          aabb.corner( Aabb::BottomRightFloor ),
                          aabb.corner( Aabb::TopLeftFloor ),
                          aabb.corner( Aabb::TopRightFloor ),
                          aabb.corner( Aabb::BottomLeftCeil ),
                          aabb.corner( Aabb::BottomRightCeil ),
                          aabb.corner( Aabb::TopLeftCeil ),
                          aabb.corner( Aabb::TopRightCeil ) } );

    static const Scalar a = 1_ra / std::sqrt( 3_ra );

    result.setNormals( { Vector3( -a, -a, -a ),
                         Vector3( +a, -a, -a ),
                         Vector3( -a, +a, -a ),
                         Vector3( +a, +a, -a ),
                         Vector3( -a, -a, +a ),
                         Vector3( +a, -a, +a ),
                         Vector3( -a, +a, +a ),
                         Vector3( +a, +a, +a ) } );

    result.setIndices( {
        Vector3ui( 0, 2, 1 ),
        Vector3ui( 1, 2, 3 ), // Floor
        Vector3ui( 0, 1, 4 ),
        Vector3ui( 4, 1, 5 ), // Front
        Vector3ui( 3, 2, 6 ),
        Vector3ui( 3, 6, 7 ), // Back
        Vector3ui( 5, 1, 3 ),
        Vector3ui( 5, 3, 7 ), // Right
        Vector3ui( 2, 0, 4 ),
        Vector3ui( 2, 4, 6 ), // Left
        Vector3ui( 4, 5, 6 ),
        Vector3ui( 6, 5, 7 ) // Top
    } );

    if ( bool( color ) ) result.colorize( *color );
    result.checkConsistency();

    return result;
}

TriangleMesh makeSharpBox( const Vector3& halfExts, const Utils::optional<Utils::Color>& color ) {
    Aabb aabb( -halfExts, halfExts );
    return makeSharpBox( aabb, color );
}

TriangleMesh makeSharpBox( const Aabb& aabb, const Utils::optional<Utils::Color>& color ) {
    TriangleMesh result;
    result.setVertices( { // Floor Face
                          aabb.corner( Aabb::BottomLeftFloor ),
                          aabb.corner( Aabb::TopLeftFloor ),
                          aabb.corner( Aabb::TopRightFloor ),
                          aabb.corner( Aabb::BottomRightFloor ),

                          // Ceil Face
                          aabb.corner( Aabb::BottomLeftCeil ),
                          aabb.corner( Aabb::BottomRightCeil ),
                          aabb.corner( Aabb::TopRightCeil ),
                          aabb.corner( Aabb::TopLeftCeil ),

                          // Left Face
                          aabb.corner( Aabb::TopLeftFloor ),
                          aabb.corner( Aabb::BottomLeftFloor ),
                          aabb.corner( Aabb::BottomLeftCeil ),
                          aabb.corner( Aabb::TopLeftCeil ),

                          // Right Face
                          aabb.corner( Aabb::BottomRightFloor ),
                          aabb.corner( Aabb::TopRightFloor ),
                          aabb.corner( Aabb::TopRightCeil ),
                          aabb.corner( Aabb::BottomRightCeil ),

                          // Bottom Face
                          aabb.corner( Aabb::BottomLeftFloor ),
                          aabb.corner( Aabb::BottomRightFloor ),
                          aabb.corner( Aabb::BottomRightCeil ),
                          aabb.corner( Aabb::BottomLeftCeil ),

                          // Top face
                          aabb.corner( Aabb::TopLeftFloor ),
                          aabb.corner( Aabb::TopLeftCeil ),
                          aabb.corner( Aabb::TopRightCeil ),
                          aabb.corner( Aabb::TopRightFloor ) } );

    result.setNormals( { // Floor face
                         Vector3( 0, 0, -1 ),
                         Vector3( 0, 0, -1 ),
                         Vector3( 0, 0, -1 ),
                         Vector3( 0, 0, -1 ),
                         // Ceil Face
                         Vector3( 0, 0, +1 ),
                         Vector3( 0, 0, +1 ),
                         Vector3( 0, 0, +1 ),
                         Vector3( 0, 0, +1 ),
                         // Left Face
                         Vector3( -1, 0, 0 ),
                         Vector3( -1, 0, 0 ),
                         Vector3( -1, 0, 0 ),
                         Vector3( -1, 0, 0 ),
                         // Right Face
                         Vector3( +1, 0, 0 ),
                         Vector3( +1, 0, 0 ),
                         Vector3( +1, 0, 0 ),
                         Vector3( +1, 0, 0 ),
                         // Bottom Face
                         Vector3( 0, -1, 0 ),
                         Vector3( 0, -1, 0 ),
                         Vector3( 0, -1, 0 ),
                         Vector3( 0, -1, 0 ),
                         // Top Face
                         Vector3( 0, +1, 0 ),
                         Vector3( 0, +1, 0 ),
                         Vector3( 0, +1, 0 ),
                         Vector3( 0, +1, 0 ) } );

    result.setIndices( {

        Vector3ui( 0, 1, 2 ),
        Vector3ui( 0, 2, 3 ), // Floor
        Vector3ui( 4, 5, 6 ),
        Vector3ui( 4, 6, 7 ), // Ceil
        Vector3ui( 8, 9, 10 ),
        Vector3ui( 8, 10, 11 ), // Left
        Vector3ui( 12, 13, 14 ),
        Vector3ui( 12, 14, 15 ), // Right
        Vector3ui( 16, 17, 18 ),
        Vector3ui( 16, 18, 19 ), // Bottom
        Vector3ui( 20, 21, 22 ),
        Vector3ui( 20, 22, 23 ) // Top
    } );

    if ( bool( color ) ) result.colorize( *color );
    result.checkConsistency();

    return result;
}

TriangleMesh
makeGeodesicSphere( Scalar radius, uint numSubdiv, const Utils::optional<Utils::Color>& color ) {
    TriangleMesh result;
    uint faceCount = uint( std::pow( 4, numSubdiv ) ) * 20;

    TriangleMesh::PointAttribHandle::Container vertices;
    TriangleMesh::NormalAttribHandle::Container normals;
    TriangleMesh::IndexContainerType indices;
    vertices.reserve( faceCount - 8 );
    normals.reserve( faceCount - 8 );
    indices.reserve( faceCount );

    // First, make an icosahedron.
    // Top vertex
    vertices.emplace_back( 0, 0, radius );
    normals.emplace_back( 0, 0, 1 );

    const Scalar sq5_5 = radius * std::sqrt( 5_ra ) / 5_ra;

    // Middle vertices are on pentagons inscribed on a circle of radius 2*sqrt(5)
    for ( int i = 0; i < 5; ++i ) {
        for ( int j = 0; j < 2; ++j ) {
            const Scalar theta = ( Scalar( i ) + ( j * 0.5_ra ) ) * Math::PiMul2 / 5_ra;

            const Scalar x = 2_ra * sq5_5 * std::cos( theta );
            const Scalar y = 2_ra * sq5_5 * std::sin( theta );
            const Scalar z = j == 0 ? sq5_5 : -sq5_5;
            vertices.emplace_back( x, y, z );
            normals.push_back( vertices.back().normalized() );
        }
    }

    // Bottom vertex
    vertices.emplace_back( 0, 0, -radius );
    normals.emplace_back( 0, 0, -1 );

    for ( int i = 0; i < 5; ++i ) {
        uint i1 = ( i + 1 ) % 5;
        // Top triangles
        indices.emplace_back( 0, 2 * i + 1, ( 2 * i1 + 1 ) );

        // Bottom triangles
        indices.emplace_back( 2 * i + 2, 11, ( 2 * i1 + 2 ) );
    }
    for ( uint i = 0; i < 10; ++i ) {
        uint i1 = ( i + 0 ) % 10 + 1;
        uint i2 = ( i + 1 ) % 10 + 1;
        uint i3 = ( i + 2 ) % 10 + 1;
        ( i % 2 ) ? indices.emplace_back( i3, i2, i1 ) : indices.emplace_back( i2, i3, i1 );
    }

    for ( uint n = 0; n < numSubdiv; ++n ) {
        VectorArray<Vector3ui> newTris; //= indices;
        // Now subdivide every face into 4 triangles.
        for ( uint i = 0; i < indices.size(); ++i ) {
            const Vector3ui& tri               = indices[i];
            std::array<Vector3, 3> triVertices = {
                vertices[tri[0]], vertices[tri[1]], vertices[tri[2]] };
            std::array<uint, 3> middles;

            for ( uint v = 0; v < 3; ++v ) {
                middles[v] = uint( vertices.size() );

                Vector3 vertex = 0.5_ra * ( triVertices[v] + triVertices[( v + 1 ) % 3] );
                vertex.normalize();

                vertices.push_back( radius * vertex );
                normals.push_back( vertex );
            }

            newTris.emplace_back( tri[0], middles[0], middles[2] );
            newTris.emplace_back( middles[0], tri[1], middles[1] );
            newTris.emplace_back( middles[2], middles[1], tri[2] );
            newTris.emplace_back( middles[0], middles[1], middles[2] );
        }
        indices = newTris;
    }

    result.setVertices( std::move( vertices ) );
    result.setNormals( std::move( normals ) );
    result.setIndices( std::move( indices ) );
    if ( bool( color ) ) result.colorize( *color );
    result.checkConsistency();

    return result;
}

TriangleMesh makeCylinder( const Vector3& a,
                           const Vector3& b,
                           Scalar radius,
                           uint nFaces,
                           const Utils::optional<Utils::Color>& color ) {
    TriangleMesh result;

    TriangleMesh::PointAttribHandle::Container vertices;
    TriangleMesh::NormalAttribHandle::Container normals;
    TriangleMesh::IndexContainerType indices;

    vertices.reserve( 2 + 3 * nFaces );
    normals.reserve( 2 + 3 * nFaces );
    indices.reserve( 6 * nFaces );

    Vector3 ab  = b - a;
    Vector3 dir = ab.normalized();

    //  Create two circles normal centered on A and B and normal to ab (use dir, since first vector
    //  must be normalized)
    Vector3 xPlane, yPlane;
    Math::getOrthogonalVectors( dir, xPlane, yPlane );
    xPlane.normalize();
    yPlane.normalize();

    Vector3 c = 0.5 * ( a + b );

    vertices.push_back( a );
    vertices.push_back( b );
    normals.push_back( -dir );
    normals.push_back( dir );

    const Scalar thetaInc( Core::Math::PiMul2 / Scalar( nFaces ) );
    for ( uint i = 0; i < nFaces; ++i ) {
        const Scalar theta = i * thetaInc;
        Vector3 normal     = std::cos( theta ) * xPlane + std::sin( theta ) * yPlane;

        // Even indices are A circle and odd indices are B circle.
        vertices.push_back( a + radius * normal );
        vertices.push_back( c + radius * normal );
        vertices.push_back( b + radius * normal );

        normals.push_back( ( normal - dir ).normalized() );
        normals.push_back( normal.normalized() );
        normals.push_back( ( normal + dir ).normalized() );
    }

    for ( uint i = 0; i < nFaces; ++i ) {
        uint bl = 3 * i + 2;                      // bottom left corner of face
        uint br = 3 * ( ( i + 1 ) % nFaces ) + 2; // bottom right corner of face
        uint ml = bl + 1;                         // mid left
        uint mr = br + 1;                         // mid right
        uint tl = ml + 1;                         // top left
        uint tr = mr + 1;                         // top right

        indices.emplace_back( bl, br, ml );
        indices.emplace_back( br, mr, ml );

        indices.emplace_back( ml, mr, tl );
        indices.emplace_back( mr, tr, tl );

        indices.emplace_back( 0, br, bl );
        indices.emplace_back( 1, tl, tr );
    }

    result.setVertices( std::move( vertices ) );
    result.setNormals( std::move( normals ) );
    result.setIndices( std::move( indices ) );
    if ( bool( color ) ) result.colorize( *color );
    result.checkConsistency();

    return result;
}

TriangleMesh makeCapsule( Scalar length,
                          Scalar radius,
                          uint nFaces,
                          const Utils::optional<Utils::Color>& color ) {
    TriangleMesh result;

    TriangleMesh::PointAttribHandle::Container vertices;
    TriangleMesh::NormalAttribHandle::Container normals;
    TriangleMesh::IndexContainerType indices;

    vertices.reserve( nFaces * nFaces + nFaces + 2 );
    normals.reserve( nFaces * nFaces + nFaces + 2 );
    indices.reserve( 2 * ( nFaces * nFaces + nFaces ) );

    const Scalar l = length / 2_ra;

    // We create a capsule by joining a cylinder with two half spheres.

    // Part 1 : create the cylinder based on 3 circles
    // Cylinder vertices.
    const Scalar thetaInc( Core::Math::PiMul2 / Scalar( nFaces ) );
    for ( uint i = 0; i < nFaces; ++i ) {
        const Scalar theta = i * thetaInc;
        Vector3 normal( std::cos( theta ), std::sin( theta ), 0 );

        // Create 3 circles
        Vector3 vertex = radius * normal;
        vertices.emplace_back( vertex[0], vertex[1], -l );
        vertices.emplace_back( vertex[0], vertex[1], 0.0 );
        vertices.emplace_back( vertex[0], vertex[1], l );

        normals.push_back( normal );
        normals.push_back( normal );
        normals.push_back( normal );
    }

    // Cylinder side faces
    for ( uint i = 0; i < nFaces; ++i ) {
        uint bl = 3 * i;                      // bottom left corner of face
        uint br = 3 * ( ( i + 1 ) % nFaces ); // bottom right corner of face
        uint ml = bl + 1;                     // mid left
        uint mr = br + 1;                     // mid right
        uint tl = ml + 1;                     // top left
        uint tr = mr + 1;                     // top right

        indices.emplace_back( bl, br, ml );
        indices.emplace_back( br, mr, ml );

        indices.emplace_back( ml, mr, tl );
        indices.emplace_back( mr, tr, tl );
    }

    // Part 2 : create the bottom hemisphere.
    const Scalar phiInc = Core::Math::Pi / Scalar( nFaces );
    uint vert_count     = uint( vertices.size() );

    // Bottom hemisphere vertices
    for ( uint j = 1; j <= nFaces / 2 - 1; ++j ) {
        const Scalar phi = Core::Math::PiDiv2 + j * phiInc;

        for ( uint i = 0; i < nFaces; ++i ) {
            const Scalar theta = i * thetaInc;

            const Vector3 normal( std::cos( theta ) * std::sin( phi ),
                                  std::sin( theta ) * std::sin( phi ),
                                  std::cos( phi ) );

            Vector3 vertex = radius * normal;
            vertex[2] -= l;

            vertices.push_back( vertex );
            normals.push_back( normal );
        }
    }
    // Add bottom point (south pole)
    vertices.emplace_back( 0, 0, -( l + radius ) );
    normals.emplace_back( 0, 0, -1 );

    // First ring of sphere triangles (joining with the cylinder)
    for ( uint i = 0; i < nFaces; ++i ) {
        uint bl = 3 * i;
        uint br = 3 * ( ( i + 1 ) % nFaces );

        uint tl = vert_count + i;
        uint tr = vert_count + ( i + 1 ) % nFaces;

        indices.emplace_back( br, bl, tl );
        indices.emplace_back( br, tl, tr );
    }

    // Next rings of the sphere triangles
    for ( uint j = 0; j < ( nFaces / 2 ) - 2; ++j ) {
        for ( uint i = 0; i < nFaces; ++i ) {
            uint bl = vert_count + j * nFaces + i;
            uint br = vert_count + j * nFaces + ( i + 1 ) % nFaces;

            uint tl = vert_count + ( j + 1 ) * nFaces + i;
            uint tr = vert_count + ( j + 1 ) * nFaces + ( i + 1 ) % nFaces;

            indices.emplace_back( br, bl, tl );
            indices.emplace_back( br, tl, tr );
        }
    }

    // End cap triangles, joining with the pole
    for ( uint i = 0; i < nFaces; ++i ) {
        const uint j = nFaces / 2 - 2;
        uint bl      = vert_count + j * nFaces + i;
        uint br      = vert_count + j * nFaces + ( i + 1 ) % nFaces;
        uint bot     = uint( vertices.size() - 1 );
        indices.emplace_back( br, bl, bot );
    }

    // Part 3 : create the top hemisphere
    vert_count = uint( vertices.size() );

    // Top hemisphere vertices
    for ( uint j = 1; j <= nFaces / 2 - 1; ++j ) {
        const Scalar phi = Core::Math::PiDiv2 - j * phiInc;

        for ( uint i = 0; i < nFaces; ++i ) {
            const Scalar theta = i * thetaInc;

            const Vector3 normal( std::cos( theta ) * std::sin( phi ),
                                  std::sin( theta ) * std::sin( phi ),
                                  std::cos( phi ) );

            Vector3 vertex = radius * normal;
            vertex[2] += l;

            vertices.push_back( vertex );
            normals.push_back( normal );
        }
    }

    // Add top point (north pole)
    vertices.emplace_back( 0, 0, ( l + radius ) );
    normals.emplace_back( 0, 0, 1 );

    // First ring of sphere triangles (joining with the cylinder)
    for ( uint i = 0; i < nFaces; ++i ) {
        uint bl = 3 * i + 2;
        uint br = 3 * ( ( i + 1 ) % nFaces ) + 2;

        uint tl = vert_count + i;
        uint tr = vert_count + ( i + 1 ) % nFaces;

        indices.emplace_back( bl, br, tl );
        indices.emplace_back( br, tr, tl );
    }

    // Next rigns of the sphere triangles
    for ( uint j = 0; j < ( nFaces / 2 ) - 2; ++j ) {
        for ( uint i = 0; i < nFaces; ++i ) {
            uint bl = vert_count + j * nFaces + i;
            uint br = vert_count + j * nFaces + ( i + 1 ) % nFaces;

            uint tl = vert_count + ( j + 1 ) * nFaces + i;
            uint tr = vert_count + ( j + 1 ) * nFaces + ( i + 1 ) % nFaces;

            indices.emplace_back( bl, br, tl );
            indices.emplace_back( br, tr, tl );
        }
    }

    // End cap triangles, joining with the pole
    for ( uint i = 0; i < nFaces; ++i ) {
        const uint j = nFaces / 2 - 2;
        uint bl      = vert_count + j * nFaces + i;
        uint br      = vert_count + j * nFaces + ( i + 1 ) % nFaces;
        uint top     = uint( vertices.size() ) - 1;
        indices.emplace_back( bl, br, top );
    }
    result.setVertices( std::move( vertices ) );
    result.setNormals( std::move( normals ) );
    result.setIndices( std::move( indices ) );
    if ( bool( color ) ) result.colorize( *color );
    result.checkConsistency();

    return result;
}

TriangleMesh makeTube( const Vector3& a,
                       const Vector3& b,
                       Scalar outerRadius,
                       Scalar innerRadius,
                       uint nFaces,
                       const Utils::optional<Utils::Color>& color ) {

    CORE_ASSERT( outerRadius > innerRadius, "Outer radius must be bigger than inner." );

    TriangleMesh result;
    TriangleMesh::PointAttribHandle::Container vertices;
    TriangleMesh::NormalAttribHandle::Container normals;
    TriangleMesh::IndexContainerType indices;
    vertices.reserve( 6 * nFaces );
    normals.reserve( 6 * nFaces );
    indices.reserve( 12 * nFaces );

    Vector3 ab  = b - a;
    Vector3 dir = ab.normalized();

    //  Create two circles normal centered on A and B and normal to ab;
    Vector3 xPlane, yPlane;
    Math::getOrthogonalVectors( dir, xPlane, yPlane );
    xPlane.normalize();
    yPlane.normalize();

    Vector3 c = 0.5 * ( a + b );

    const Scalar thetaInc( Core::Math::PiMul2 / Scalar( nFaces ) );
    for ( uint i = 0; i < nFaces; ++i ) {
        const Scalar theta = i * thetaInc;

        Vector3 normal = std::cos( theta ) * xPlane + std::sin( theta ) * yPlane;

        vertices.push_back( a + outerRadius * normal );
        vertices.push_back( c + outerRadius * normal );
        vertices.push_back( b + outerRadius * normal );

        vertices.push_back( a + innerRadius * normal );
        vertices.push_back( c + innerRadius * normal );
        vertices.push_back( b + innerRadius * normal );

        normals.push_back( ( -dir + normal ).normalized() );
        normals.push_back( normal );
        normals.push_back( ( dir + normal ).normalized() );

        normals.push_back( ( -dir - normal ).normalized() );
        normals.push_back( -normal );
        normals.push_back( ( dir - normal ).normalized() );
    }

    for ( uint i = 0; i < nFaces; ++i ) {
        // Outer face.
        uint obl = 6 * i;                      // bottom left corner of outer face
        uint obr = 6 * ( ( i + 1 ) % nFaces ); // bottom right corner of outer face
        uint oml = obl + 1;                    // mid left
        uint omr = obr + 1;                    // mid right
        uint otl = oml + 1;                    // top left
        uint otr = omr + 1;                    // top right

        // Inner face
        uint ibl = 6 * i + 3;                      // bottom left corner of inner face
        uint ibr = 6 * ( ( i + 1 ) % nFaces ) + 3; // bottom right corner of inner face
        uint iml = ibl + 1;                        // mid left
        uint imr = ibr + 1;                        // mid right
        uint itl = iml + 1;                        // top left
        uint itr = imr + 1;                        // top right

        // Outer face triangles, just like a regular cylinder.

        indices.emplace_back( obl, obr, oml );
        indices.emplace_back( obr, omr, oml );

        indices.emplace_back( oml, omr, otl );
        indices.emplace_back( omr, otr, otl );

        // Inner face triangles (note how order is reversed because inner face points inwards).

        indices.emplace_back( ibr, ibl, iml );
        indices.emplace_back( ibr, iml, imr );

        indices.emplace_back( imr, iml, itl );
        indices.emplace_back( imr, itl, itr );

        // Bottom face quad
        indices.emplace_back( ibr, obr, ibl );
        indices.emplace_back( obl, ibl, obr );

        // Top face quad
        indices.emplace_back( otr, itr, itl );
        indices.emplace_back( itl, otl, otr );
    }
    result.setVertices( std::move( vertices ) );
    result.setNormals( std::move( normals ) );
    result.setIndices( std::move( indices ) );
    if ( bool( color ) ) result.colorize( *color );
    result.checkConsistency();

    return result;
}

TriangleMesh makeCone( const Vector3& base,
                       const Vector3& tip,
                       Scalar radius,
                       uint nFaces,
                       const Utils::optional<Utils::Color>& color ) {
    TriangleMesh result;
    TriangleMesh::PointAttribHandle::Container vertices;
    TriangleMesh::NormalAttribHandle::Container normals;
    TriangleMesh::IndexContainerType indices;
    vertices.reserve( 2 + nFaces );
    normals.reserve( 2 + nFaces );
    indices.reserve( 2 * nFaces );

    Vector3 ab  = tip - base;
    Vector3 dir = ab.normalized();

    //  Create two circles normal centered on A and B and normal to ab;
    Vector3 xPlane, yPlane;
    Math::getOrthogonalVectors( dir, xPlane, yPlane );
    xPlane.normalize();
    yPlane.normalize();

    vertices.push_back( base );
    vertices.push_back( tip );
    normals.push_back( -dir );
    normals.push_back( dir );

    const Scalar thetaInc( Core::Math::PiMul2 / Scalar( nFaces ) );
    for ( uint i = 0; i < nFaces; ++i ) {
        const Scalar theta = i * thetaInc;
        Vector3 normal     = std::cos( theta ) * xPlane + std::sin( theta ) * yPlane;

        vertices.push_back( base + radius * normal );
        normals.push_back( ( normal - dir ).normalized() );
    }

    for ( uint i = 0; i < nFaces; ++i ) {
        uint bl = i + 2;                      // bottom left corner of face
        uint br = ( ( i + 1 ) % nFaces ) + 2; // bottom right corner of face

        indices.emplace_back( 0, br, bl );
        indices.emplace_back( 1, bl, br );
    }
    result.setVertices( std::move( vertices ) );
    result.setNormals( std::move( normals ) );
    result.setIndices( std::move( indices ) );
    if ( bool( color ) ) result.colorize( *color );
    result.checkConsistency();

    return result;
}

TriangleMesh makePlaneGrid( const uint rows,
                            const uint cols,
                            const Vector2& halfExts,
                            const Transform& T,
                            const Utils::optional<Utils::Color>& color ) {
    TriangleMesh result;
    TriangleMesh::PointAttribHandle::Container vertices;
    TriangleMesh::NormalAttribHandle::Container normals;
    TriangleMesh::IndexContainerType indices;

    const uint R      = ( rows + 1 );
    const uint C      = ( cols + 1 );
    const uint v_size = C * R;
    const uint t_size = 2 * cols * rows;

    vertices.resize( v_size );
    normals.resize( v_size );
    indices.reserve( t_size );

    const Vector3 X = T.linear().col( 0 ).normalized();
    const Vector3 Y = T.linear().col( 1 ).normalized();
    const Vector3 Z = T.linear().col( 2 ).normalized();

    const Vector3 x = ( 2_ra * halfExts[0] * X ) / Scalar( cols );
    const Vector3 y = ( 2_ra * halfExts[1] * Y ) / Scalar( rows );
    const Vector3 o = T.translation() - ( halfExts[0] * X ) - ( halfExts[1] * Y );

    Grid<uint, 2> v( { R, C } );
    for ( uint i = 0; i < R; ++i ) {
        for ( uint j = 0; j < C; ++j ) {
            const uint id    = ( i * C ) + j;
            v.at( { i, j } ) = id;
            vertices[id]     = o + ( i * y ) + ( j * x );
            normals[id]      = Z;
        }
    }

    for ( uint i = 0; i < rows; ++i ) {
        for ( uint j = 0; j < cols; ++j ) {
            indices.emplace_back(
                v.at( { i, j } ), v.at( { i, j + 1 } ), v.at( { i + 1, j + 1 } ) );
            indices.emplace_back(
                v.at( { i, j } ), v.at( { i + 1, j + 1 } ), v.at( { i + 1, j } ) );
        }
    }
    result.setVertices( std::move( vertices ) );
    result.setNormals( std::move( normals ) );
    result.setIndices( std::move( indices ) );
    if ( bool( color ) ) result.colorize( *color );
    result.checkConsistency();

    return result;
}
} // namespace Geometry
} // namespace Core
} // namespace Ra
