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
////////////////  MultiIndexedGeometry  ///////////////////////////////

GeometryDisplayable::GeometryDisplayable( const std::string& name,
                                          typename Core::Geometry::MultiIndexedGeometry&& geom,
                                          typename base::MeshRenderMode renderMode ) :
    base( name, renderMode ), m_geom( std::move( geom ) ) {}

GeometryDisplayable::~GeometryDisplayable() {}

void GeometryDisplayable::loadGeometry( Core::Geometry::MultiIndexedGeometry&& mesh ) {
    m_geomLayers.clear();
    m_geom = std::move( mesh );
    setupCoreMeshObservers();
}

void GeometryDisplayable::setupCoreMeshObservers() {
    int idx = 0;
    m_dataDirty.resize( m_geom.vertexAttribs().getNumAttribs() );
    m_vbos.resize( m_geom.vertexAttribs().getNumAttribs() );
    // here capture ref to idx to propagate idx incrementation
    m_geom.vertexAttribs().for_each_attrib( [&idx, this]( Ra::Core::Utils::AttribBase* b ) {
        auto name              = b->getName();
        m_handleToBuffer[name] = idx;
        m_dataDirty[idx]       = true;

        // create a identity translation if name is not already translated.
        addToTranslationTable( name );

        b->attach( AttribObserver( this, idx ) );
        ++idx;
    } );

    // add an observer on attrib manipulation.
    m_geom.vertexAttribs().attachMember( this, &GeometryDisplayable::addAttribObserver );
    m_isDirty = true;
}

void GeometryDisplayable::addToTranslationTable( const std::string& name ) {
    auto it = m_translationTableMeshToShader.find( name );
    if ( it == m_translationTableMeshToShader.end() )
    {
        m_translationTableMeshToShader[name] = name;
        m_translationTableShaderToMesh[name] = name;
    }
}

void GeometryDisplayable::addAttribObserver( const std::string& name ) {
    // this observer is called each time an attrib is added or removed from m_mesh
    auto attrib = m_geom.getAttribBase( name );
    // if attrib not nullptr, then it's an attrib add, so attach an observer to it

    if ( attrib )
    {
        auto itr = m_handleToBuffer.find( name );
        if ( itr == m_handleToBuffer.end() )
        {
            m_handleToBuffer[name] = m_dataDirty.size();

            addToTranslationTable( name );

            m_dataDirty.push_back( true );
            m_vbos.emplace_back( nullptr );
        }
        auto idx = m_handleToBuffer[name];
        attrib->attach( AttribObserver( this, idx ) );
    }
    // else it's an attrib remove, do nothing, cleanup will be done in updateGL()
    else
    {}
}

void GeometryDisplayable::setAttribNameCorrespondence( const std::string& meshAttribName,
                                                       const std::string& shaderAttribName ) {

    // clean previously set translation

    auto it1 = std::find_if( m_translationTableShaderToMesh.begin(),
                             m_translationTableShaderToMesh.end(),
                             [&meshAttribName]( const TranslationTable::value_type& p ) {
                                 return p.second == meshAttribName;
                             } );

    if ( it1 != m_translationTableShaderToMesh.end() ) m_translationTableShaderToMesh.erase( it1 );

    auto it2 = std::find_if( m_translationTableMeshToShader.begin(),
                             m_translationTableMeshToShader.end(),
                             [&shaderAttribName]( const TranslationTable::value_type& p ) {
                                 return p.second == shaderAttribName;
                             } );

    if ( it2 != m_translationTableMeshToShader.end() ) m_translationTableMeshToShader.erase( it2 );

    m_translationTableShaderToMesh[shaderAttribName] = meshAttribName;
    m_translationTableMeshToShader[meshAttribName]   = shaderAttribName;
}

bool GeometryDisplayable::addRenderLayer( LayerKeyType key ) {
    if ( !m_geom.containsLayer( key ) ) return false;
    auto it = m_geomLayers.find( key );
    if ( it == m_geomLayers.end() ) return false;

    VaoIndices* vao = new VaoIndices;
    auto& geomLayer = m_geom.getLayerWithLock( key );
    int observerId  = geomLayer.attach( VaoIndices::IndicesObserver( vao ) );
    m_geom.unlockLayer( key );

    m_geomLayers[key] = { observerId, vao };

    return false;
}
bool GeometryDisplayable::removeRenderLayer( LayerKeyType key ) {
    auto it = m_geomLayers.find( key );
    if ( it == m_geomLayers.end() ) return false;

    // the layer might have already been deleted
    if ( m_geom.containsLayer( key ) )
    {
        auto& geomLayer = m_geom.getLayerWithLock( key );
        geomLayer.detach( it->second.first );
        m_geom.unlockLayer( key );
    }
    delete ( it->second.second );
    m_geomLayers.erase( it );

    return true;
}

void GeometryDisplayable::updateGL_specific_impl() {
    CORE_ASSERT( false, "not implemented yet" );
    //    if ( !m_indices )
    //    {
    //        m_indices      = globjects::Buffer::create();
    //        m_indicesDirty = true;
    //    }
    //    if ( m_indicesDirty )
    //    {
    //        /// this one do not work since m_indices is not a std::vector
    //        // m_indices->setData( m_mesh.m_indices, GL_DYNAMIC_DRAW );
    //        m_numElements =
    //            base::m_mesh.getIndices().size() *
    //            base::CoreGeometry::IndexType::RowsAtCompileTime;
    //
    //        m_indices->setData(
    //            static_cast<gl::GLsizeiptr>( base::m_mesh.getIndices().size() *
    //                                         sizeof( typename base::CoreGeometry::IndexType ) ),
    //            base::m_mesh.getIndices().data(),
    //            GL_STATIC_DRAW );
    //        m_indicesDirty = false;
    //    }
    //    if ( !base::m_vao ) { base::m_vao = globjects::VertexArray::create(); }
    //    base::m_vao->bind();
    //    base::m_vao->bindElementBuffer( m_indices.get() );
    //    base::m_vao->unbind();
    /// \todo implement !
}

void GeometryDisplayable::render( const ShaderProgram* ) {
    CORE_ASSERT( false, "not implemented yet" );
    //    if ( base::m_vao )
    //    {
    //        GL_CHECK_ERROR;
    //        base::m_vao->bind();
    //        base::autoVertexAttribPointer( prog );
    //        GL_CHECK_ERROR;
    //        base::m_vao->drawElements( static_cast<GLenum>( base::m_renderMode ),
    //                                   GLsizei( m_numElements ),
    //                                   GL_UNSIGNED_INT,
    //                                   nullptr );
    //        GL_CHECK_ERROR;
    //        base::m_vao->unbind();
    //        GL_CHECK_ERROR;
    //    }
    /// \todo implement !
}

void GeometryDisplayable::autoVertexAttribPointer( const ShaderProgram* prog ) {

    auto glprog           = prog->getProgramObject();
    gl::GLint attribCount = glprog->get( GL_ACTIVE_ATTRIBUTES );

    for ( GLint idx = 0; idx < attribCount; ++idx )
    {
        const gl::GLsizei bufSize = 256;
        gl::GLchar name[bufSize];
        gl::GLsizei length;
        gl::GLint size;
        gl::GLenum type;
        glprog->getActiveAttrib( idx, bufSize, &length, &size, &type, name );
        auto loc = glprog->getAttributeLocation( name );

        auto attribName = m_translationTableShaderToMesh[name];
        auto attrib     = m_geom.getAttribBase( attribName );

        if ( attrib && attrib->getSize() > 0 )
        {
            m_vao->enable( loc );
            auto binding = m_vao->binding( idx );
            binding->setAttribute( loc );
            CORE_ASSERT( m_vbos[m_handleToBuffer[attribName]].get(), "vbo is nullptr" );
#ifdef CORE_USE_DOUBLE
            binding->setBuffer( m_vbos[m_handleToBuffer[attribName]].get(),
                                0,
                                attrib->getElementSize() * sizeof( float ) );
#else

            binding->setBuffer(
                m_vbos[m_handleToBuffer[attribName]].get(), 0, attrib->getStride() );
#endif
            binding->setFormat( attrib->getElementSize(), GL_SCALAR );
        }
        else
        { m_vao->disable( loc ); }
    }
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
