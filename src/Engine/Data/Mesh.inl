#pragma once
#include <Engine/Data/Mesh.hpp>

#include <Engine/Data/ShaderProgram.hpp>

#include <Engine/Data/ShaderProgram.hpp>
#include <globjects/Buffer.h>
#include <globjects/Program.h>
#include <globjects/VertexAttributeBinding.h>

namespace Ra {
namespace Engine {
namespace Data {
////////////////  AttribArrayDisplayable ///////////////////////////////

void AttribArrayDisplayable::setRenderMode( MeshRenderMode mode ) {
    m_renderMode = mode;
    updatePickingRenderMode();
}

AttribArrayDisplayable::MeshRenderMode AttribArrayDisplayable::getRenderMode() const {
    return m_renderMode;
}

std::string AttribArrayDisplayable::getAttribName( MeshData type ) {
    if ( type == VERTEX_POSITION ) return {"in_position"};
    if ( type == VERTEX_NORMAL ) return {"in_normal"};
    if ( type == VERTEX_TANGENT ) return {"in_tangent"};
    if ( type == VERTEX_BITANGENT ) return {"in_bitangent"};
    if ( type == VERTEX_TEXCOORD ) return {"in_texcoord"};
    if ( type == VERTEX_COLOR ) return {"in_color"};
    if ( type == VERTEX_WEIGHTS ) return {"in_weight"};
    if ( type == VERTEX_WEIGHT_IDX ) return {"in_weight_idx"};
    return {"invalid mesh data attr name"};
}

///////////////// VaoIndices  ///////////////////////

void VaoIndices::setIndicesDirty() {
    m_indicesDirty = true;
}

////////////////  CoreGeometryDisplayable ///////////////////////////////

template <typename CoreGeometry>
CoreGeometryDisplayable<CoreGeometry>::CoreGeometryDisplayable( const std::string& name,
                                                                MeshRenderMode renderMode ) :
    base( name, renderMode ) {
    setupCoreMeshObservers();
}

template <typename CoreGeometry>
const Ra::Core::Geometry::AbstractGeometry&
CoreGeometryDisplayable<CoreGeometry>::getAbstractGeometry() const {
    return m_mesh;
}

template <typename CoreGeometry>
Ra::Core::Geometry::AbstractGeometry& CoreGeometryDisplayable<CoreGeometry>::getAbstractGeometry() {
    return m_mesh;
}

template <typename CoreGeometry>
const Ra::Core::Geometry::AttribArrayGeometry&
CoreGeometryDisplayable<CoreGeometry>::getAttribArrayGeometry() const {
    return m_mesh;
}

template <typename CoreGeometry>
Ra::Core::Geometry::AttribArrayGeometry&
CoreGeometryDisplayable<CoreGeometry>::getAttribArrayGeometry() {
    return m_mesh;
}

template <typename CoreGeometry>
const CoreGeometry& CoreGeometryDisplayable<CoreGeometry>::getCoreGeometry() const {
    return m_mesh;
}

template <typename CoreGeometry>
CoreGeometry& CoreGeometryDisplayable<CoreGeometry>::getCoreGeometry() {
    return m_mesh;
}

template <typename CoreGeometry>
void CoreGeometryDisplayable<CoreGeometry>::addToTranslationTable( const std::string& name ) {
    auto it = m_translationTableMeshToShader.find( name );
    if ( it == m_translationTableMeshToShader.end() )
    {
        m_translationTableMeshToShader[name] = name;
        m_translationTableShaderToMesh[name] = name;
    }
}

template <typename CoreGeometry>
void CoreGeometryDisplayable<CoreGeometry>::addAttribObserver( const std::string& name ) {
    // this observer is called each time an attrib is added or removed from m_mesh
    auto attrib = m_mesh.getAttribBase( name );
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

template <typename CoreGeometry>
void CoreGeometryDisplayable<CoreGeometry>::autoVertexAttribPointer( const ShaderProgram* prog ) {

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
        auto attrib     = m_mesh.getAttribBase( attribName );

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

template <typename T>
void CoreGeometryDisplayable<T>::loadGeometry_common( T&& mesh ) {
    m_mesh = std::move( mesh );
    setupCoreMeshObservers();
}

template <typename T>

void CoreGeometryDisplayable<T>::setupCoreMeshObservers() {
    int idx = 0;
    m_dataDirty.resize( m_mesh.vertexAttribs().getNumAttribs() );
    m_vbos.resize( m_mesh.vertexAttribs().getNumAttribs() );
    // here capture ref to idx to propagate idx incrementation
    m_mesh.vertexAttribs().for_each_attrib( [&idx, this]( Ra::Core::Utils::AttribBase* b ) {
        auto name              = b->getName();
        m_handleToBuffer[name] = idx;
        m_dataDirty[idx]       = true;

        // create a identity translation if name is not already translated.
        addToTranslationTable( name );

        b->attach( AttribObserver( this, idx ) );
        ++idx;
    } );

    // add an observer on attrib manipulation.
    m_mesh.vertexAttribs().attachMember(
        this, &CoreGeometryDisplayable<CoreGeometry>::addAttribObserver );
    m_isDirty = true;
}

/// Helper function that calls Ra::Core::CoreGeometry::addAttrib()
template <typename CoreGeometry>
template <typename A>
Ra::Core::Utils::AttribHandle<A>
CoreGeometryDisplayable<CoreGeometry>::addAttrib( const std::string& name,
                                                  const typename Core::VectorArray<A>& data ) {
    return m_mesh.addAttrib( name, data );
}

template <typename CoreGeometry>
size_t CoreGeometryDisplayable<CoreGeometry>::getNumVertices() const {
    return m_mesh.vertices().size();
}

template <typename CoreGeometry>
void CoreGeometryDisplayable<CoreGeometry>::loadGeometry( CoreGeometry&& /*mesh*/ ) {
    CORE_ASSERT( false, "must be specialized" );
}

template <typename CoreGeometry>
void CoreGeometryDisplayable<CoreGeometry>::updateGL() {
    if ( m_isDirty )
    {
        // Check that our dirty bits are consistent.
        ON_ASSERT( bool dirtyTest = false; for ( auto d
                                                 : m_dataDirty ) { dirtyTest = dirtyTest || d; } );
        CORE_ASSERT( dirtyTest == m_isDirty, "Dirty flags inconsistency" );
        CORE_ASSERT( !( m_mesh.vertices().empty() ), "No vertex." );

        updateGL_specific_impl();
#ifdef CORE_USE_DOUBLE
        // need convserion
        auto func = [this]( Ra::Core::Utils::AttribBase* b ) {
            auto idx = m_handleToBuffer[b->getName()];

            if ( m_dataDirty[idx] )
            {
                if ( !m_vbos[idx] ) { m_vbos[idx] = globjects::Buffer::create(); }

                auto stride      = b->getStride();
                auto eltSize     = b->getElementSize();
                auto size        = b->getSize();
                auto data        = std::make_unique<float[]>( size * eltSize );
                const void* ptr  = b->dataPtr();
                const char* cptr = reinterpret_cast<const char*>( ptr );

                for ( size_t i = 0; i < size; i++ )
                {
                    auto tptr = reinterpret_cast<const Scalar*>( cptr + i * stride );
                    for ( size_t j = 0; j < eltSize; ++j )
                    {
                        data[i * eltSize + j] = tptr[j];
                    }
                }

                m_vbos[idx]->setData(
                    size * eltSize * sizeof( float ), data.get(), GL_DYNAMIC_DRAW );

                m_dataDirty[idx] = false;
            }
        };

#else
        auto func = [this]( Ra::Core::Utils::AttribBase* b ) {
            auto idx = m_handleToBuffer[b->getName()];

            if ( m_dataDirty[idx] )
            {
                if ( !m_vbos[idx] ) { m_vbos[idx] = globjects::Buffer::create(); }
                m_vbos[idx]->setData( b->getBufferSize(), b->dataPtr(), GL_DYNAMIC_DRAW );
                m_dataDirty[idx] = false;
            }
        };
#endif
        m_mesh.vertexAttribs().for_each_attrib( func );

        // cleanup removed attrib
        for ( auto buffer : m_handleToBuffer )
        {
            // do not remove name from handleToBuffer to keep index ...
            // we could also update handleToBuffer, m_vbos, m_dataDirty
            if ( !m_mesh.hasAttrib( buffer.first ) && m_vbos[buffer.second] )
            {
                m_vbos[buffer.second].reset( nullptr );
                m_dataDirty[buffer.second] = false;
            }
        }

        GL_CHECK_ERROR;
        m_isDirty = false;
    }
}

template <typename CoreGeometry>
void CoreGeometryDisplayable<CoreGeometry>::setAttribNameCorrespondance(
    const std::string& meshAttribName,
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

////////////////  IndexedGeometry  ///////////////////////////////

template <typename T>
IndexedGeometry<T>::IndexedGeometry( const std::string& name,
                                     typename base::CoreGeometry&& geom,
                                     typename base::MeshRenderMode renderMode ) :
    base( name, renderMode ) {
    loadGeometry( std::move( geom ) );
}

template <typename T>
void IndexedGeometry<T>::loadGeometry( T&& mesh ) {
    setIndicesDirty();
    base::loadGeometry_common( std::move( mesh ) );

    // indices
    base::m_mesh.attach( IndicesObserver( this ) );
}

template <typename T>
void IndexedGeometry<T>::updateGL_specific_impl() {
    if ( !m_indices )
    {
        m_indices      = globjects::Buffer::create();
        m_indicesDirty = true;
    }
    if ( m_indicesDirty )
    {
        /// this one do not work since m_indices is not a std::vector
        // m_indices->setData( m_mesh.m_indices, GL_DYNAMIC_DRAW );
        m_numElements =
            base::m_mesh.getIndices().size() * base::CoreGeometry::IndexType::RowsAtCompileTime;

        m_indices->setData(
            static_cast<gl::GLsizeiptr>( base::m_mesh.getIndices().size() *
                                         sizeof( typename base::CoreGeometry::IndexType ) ),
            base::m_mesh.getIndices().data(),
            GL_STATIC_DRAW );
        m_indicesDirty = false;
    }
    if ( !base::m_vao ) { base::m_vao = globjects::VertexArray::create(); }
    base::m_vao->bind();
    base::m_vao->bindElementBuffer( m_indices.get() );
    base::m_vao->unbind();
}

template <typename T>
void IndexedGeometry<T>::render( const ShaderProgram* prog ) {
    if ( base::m_vao )
    {
        GL_CHECK_ERROR;
        base::m_vao->bind();
        base::autoVertexAttribPointer( prog );
        GL_CHECK_ERROR;
        base::m_vao->drawElements( static_cast<GLenum>( base::m_renderMode ),
                                   GLsizei( m_numElements ),
                                   GL_UNSIGNED_INT,
                                   nullptr );
        GL_CHECK_ERROR;
        base::m_vao->unbind();
        GL_CHECK_ERROR;
    }
}

///////// PointCloud //////////

PointCloud::PointCloud( const std::string& name,
                        typename base::CoreGeometry&& geom,
                        typename base::MeshRenderMode renderMode ) :
    base( name, renderMode ) {
    loadGeometry( std::move( geom ) );
}

PointCloud::PointCloud( const std::string& name, typename base::MeshRenderMode renderMode ) :
    base( name, renderMode ) {}

/////////  LineMesh ///////////

LineMesh::LineMesh( const std::string& name,
                    typename base::CoreGeometry&& geom,
                    typename base::MeshRenderMode renderMode ) :
    base( name, std::move( geom ), renderMode ) {}

LineMesh::LineMesh( const std::string& name, typename base::MeshRenderMode renderMode ) :
    base( name, renderMode ) {}

/////////  PolyMesh ///////////

size_t PolyMesh::getNumFaces() const {
    return getCoreGeometry().getIndices().size();
}

void PolyMesh::updateGL_specific_impl() {
    if ( !m_indices )
    {
        m_indices      = globjects::Buffer::create();
        m_indicesDirty = true;
    }
    if ( m_indicesDirty )
    {
        triangulate();
        /// this one do not work since m_indices is not a std::vector
        // m_indices->setData( m_mesh.m_indices, GL_DYNAMIC_DRAW );
        m_numElements = m_triangleIndices.size() * PolyMesh::IndexType::RowsAtCompileTime;

        m_indices->setData(
            static_cast<gl::GLsizeiptr>( m_triangleIndices.size() * sizeof( PolyMesh::IndexType ) ),
            m_triangleIndices.data(),
            GL_STATIC_DRAW );
        m_indicesDirty = false;
    }
    if ( !base::m_vao ) { base::m_vao = globjects::VertexArray::create(); }
    base::m_vao->bind();
    base::m_vao->bindElementBuffer( m_indices.get() );
    base::m_vao->unbind();
}

void PolyMesh::triangulate() {
    m_triangleIndices.clear();
    m_triangleIndices.reserve( m_mesh.getIndices().size() );
    for ( const auto& face : m_mesh.getIndices() )
    {
        if ( face.size() == 3 ) { m_triangleIndices.push_back( face ); }
        else
        {
            /// simple sew triangulation
            int minus {int( face.size() ) - 1};
            int plus {0};
            while ( plus + 1 < minus )
            {
                if ( ( plus - minus ) % 2 )
                {
                    m_triangleIndices.emplace_back( face[plus], face[plus + 1], face[minus] );
                    ++plus;
                }
                else
                {
                    m_triangleIndices.emplace_back( face[minus], face[plus], face[minus - 1] );
                    --minus;
                }
            }
        }
    }
}

} // namespace Data
} // namespace Engine
} // namespace Ra
