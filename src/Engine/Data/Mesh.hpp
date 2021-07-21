#pragma once

#include <Engine/RaEngine.hpp>

#include <Engine/Data/DisplayableObject.hpp>

#include <Core/Asset/GeometryData.hpp>
#include <Core/Containers/VectorArray.hpp>
#include <Core/Geometry/TriangleMesh.hpp>
#include <Core/Utils/Color.hpp>
#include <Core/Utils/Log.hpp>

#include <globjects/Buffer.h>
#include <globjects/VertexArray.h>

#include <array>
#include <map>
#include <vector>

#include <Core/Utils/ObjectWithSemantic.hpp>
#include <iterator>

namespace Ra {
namespace Engine {
namespace Data {
class ShaderProgram;

using namespace Ra::Core::Utils;

/// VAO + VBO attributes management,
/// also manage draw calls
class RA_ENGINE_API Vao
{
    /// \todo not used for now ... but may be if we allow multiple vao per mesh
    std::unique_ptr<globjects::VertexArray> m_vao;
    std::vector<std::unique_ptr<globjects::Buffer>> m_vbos;
    std::vector<bool> m_dataDirty;
    std::map<std::string, int> m_handleToBuffer;
};

/**
 * A class representing an openGL general mesh to be displayed.
 * It stores the vertex attributes, indices, and can be rendered
 * with a specific render mode (e.g. GL_TRIANGLES or GL_LINES).
 * It maintains the attributes and keeps them in sync with the GPU.
 * \note Attribute names are used to automatic location binding when using shaders.
 */
class RA_ENGINE_API AttribArrayDisplayable : public Displayable
{
  public:
    /// List of standard vertex attributes.
    /// Corresponding standard vertex attribute names are obtained with getAttribName
    /// Information which is in the mesh geometry
    enum MeshData : uint {
        VERTEX_POSITION,   ///< Vertex positions
        VERTEX_NORMAL,     ///< Vertex normals
        VERTEX_TANGENT,    ///< Vertex tangent 1
        VERTEX_BITANGENT,  ///< Vertex tangent 2
        VERTEX_TEXCOORD,   ///< U,V  texture coords (last coordinate not used)
        VERTEX_COLOR,      ///< RGBA color.
        VERTEX_WEIGHTS,    ///< Skinning weights (not used)
        VERTEX_WEIGHT_IDX, ///< Associated weight bones

        MAX_DATA
    };

    /// Render mode enum used when render()/
    /// values taken from OpenGL specification
    /// @see https://www.khronos.org/registry/OpenGL/api/GL/glcorearb.h
    enum MeshRenderMode : uint {
        RM_POINTS                   = 0x0000,
        RM_LINES                    = 0x0001, // decimal value: 1
        RM_LINE_LOOP                = 0x0002, // decimal value: 2
        RM_LINE_STRIP               = 0x0003, // decimal value: 3
        RM_TRIANGLES                = 0x0004, // decimal value: 4
        RM_TRIANGLE_STRIP           = 0x0005, // decimal value: 5
        RM_TRIANGLE_FAN             = 0x0006, // decimal value: 6
        RM_QUADS                    = 0x0007, // decimal value: 7
        RM_QUAD_STRIP               = 0x0008, // decimal value: 8
        RM_POLYGON                  = 0x0009, // decimal value: 9
        RM_LINES_ADJACENCY          = 0x000A, // decimal value: 10
        RM_LINE_STRIP_ADJACENCY     = 0x000B, // decimal value: 11
        RM_TRIANGLES_ADJACENCY      = 0x000C, // decimal value: 12
        RM_TRIANGLE_STRIP_ADJACENCY = 0x000D, // decimal value: 13
        RM_PATCHES                  = 0x000E, // decimal value: 14
    };

  public:
    explicit AttribArrayDisplayable( const std::string& name,
                                     MeshRenderMode renderMode = RM_TRIANGLES );
    AttribArrayDisplayable( const AttribArrayDisplayable& rhs ) = delete;
    void operator=( const AttribArrayDisplayable& rhs ) = delete;

    // no need to detach listener since TriangleMesh is owned by Mesh.
    ~AttribArrayDisplayable() {}

    using Displayable::getName;

    /// Set the render mode.
    inline void setRenderMode( MeshRenderMode mode );
    /// Get the render mode.
    inline MeshRenderMode getRenderMode() const;

    /// @name
    /// Mark attrib data as dirty, forcing an update of the OpenGL buffer.
    ///@{

    /// Use getAttribName to find the corresponding name and call setDirty(const std::string& name).
    /// \param type: the data to set to dirty.
    void setDirty( const MeshData& type );

    /// \param name: data buffer name to set to dirty
    void setDirty( const std::string& name );

    /// If index is greater than then number of buffer, this function as no effect.
    /// \param index: the data buffer index to set to dirty.
    void setDirty( unsigned int index );
    ///@}

    /// This function is called at the start of the rendering.
    /// It will update the necessary openGL buffers.
    void updateGL() override = 0;

    ///@{
    /// Get the name expected for a given attrib.
    static inline std::string getAttribName( MeshData type );
    ///@}

    ///@{
    /**  Returns the underlying CoreGeometry as an Core::Geometry::AttribArrayGeometry */
    virtual const Core::Geometry::AttribArrayGeometry& getAttribArrayGeometry() const = 0;
    virtual Core::Geometry::AttribArrayGeometry& getAttribArrayGeometry()             = 0;
    ///@}

  protected:
    /// Update the picking render mode according to the object render mode
    void updatePickingRenderMode();

    class AttribObserver
    {
      public:
        explicit AttribObserver( AttribArrayDisplayable* displayable, int idx ) :
            m_displayable( displayable ), m_idx( idx ) {}
        void operator()() {
            if ( m_idx < int( m_displayable->m_dataDirty.size() ) )
            {
                m_displayable->m_dataDirty[m_idx] = true;
                m_displayable->m_isDirty          = true;
            }
            else
            {
                /// \todo Should never be here
                LOG( logDEBUG ) << "Invalid dirty bit notified on " << m_displayable->getName();
            }
        }

      private:
        AttribArrayDisplayable* m_displayable;
        int m_idx;
    };

  protected:
    std::unique_ptr<globjects::VertexArray> m_vao;

    MeshRenderMode m_renderMode {MeshRenderMode::RM_TRIANGLES};

    // m_vbos and m_dataDirty have the same size and are indexed thru m_handleToBuffer[attribName]
    std::vector<std::unique_ptr<globjects::Buffer>> m_vbos;
    std::vector<bool> m_dataDirty;

    // Geometry attrib name (std::string) to buffer id (int)
    // buffer id are indices in m_vbos and m_dataDirty
    std::map<std::string, unsigned int> m_handleToBuffer;

    /// General dirty bit of the mesh. Must be equivalent of the "or" of the other dirty flags.
    /// an empty mesh is not dirty
    bool m_isDirty {false};
};

/// Concept class to ensure consistent naming of VaoIndices accross derived classes.
class RA_ENGINE_API VaoIndices
{
  public:
    /// Tag the indices as dirty, asking for a update to gpu.
    inline void setIndicesDirty();

    ///\todo Add test for Indices observer
    class IndicesObserver
    {
      public:
        /// not tested
        explicit IndicesObserver( VaoIndices* displayable ) : m_displayable {displayable} {}
        /// not tested
        void operator()() { m_displayable->m_indicesDirty = true; }

      private:
        VaoIndices* m_displayable;
    };

  protected:
    std::unique_ptr<globjects::Buffer> m_indices {nullptr};
    bool m_indicesDirty {true};
    /// number of elements to draw (i.e number of indices to use)
    /// automatically set by updateGL(), not meaningfull if m_indicesDirty.
    size_t m_numElements {0};
};

/// This class handles an attrib array displayable on gpu only, without core
/// geometry. Use only when you don't need to access the cpu geometry again, or
/// when you need to specify special indices.
template <typename I>
class IndexedAttribArrayDisplayable : public AttribArrayDisplayable, public VaoIndices
{
    using IndexType          = I;
    using IndexContainerType = Ra::Core::AlignedStdVector<IndexType>;

    template <typename T>
    inline void addAttrib( const std::string& name,
                           const typename Ra::Core::Utils::Attrib<T>::Container& data );
    template <typename T>
    inline void addAttrib( const std::string& name,
                           const typename Ra::Core ::Utils::Attrib<T>::Container&& data );
    inline void updateGL() override;

    inline void render( const ShaderProgram* prog ) override;

  protected:
    /// assume m_vao is bound.
    inline void autoVertexAttribPointer( const ShaderProgram* prog );
    IndexContainerType m_cpu_indices;
    AttribManager m_attribManager;
};

/// Template class to manage the Displayable aspect of a Core Geomertry, such as TriangleMesh.
template <typename T>
class CoreGeometryDisplayable : public AttribArrayDisplayable
{
  public:
    using base         = AttribArrayDisplayable;
    using CoreGeometry = T;

    explicit CoreGeometryDisplayable( const std::string& name,
                                      MeshRenderMode renderMode = RM_TRIANGLES );

    ///@{
    /**  Returns the underlying CoreGeometry as an Core::Geometry::AbstractGeometry */
    inline const Core::Geometry::AbstractGeometry& getAbstractGeometry() const override;
    inline Core::Geometry::AbstractGeometry& getAbstractGeometry() override;
    ///@}

    inline const Core::Geometry::AttribArrayGeometry& getAttribArrayGeometry() const override;
    inline Core::Geometry::AttribArrayGeometry& getAttribArrayGeometry() override;

    ///@{
    /// Returns the underlying CoreGeometry
    inline const CoreGeometry& getCoreGeometry() const;
    inline CoreGeometry& getCoreGeometry();
    ///@}

    /// Helper function that calls Ra::Core::CoreGeometry::addAttrib()
    template <typename A>
    inline Ra::Core::Utils::AttribHandle<A> addAttrib( const std::string& name,
                                                       const typename Core::VectorArray<A>& data );
    inline size_t getNumVertices() const override;

    /// Use the given geometry as base for a display mesh.
    /// This will move \p mesh and *this will take the ownership
    /// of the data.
    /// The currently owned mesh is deleted, if any.
    /// This method should be called to set or replace the CoreGeometry, if you
    /// want to update attributes or indices, use getCoreGeometry and
    /// Core::Geometry::AttribArrayGeometry setters instead.
    /// \warning For indices, you must call setIndicesDirty after modification.
    /// \todo add observer mecanism for indices.
    virtual void loadGeometry( CoreGeometry&& mesh );

    /// Update (i.e. send to GPU) the buffers marked as dirty
    void updateGL() override;

    /// Bind meshAttribName to shaderAttribName.
    /// meshAttribName is a vertex attrib added to the underlying CoreGeometry
    /// shaderAttribName is the name of the input paramter of the shader.
    /// By default the same name is used, but this mecanism allows to override
    /// this behavior.
    /// Only one shaderAttribName can be bound to a meshAttribName and the other
    /// way round.
    /// \param meshAttribName: name of the attribute on the CoreGeomtry side
    /// \param shaderAttribName: name of the input vertex attribute on the
    /// shader side.
    void setAttribNameCorrespondance( const std::string& meshAttribName,
                                      const std::string& shaderAttribName );

  protected:
    virtual void updateGL_specific_impl() {}

    void loadGeometry_common( CoreGeometry&& mesh );
    void setupCoreMeshObservers();

    /// assume m_vao is bound.
    void autoVertexAttribPointer( const ShaderProgram* prog );

    /// m_mesh Observer method, called whenever an attrib is added or removed from
    /// m_mesh.
    /// it adds an observer to the new attrib.
    void addAttribObserver( const std::string& name );

    void addToTranslationTable( const std::string& name );

    /// Core::Mesh attrib name to Render::Mesh attrib name
    using TranslationTable = std::map<std::string, std::string>;
    TranslationTable m_translationTableMeshToShader;
    TranslationTable m_translationTableShaderToMesh;

    CoreGeometry m_mesh;
};

/// A PointCloud without indices
class RA_ENGINE_API PointCloud : public CoreGeometryDisplayable<Core::Geometry::PointCloud>
{
    using base = CoreGeometryDisplayable<Core::Geometry::PointCloud>;

  public:
    using base::CoreGeometryDisplayable;
    inline explicit PointCloud(
        const std::string& name,
        typename base::CoreGeometry&& geom,
        typename base::MeshRenderMode renderMode = base::MeshRenderMode::RM_POINTS );

    inline explicit PointCloud( const std::string& name, MeshRenderMode renderMode = RM_POINTS );

    /// use glDrawArrays to draw all the points in the point cloud
    void render( const ShaderProgram* prog ) override;

    void loadGeometry( Core::Geometry::PointCloud&& mesh ) override;

  protected:
    void updateGL_specific_impl() override;
};

/// An engine mesh owning CoreGeometry, with indices
template <typename T>
class IndexedGeometry : public CoreGeometryDisplayable<T>, public VaoIndices
{
  public:
    using base = CoreGeometryDisplayable<T>;
    using CoreGeometryDisplayable<T>::CoreGeometryDisplayable;
    explicit IndexedGeometry(
        const std::string& name,
        typename base::CoreGeometry&& geom,
        typename base::MeshRenderMode renderMode = base::MeshRenderMode::RM_TRIANGLES );

    void render( const ShaderProgram* prog ) override;

    void loadGeometry( T&& mesh ) override;

  protected:
    void updateGL_specific_impl() override;
};

/// An engine mesh owning a MultiIndexedCoreGeometry, with multiple indices layer.
/// \todo Work in progress.
template <typename T>
class MultiIndexedGeometry : public CoreGeometryDisplayable<T>
{
  public:
    using base = CoreGeometryDisplayable<T>;
    using CoreGeometryDisplayable<T>::CoreGeometryDisplayable;
    explicit MultiIndexedGeometry(
        const std::string& name,
        typename base::CoreGeometry&& geom,
        typename base::MeshRenderMode renderMode = base::MeshRenderMode::RM_TRIANGLES );
    void render( const ShaderProgram* prog ) override;

    void loadGeometry( T&& mesh ) override;

  protected:
    void updateGL_specific_impl() override;

    using LayerSemanticCollection = Core::Utils::ObjectWithSemantic::SemanticNameCollection;
    using LayerSemantic           = Core::Utils::ObjectWithSemantic::SemanticName;
    using LayerKeyType            = std::pair<LayerSemanticCollection, std::string>;

    using EntryType = std::pair<bool, VaoIndices*>;
    struct RA_CORE_API KeyHash {
        std::size_t operator()( const LayerKeyType& k ) const {
            // Mix semantic collection into a single identifier string
            std::ostringstream stream;
            std::copy(
                k.first.begin(), k.first.end(), std::ostream_iterator<std::string>( stream, "" ) );
            std::string result = stream.str();
            std::sort( result.begin(), result.end() );

            // Combine with layer name hash
            return std::hash<std::string> {}( result ) ^
                   ( std::hash<std::string> {}( k.second ) << 1 );
        }
    };
    std::unordered_map<LayerKeyType, EntryType, KeyHash> m_indices;
};

/// LineMesh, own a Core::Geometry::LineMesh
class RA_ENGINE_API LineMesh : public IndexedGeometry<Core::Geometry::LineMesh>
{
    using base = IndexedGeometry<Core::Geometry::LineMesh>;

  public:
    using base::IndexedGeometry;
    inline explicit LineMesh(
        const std::string& name,
        typename base::CoreGeometry&& geom,
        typename base::MeshRenderMode renderMode = base::MeshRenderMode::RM_LINES );
    inline explicit LineMesh(
        const std::string& name,
        typename base::MeshRenderMode renderMode = base::MeshRenderMode::RM_LINES );

  protected:
  private:
};

/// Mesh, own a Core::Geometry::TriangleMesh
class RA_ENGINE_API Mesh : public IndexedGeometry<Core::Geometry::TriangleMesh>
{
    using base = IndexedGeometry<Core::Geometry::TriangleMesh>;

  public:
    using base::IndexedGeometry;
    size_t getNumFaces() const override;

    /**
     * Use the given vertices and indices to build a display mesh according to
     * the MeshRenderMode.
     * \note This has to be used for non RM_TRIANGLES meshes only.
     * \note Also removes all vertex attributes.
     * \warning This will disappear as soon as old code will be removed.
     */
    using base::loadGeometry;
    [[deprecated]] void loadGeometry( const Core::Vector3Array& vertices,
                                      const std::vector<uint>& indices );

  protected:
  private:
};

/// PolyMesh, own a Core::Geometry::PolyMesh
/// This class handle the GPU representation of a polyhedron mesh.
/// Each face of the polyhedron (typically quads) are assume to be planar and convex.
/// Simple triangulation is performed on the fly before sending data to the GPU.
class RA_ENGINE_API PolyMesh : public IndexedGeometry<Core::Geometry::PolyMesh>
{
    using base      = IndexedGeometry<Core::Geometry::PolyMesh>;
    using IndexType = Core::Vector3ui;

  public:
    using base::IndexedGeometry;
    inline size_t getNumFaces() const override;

  protected:
    inline void updateGL_specific_impl() override;

  private:
    inline void triangulate();
    Core::AlignedStdVector<IndexType> m_triangleIndices;
};

/// create a TriangleMesh, PolyMesh or other Core::*Mesh from GeometryData
template <typename CoreMeshType>
CoreMeshType createCoreMeshFromGeometryData( const Ra::Core::Asset::GeometryData* data ) {
    CoreMeshType mesh;
    typename CoreMeshType::PointAttribHandle::Container vertices;
    typename CoreMeshType::NormalAttribHandle::Container normals;
    typename CoreMeshType::IndexContainerType indices;

    vertices.reserve( data->getVerticesSize() );
    std::copy(
        data->getVertices().begin(), data->getVertices().end(), std::back_inserter( vertices ) );

    if ( data->hasNormals() )
    {
        normals.reserve( data->getVerticesSize() );
        std::copy(
            data->getNormals().begin(), data->getNormals().end(), std::back_inserter( normals ) );
    }

    const auto& faces = data->getFaces();
    indices.reserve( faces.size() );
    std::copy( faces.begin(), faces.end(), std::back_inserter( indices ) );
    mesh.setVertices( std::move( vertices ) );
    mesh.setNormals( std::move( normals ) );

    // \todo remove when data will handle all the attributes in a coherent way.
    if ( data->hasTangents() )
    {
        mesh.addAttrib( Data::Mesh::getAttribName( Data::Mesh::VERTEX_TANGENT ),
                        data->getTangents() );
    }

    if ( data->hasBiTangents() )
    {
        mesh.addAttrib( Data::Mesh::getAttribName( Data::Mesh::VERTEX_BITANGENT ),
                        data->getBiTangents() );
    }

    if ( data->hasTextureCoordinates() )
    {
        mesh.addAttrib( Data::Mesh::getAttribName( Data::Mesh::VERTEX_TEXCOORD ),
                        data->getTexCoords() );
    }

    // add custom attribs
    // only attributs not handled before are handled by data->getAttribManager()
    // but futur plan will handle also "usual" attibutes this way
    mesh.vertexAttribs().copyAllAttributes( data->getAttribManager() );

    // also this one will be automatically done with the futur behavior
    //        mesh->addData( Data::Mesh::VERTEX_WEIGHTS, meshData.weights );

    mesh.setIndices( std::move( indices ) );

    return mesh;
}

/// Helpers to get RenderMesh type from CoreMesh Type
namespace RenderMeshType {
template <class CoreMeshT>
struct getType {};

template <>
struct getType<Ra::Core::Geometry::LineMesh> {
    using Type = Ra::Engine::Data::LineMesh;
};

template <>
struct getType<Ra::Core::Geometry::TriangleMesh> {
    using Type = Ra::Engine::Data::Mesh;
};

template <>
struct getType<Ra::Core::Geometry::PolyMesh> {
    using Type = Ra::Engine::Data::PolyMesh;
};
} // namespace RenderMeshType

/// create Mesh, PolyMesh Engine::Data::*Mesh * from GeometryData
template <typename CoreMeshType>
typename RenderMeshType::getType<CoreMeshType>::Type*
createMeshFromGeometryData( const std::string& name, const Ra::Core::Asset::GeometryData* data ) {
    using MeshType = typename RenderMeshType::getType<CoreMeshType>::Type;

    CoreMeshType mesh = createCoreMeshFromGeometryData<CoreMeshType>( data );

    MeshType* ret = new MeshType {name};
    ret->loadGeometry( std::move( mesh ) );

    return ret;
}

} // namespace Data
} // namespace Engine
} // namespace Ra
#include <Engine/Data/Mesh.inl>
