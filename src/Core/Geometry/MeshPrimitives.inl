#pragma once
#include <Core/Geometry/MeshPrimitives.hpp>
#include <Core/Math/LinearAlgebra.hpp> // Math::pi

namespace Ra {
namespace Core {
namespace Geometry {

template <uint U, uint V>
TriangleMesh makeParametricSphere( Scalar radius, const Utils::optional<Utils::Color>& color ) {
    constexpr uint slices = U;
    constexpr uint stacks = V;

    TriangleMesh result;

    TriangleMesh::PointAttribHandle::Container vertices;
    TriangleMesh::NormalAttribHandle::Container normals;
    TriangleMesh::IndexContainerType indices;

    vertices.reserve( 2 + slices * ( stacks - 1 ) );
    normals.reserve( 2 + slices * ( stacks - 1 ) );
    indices.reserve( 2 * slices * ( stacks - 1 ) );

    for ( uint u = 0; u < slices; ++u ) {
        const Scalar theta = Scalar( 2 * u ) * Core::Math::Pi / Scalar( slices );
        for ( uint v = 1; v < stacks; ++v ) {
            // Regular vertices on the sphere.
            const Scalar phi = Scalar( v ) * Core::Math::Pi / Scalar( stacks );
            vertices.push_back( Vector3( radius * std::cos( theta ) * std::sin( phi ),
                                         radius * std::sin( theta ) * std::sin( phi ),
                                         radius * std::cos( phi ) ) );
            normals.push_back( vertices.back().normalized() );

            // Regular triangles
            if ( v > 1 ) {
                const uint nextSlice = ( ( u + 1 ) % slices ) * ( stacks - 1 );
                const uint baseSlice = u * ( stacks - 1 );
                indices.push_back(
                    Vector3ui( baseSlice + v - 2, baseSlice + v - 1, nextSlice + v - 1 ) );
                indices.push_back(
                    Vector3ui( baseSlice + v - 2, nextSlice + v - 1, nextSlice + v - 2 ) );
            }
        }
    }

    // Add the pole vertices.
    uint northPoleIdx = vertices.size();
    vertices.push_back( Vector3( 0, 0, radius ) );
    normals.push_back( Vector3( 0, 0, 1 ) );
    uint southPoleIdx = vertices.size();
    vertices.push_back( Vector3( 0, 0, -radius ) );
    normals.push_back( Vector3( 0, 0, -1 ) );

    result.setVertices( std::move( vertices ) );
    result.setNormals( std::move( normals ) );

    // Add the polar caps triangles.
    for ( uint u = 0; u < slices; ++u ) {
        const uint nextSlice = ( ( u + 1 ) % slices ) * ( stacks - 1 );
        const uint baseSlice = u * ( stacks - 1 );
        indices.push_back( Vector3ui( northPoleIdx, baseSlice, nextSlice ) );
        indices.push_back(
            Vector3ui( southPoleIdx, nextSlice + stacks - 2, baseSlice + stacks - 2 ) );
    }

    result.setIndices( std::move( indices ) );
    if ( bool( color ) ) result.colorize( *color );
    result.checkConsistency();

    return result;
}

template <uint U, uint V>
TriangleMesh makeParametricTorus( Scalar majorRadius,
                                  Scalar minorRadius,
                                  const Utils::optional<Utils::Color>& color ) {
    TriangleMesh result;
    TriangleMesh::PointAttribHandle::Container vertices;
    TriangleMesh::NormalAttribHandle::Container normals;
    TriangleMesh::IndexContainerType indices;

    vertices.reserve( U * V );
    normals.reserve( V * V );
    indices.reserve( 2 * U * V );

    for ( uint iu = 0; iu < U; ++iu ) {
        Scalar u = Scalar( iu ) * Core::Math::PiMul2 / Scalar( U );
        Core::Vector3 circleCenter( majorRadius * std::cos( u ), majorRadius * std::sin( u ), 0.f );

        for ( uint iv = 0; iv < V; ++iv ) {
            Scalar v = Scalar( iv ) * Core::Math::PiMul2 / Scalar( V );

            Core::Vector3 vertex( ( majorRadius + minorRadius * std::cos( v ) ) * std::cos( u ),
                                  ( majorRadius + minorRadius * std::cos( v ) ) * std::sin( u ),
                                  minorRadius * std::sin( v ) );

            vertices.push_back( vertex );
            normals.push_back( ( vertex - circleCenter ).normalized() );

            indices.push_back( Vector3ui(
                iu * V + iv, ( ( iu + 1 ) % U ) * V + iv, iu * V + ( ( iv + 1 ) % V ) ) );
            indices.push_back( Vector3ui( ( ( iu + 1 ) % U ) * V + iv,
                                          ( ( iu + 1 ) % U ) * V + ( ( iv + 1 ) % V ),
                                          iu * V + ( ( iv + 1 ) % V ) ) );
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
