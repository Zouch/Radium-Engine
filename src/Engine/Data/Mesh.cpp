#include <Engine/Data/Mesh.hpp>

#include <numeric>

#include <Core/Utils/Attribs.hpp>
#include <Core/Utils/Log.hpp>
#include <Engine/Data/ShaderProgram.hpp>
#include <Engine/OpenGL.hpp>

#include <globjects/Buffer.h>
#include <globjects/VertexArray.h>

namespace Ra {
namespace Engine {
namespace Data {
using namespace Ra::Core::Utils;

// Dirty is initializes as false so that we do not create the vao while
// we have no data to send to the gpu.
AttribArrayDisplayable::AttribArrayDisplayable( const std::string& name,
                                                MeshRenderMode renderMode ) :
    Displayable( name ), m_renderMode { renderMode } {
    CORE_ASSERT( m_renderMode == RM_POINTS || m_renderMode == RM_LINES ||
                     m_renderMode == RM_LINE_LOOP || m_renderMode == RM_LINE_STRIP ||
                     m_renderMode == RM_TRIANGLES || m_renderMode == RM_TRIANGLE_STRIP ||
                     m_renderMode == RM_TRIANGLE_FAN || m_renderMode == RM_LINES_ADJACENCY ||
                     m_renderMode == RM_LINE_STRIP_ADJACENCY,
                 "Unsupported render mode" );

    updatePickingRenderMode();
}

size_t Mesh::getNumFaces() const {
    ///\todo fix this once we have explicit triangle fan and strip management.
    switch ( getRenderMode() )
    {
    case MeshRenderMode::RM_TRIANGLE_STRIP:
        [[fallthrough]];
    case MeshRenderMode::RM_TRIANGLE_FAN:
        return ( getCoreGeometry().getIndices().size() - 1 ) * 3 + 1;
    case MeshRenderMode::RM_TRIANGLES:
        return getCoreGeometry().getIndices().size();
    default:
        return size_t( 0 );
    }
}

void Mesh::loadGeometry( const Core::Vector3Array& vertices, const std::vector<uint>& indices ) {
    // Do not remove this function to force everyone to use TriangleMesh.
    //  ... because we have some line meshes as well...
    /// \todo find an alternative
    Core::Geometry::TriangleMesh mesh;

    auto nIdx = indices.size();

    if ( indices.empty() )
    {
        m_numElements = vertices.size();
        setRenderMode( RM_POINTS );
    }
    else
        m_numElements = nIdx;
    mesh.setVertices( vertices );

    // Check that when loading a TriangleMesh we actually have triangles or lines.
    CORE_ASSERT( m_renderMode != GL_TRIANGLES || nIdx % 3 == 0,
                 "There should be 3 indices per triangle " );
    CORE_ASSERT( m_renderMode != GL_LINES || nIdx % 2 == 0, "There should be 2 indices per line" );
    CORE_ASSERT( m_renderMode != GL_LINES_ADJACENCY || nIdx % 4 == 0,
                 "There should be 4 indices per line adjacency" );
    Core::Geometry::TriangleMesh::IndexContainerType mindices;
    for ( uint i = 0; i < indices.size(); i = i + 3 )
    {
        // We store all indices in order. This means that for lines we have
        // (L00, L01, L10), (L11, L20, L21) etc. We fill the missing by wrapping around indices.
        mindices.push_back( { indices[i], indices[( i + 1 ) % nIdx], indices[( i + 2 ) % nIdx] } );
    }

    mesh.setIndices( std::move( mindices ) );
    m_dataDirty.clear();
    m_vbos.clear();

    ///\todo check line vs triangle here is a bug
    loadGeometry( std::move( mesh ) );
}

void AttribArrayDisplayable::updatePickingRenderMode() {
    switch ( getRenderMode() )
    {
    case AttribArrayDisplayable::RM_POINTS: {
        Displayable::m_pickingRenderMode = PKM_POINTS;
        break;
    }
    case AttribArrayDisplayable::RM_LINES: // fall through
        [[fallthrough]];
    case AttribArrayDisplayable::RM_LINE_LOOP: // fall through
        [[fallthrough]];
    case AttribArrayDisplayable::RM_LINE_STRIP: {
        Displayable::m_pickingRenderMode = PKM_LINES;
        break;
    }
    case AttribArrayDisplayable::RM_LINES_ADJACENCY: // fall through
        [[fallthrough]];
    case AttribArrayDisplayable::RM_LINE_STRIP_ADJACENCY: {
        Displayable::m_pickingRenderMode = PKM_LINE_ADJ;
        break;
    }
    case AttribArrayDisplayable::RM_TRIANGLES:
        [[fallthrough]];
    case AttribArrayDisplayable::RM_TRIANGLE_STRIP:
        [[fallthrough]];
    case AttribArrayDisplayable::RM_TRIANGLE_FAN: {
        Displayable::m_pickingRenderMode = PKM_TRI;
        break;
    }
    default: {
        Displayable::m_pickingRenderMode = NO_PICKING;
        break;
    }
    }
}

void AttribArrayDisplayable::setDirty( const std::string& name ) {
    auto itr = m_handleToBuffer.find( name );
    if ( itr == m_handleToBuffer.end() )
    {
        m_handleToBuffer[name] = m_dataDirty.size();
        m_dataDirty.push_back( true );
        m_vbos.emplace_back( nullptr );
    }
    else
        m_dataDirty[itr->second] = true;

    m_isDirty = true;
}

void AttribArrayDisplayable::setDirty( unsigned int index ) {
    if ( index < m_dataDirty.size() )
    {
        m_dataDirty[index] = true;
        m_isDirty          = true;
    }
}

void AttribArrayDisplayable::setDirty( const AttribArrayDisplayable::MeshData& type ) {
    auto name = getAttribName( type );
    auto itr  = m_handleToBuffer.find( name );
    if ( itr == m_handleToBuffer.end() )
    {
        m_handleToBuffer[name] = m_dataDirty.size();
        m_dataDirty.push_back( true );
        m_vbos.emplace_back( nullptr );
    }
    else
        m_dataDirty[itr->second] = true;

    m_isDirty = true;
}

void PointCloud::render( const ShaderProgram* prog ) {
    if ( m_vao )
    {
        autoVertexAttribPointer( prog );
        m_vao->bind();
        m_vao->drawArrays(
            static_cast<GLenum>( m_renderMode ), 0, GLsizei( m_mesh.vertices().size() ) );
        m_vao->unbind();
    }
}

void PointCloud::loadGeometry( Core::Geometry::PointCloud&& mesh ) {
    loadGeometry_common( std::move( mesh ) );
}

void PointCloud::updateGL_specific_impl() {
    if ( !m_vao ) { m_vao = globjects::VertexArray::create(); }
}

} // namespace Data
} // namespace Engine
} // namespace Ra
