#pragma once

#include <Core/RaCore.hpp>

#include <Core/Containers/VectorArray.hpp>
#include <Core/Geometry/OpenMesh.hpp>
#include <Core/Geometry/TriangleMesh.hpp>
#include <Core/Types.hpp>
#include <Core/Utils/Index.hpp>
#include <Core/Utils/StdOptional.hpp>

#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>
#include <OpenMesh/Core/Mesh/Traits.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <OpenMesh/Core/Utils/Property.hh>
#include <OpenMesh/Core/Utils/PropertyManager.hh>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <set>

namespace Ra {
namespace Core {
namespace Geometry {
using namespace Utils; // log, AttribXXX

/**
 * Define the Traits to be used by OpenMesh for TopologicalMesh.
 */
struct TopologicalMeshTraits : OpenMesh::DefaultTraits {
    using Point  = Ra::Core::Vector3;
    using Normal = Ra::Core::Vector3;

    VertexAttributes( OpenMesh::Attributes::Status | OpenMesh::Attributes::Normal );
    FaceAttributes( OpenMesh::Attributes::Status | OpenMesh::Attributes::Normal );
    EdgeAttributes( OpenMesh::Attributes::Status );
    // Add  OpenMesh::Attributes::PrevHalfedge for efficiency ?
    HalfedgeAttributes( OpenMesh::Attributes::Status | OpenMesh::Attributes::Normal );
};

/**
 * This class represents a mesh with topological information on the
 * vertex graph, using a half-edge representation.
 *
 * This integration is inspired by:
 * https://gist.github.com/Unril/03fa353d0461ed6bd41d
 *
 * \todo rename methods to respect Radium guideline (get/set/is, camelCase)
 * \todo private inheritance from OpenMesh, and import relevant methods.
 */
class RA_CORE_API TopologicalMesh : public OpenMesh::PolyMesh_ArrayKernelT<TopologicalMeshTraits>
{
  private:
    using base = OpenMesh::PolyMesh_ArrayKernelT<TopologicalMeshTraits>;
    using base::PolyMesh_ArrayKernelT;
    using Vector3 = Ra::Core::Vector3;
    using Index   = Ra::Core::Utils::Index;
    class Wedge;
    class WedgeCollection;

  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    class WedgeData;
    using WedgeIndex = Ra::Core::Utils::Index;

    /**
     * Construct an empty topological mesh, only initialize mandatory properties.
     */
    explicit TopologicalMesh();

    /**
     * \brief Convenience constructor
     * \see TopologicalMesh( const Ra::Core::Geometry::TriangleMesh&, NonManifoldFaceCommand)
     */
    template <typename MeshIndex>
    explicit TopologicalMesh( [[deprecated(
        "Use MultiIndexedGeometry instead" )]] const Ra::Core::Geometry::IndexedGeometry<MeshIndex>&
                                  mesh );

    /**
     * \brief Convenience constructor
     * \see TopologicalMesh( const Ra::Core::Geometry::TriangleMesh&, NonManifoldFaceCommand)
     * @todo, when MultiIndexedGeometry will be operational, will this replace the above ?
     */
    explicit TopologicalMesh(
        const Ra::Core::Geometry::MultiIndexedGeometry& mesh,
        const Ra::Core::Geometry::MultiIndexedGeometry::LayerKeyType& layerKey );

    /**
     * Construct a topological mesh from a triangle mesh.
     * This operation merges vertex with same position, but keeps vertex
     * attributes on halfedges, so that TriangleMesh vertices with the same 3D
     * position are represented only once in the topological mesh.
     * \note This is a costly operation.
     *
     * \tparam NonManifoldFaceCommand Command executed when non-manifold faces are
     * found. API and default implementation:
     * \snippet Core/Geometry/TopologicalMesh.cpp Default command implementation
     *
     */
    template <typename MeshIndex, typename NonManifoldFaceCommand>
    explicit TopologicalMesh( [[deprecated( "Use MultiIndexedGeometry instead" )]] const Ra::Core::
                                  Geometry::IndexedGeometry<MeshIndex>& mesh,
                              NonManifoldFaceCommand command );

    /**
     * Construct a topological mesh from a triangle mesh.
     * This operation merges vertex with same position, but keeps vertex
     * attributes on halfedges, so that TriangleMesh vertices with the same 3D
     * position are represented only once in the topological mesh.
     * \note This is a costly operation.
     *
     * \tparam NonManifoldFaceCommand Command executed when non-manifold faces are
     * found. API and default implementation:
     * \snippet Core/Geometry/TopologicalMesh.cpp Default command implementation
     * @todo, when MultiIndexedGeometry will be operational, will this replace the above ?
     *
     */
    template <typename NonManifoldFaceCommand>
    explicit TopologicalMesh(
        const Ra::Core::Geometry::MultiIndexedGeometry& mesh,
        const Ra::Core::Geometry::MultiIndexedGeometry::LayerKeyType& layerKey,
        NonManifoldFaceCommand command );

    inline void
    initWithWedge( const Ra::Core::Geometry::MultiIndexedGeometry& mesh,
                   const Ra::Core::Geometry::MultiIndexedGeometry::LayerKeyType& layerKey );

    template <typename NonManifoldFaceCommand>
    void initWithWedge( const Ra::Core::Geometry::MultiIndexedGeometry& mesh,
                        const Ra::Core::Geometry::MultiIndexedGeometry::LayerKeyType& layerKey,
                        NonManifoldFaceCommand command );
    /**
     * Return a triangleMesh from the topological mesh.
     * \note This is a costly operation.
     * \warning It uses the attributes defined on halfedges. Do not work well if attribs are defined
     * on wedges (initWithWedge), in this case use toTriangleMeshFromWedges()
     */
    TriangleMesh toTriangleMesh();

    /**
     * Return a triangleMesh from the topological mesh.
     * \note This is a costly operation.
     * \warning It uses the attributes defined on wedges
     */
    PolyMesh toPolyMesh();

    LineMesh toLineMesh();

    /**
     * Update triangle mesh data, assuming the mesh and this topo mesh has the
     * same topology.
     */
    void updateTriangleMesh( Ra::Core::Geometry::MultiIndexedGeometry& mesh );
    void updateTriangleMeshNormals( Ra::Core::Geometry::MultiIndexedGeometry& mesh );
    void updateTriangleMeshNormals( AttribArrayGeometry::NormalAttribHandle::Container& normals );

    void update( const Ra::Core::Geometry::MultiIndexedGeometry& mesh );
    void updateNormals( const Ra::Core::Geometry::MultiIndexedGeometry& mesh );
    void updatePositions( const Ra::Core::Geometry::MultiIndexedGeometry& mesh );
    void updatePositions( const AttribArrayGeometry::PointAttribHandle::Container& vertices );
    // import other version of halfedge_handle method
    using base::halfedge_handle;

    /**
     * Return the half-edge associated with a given vertex and face.
     * \note Asserts if vh is not a member of fh.
     */
    inline HalfedgeHandle halfedge_handle( VertexHandle vh, FaceHandle fh ) const;

    /// Import Base definition of normal and set normal.
    ///@{
    using base::normal;
    using base::set_normal;
    ///@}

    /**
     * Set the normal n to all the wedges that share to vh.
     * If you work with vertex normals, please call this function on all vertex
     * handles before convertion with toTriangleMesh.
     */
    inline void propagate_normal_to_wedges( VertexHandle vh );

    /**
     * Return a handle to the halfedge property storing vertices indices within
     * the TriangleMesh *this has been built on.
     */
    inline const OpenMesh::HPropHandleT<Index>& getInputTriangleMeshIndexPropHandle() const;

    /**
     * Return a handle to the halfedge property storing vertices indices within
     * the TriangleMesh returned by toTriangleMesh().
     * \note This property is valid only after toTriangleMesh() has been called.
     */
    inline const OpenMesh::HPropHandleT<Index>& getOutputTriangleMeshIndexPropHandle() const;

    /**
     * \name Dealing with normals
     * Utils to deal with normals when modifying the mesh topology.
     */
    ///@{

    void updateWedgeNormals();
    ///@}

    /// update wedge's position to correspond to current point position, which might not be the case
    /// if point(handle) = something; has been used.
    void copyPointsPositionToWedges();
    /**
     * \name Topological operations
     */
    ///@{
    /**
     * Apply a 2-4 edge split.
     * \param eh The handle to the edge to split.
     * \param f The interpolation factor to place the new point on the edge.
     *          Must be in [0,1].
     * \return True if the edge has been split, false otherwise.
     * \note Only applies on edges between 2 triangles, and if \a f is in [0,1].
     * \note Mesh attributes are linearly interpolated on the newly created
     * halfedge.
     * \note f=0 correspond to halfedge_handle( eh, 0 ) (i.e. first vertex of
     * the edge)
     */
    bool splitEdge( TopologicalMesh::EdgeHandle eh, Scalar f );

    /**
     * Halfedge collapes \a he.
     * vo=from_vertex_handle(he) is deleted.
     * After collapse vo incoming halfedges points to vh = to_vertex_handle(he).
     * Wedge indices are updated to reflect the change in topology.
     * For detailed topological modifications see \ref develmeshes.
     * \param he halfedge's hangle to collapse.
     */
    void collapse( HalfedgeHandle, bool = false );
    [[deprecated( "use collapse() instead." )]] void
    collapseWedge( TopologicalMesh::HalfedgeHandle he, bool keepFromVertex = false );
    ///@}

    /**
     * Return the set of WedgeIndex incident to a given Vertex \a vh.
     * only valid non deleted wedges are present in the set.
     */
    inline std::set<WedgeIndex> getVertexWedges( OpenMesh::VertexHandle vh ) const;

    /**
     * get the wedge index associated with an halfedge
     */
    inline WedgeIndex getWedgeIndex( OpenMesh::HalfedgeHandle heh ) const;

    /**
     * Access to wedge data.
     * \param idx must be valid and correspond to a non delete wedge index.
     */
    inline const WedgeData& getWedgeData( const WedgeIndex& idx ) const;
    template <typename T>
    [[deprecated( "use getWedgeAttrib() instead." )]] const T&
    getWedgeData( const WedgeIndex& idx, const std::string& name ) const;
    template <typename T>
    const T& getWedgeAttrib( const WedgeIndex& idx, const std::string& name ) const;

    /**
     * Return the wedge refcount, for debug purpose.
     */
    inline unsigned int getWedgeRefCount( const WedgeIndex& idx ) const;

    /** set WedgeData \a wd to the wedge with index \a widx.
     * All halfedge that point to widx will get the new values.
     * \param widx index of the wedge
     * \param wd data to set to wedge that correspond to widx
     */
    inline void setWedgeData( WedgeIndex widx, const WedgeData& wd );

    /**
     * Change the WedgeData associated for \a idx, for attrib \a name to \a value.
     * The data is changed for all halfedges referencing this wedge.
     * \return true if the wedge is set, false if nothing set, i.e. if name is not an attrib of
     * type T.
     */
    template <typename T>
    [[deprecated( "use setWedgeAttrib() instead." )]] inline bool
    setWedgeData( const WedgeIndex& idx, const std::string& name, const T& value );

    template <typename T>
    inline bool setWedgeAttrib( const WedgeIndex& idx, const std::string& name, const T& value );

    /** return a WedgeData with all attrib initialized to default values */
    inline WedgeData newWedgeData() const { return m_wedges.newWedgeData(); }
    /** return a WedgeData with position (and vertex handle) initialized from the value of he's to
     * vertex. */
    inline WedgeData newWedgeData( HalfedgeHandle he ) const {
        return m_wedges.newWedgeData( to_vertex_handle( he ), point( to_vertex_handle( he ) ) );
    }

    /**
     * Replace the wedge data associated with an halfedge.
     * The old wedge is "deleted". If wedge data correspond to an already
     * present wedge, it's index is used.
     */
    inline void replaceWedge( OpenMesh::HalfedgeHandle he, const WedgeData& wd );

    template <typename T>
    inline int addWedgeAttrib( const std::string& name, T value = {} ) {
        return m_wedges.addAttrib<T>( name, value );
    }

    /**
     * Replace the wedge index associated with an halfedge.
     * The old wedge is "deleted". The new wedge reference count is incremented.
     */
    inline void replaceWedgeIndex( OpenMesh::HalfedgeHandle he, const WedgeIndex& widx );

    /**
     * call mergeEquelWedges( vh ) for every vertices of the mesh.
     * \see void mergeEqualWedges( OpenMesh::VertexHandle vh );
     */
    inline void mergeEqualWedges();

    /**
     * Merge (make same index) wegdes with the same data around \a vh
     * \param vh vertex handle to process
     */
    inline void mergeEqualWedges( OpenMesh::VertexHandle vh );

    /// Remove deleted element from the mesh, including wedges.
    void garbage_collection();
    inline void clean() {
        base::clean();
        m_wedges.clean();
    }

    inline const std::vector<std::string>& getVec4AttribNames() const;
    inline const std::vector<std::string>& getVec3AttribNames() const;
    inline const std::vector<std::string>& getVec2AttribNames() const;
    inline const std::vector<std::string>& getFloatAttribNames() const;

    /// true if more than one wedge arount vertex \a vh, false if only one wedge
    inline bool isFeatureVertex( const VertexHandle& vh ) const;

    /// true if at least one of edge's vertex as two different wedge arount the
    /// edge.
    /// false if the two vertices have the same wedge for both face aside the edge.
    inline bool isFeatureEdge( const EdgeHandle& eh ) const;

    inline const OpenMesh::HPropHandleT<WedgeIndex>& getWedgeIndexPph() const;

    void delete_face( FaceHandle _fh, bool _delete_isolated_vertices = true );

    /**
     * is the vertex a "bow tie" vertex ?
     * \note Alias for OpenMesh::is_manifold
     */
    bool isManifold( VertexHandle vh ) const;

    /// Check if evrything looks right in the data structure
    /// \return true if ok, false if ko.
    bool checkIntegrity() const;

    void triangulate();

    /**
     * Inner class WedgeData represents the actual data per wedge, including position.
     *
     * At any time m_position as to be equal to the wedge's vertex point.
     * All wedges have the same set of attributes.
     * Access and management is delegated to TopologicalMesh and WedgeCollection
     */
    class WedgeData
    {
      public:
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW

        explicit WedgeData() = default;
        inline bool operator==( const WedgeData& lhs ) const;
        inline bool operator!=( const WedgeData& lhs ) const;
        inline bool operator<( const WedgeData& lhs ) const;

        template <typename T>
        inline VectorArray<T>& getAttribArray();
        template <typename T>
        inline const VectorArray<T>& getAttribArray() const;
        friend Wedge;

        //        Index m_inputTriangleMeshIndex;
        //        Index m_outputTriangleMeshIndex;
        VertexHandle m_vertexHandle;
        Vector3 m_position {};
        VectorArray<Scalar> m_floatAttrib;
        VectorArray<Vector2> m_vector2Attrib;
        VectorArray<Vector3> m_vector3Attrib;
        VectorArray<Vector4> m_vector4Attrib;

      private:
        // return 1 : equals, 2: strict less, 3: strict greater
        template <typename T>
        static int compareVector( const T& a, const T& b );
    };

  private:
    // base on openmesh version
    void collapse_edge( HalfedgeHandle, bool );
    void collapse_loop( HalfedgeHandle );

    /**
     * This private class manage wedge data and refcount, to maintain deleted status
     *
     * \internal We need to export this class to make it accessible in .inl
     */
    class RA_CORE_API Wedge
    {
      public:
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW

        explicit Wedge() {}
        explicit Wedge( const WedgeData& wd ) : m_wedgeData { wd }, m_refCount { 1 } {};
        const WedgeData& getWedgeData() const { return m_wedgeData; }
        void setWedgeData( const WedgeData& wedgeData ) { m_wedgeData = wedgeData; }
        void setWedgeData( WedgeData&& wedgeData ) { m_wedgeData = std::move( wedgeData ); }
        void incrementRefCount() { ++m_refCount; }
        void decrementRefCount() {
            if ( m_refCount ) --m_refCount;
        }
        /// comparison ignore refCount
        bool operator==( const Wedge& lhs ) const { return m_wedgeData == lhs.m_wedgeData; }

        bool isDeleted() const { return m_refCount == 0; }
        unsigned int getRefCount() const { return m_refCount; }

        friend WedgeCollection;
        friend TopologicalMesh;

      private:
        WedgeData& getWedgeData() { return m_wedgeData; }

        WedgeData m_wedgeData {};
        unsigned int m_refCount { 0 };
    };

    /**
     * This private class manage the wedge collection.
     * Most of the data members are public so that the enclosing class can
     * easily manage the data.
     *
     * \internal We need to export this class to make it accessible in .inl
     */
    class RA_CORE_API WedgeCollection
    {
      public:
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW

        /**
         * Add wd to the wedge collection, and return the index.
         * If a wedge with same data is already present, it's index is returned,
         * otherwise a new wedge is added to the wedges collection.
         * \param wd Data to insert.
         * \return the index of the inserted (or found) wedge.
         */
        WedgeIndex add( const WedgeData& wd );

        /**
         * Delete the wedge \a idx from the collection.
         * These deletion actually just remove one reference from an halfedge
         * to the wedge data. If the wedge is still referenced by other
         * halfedges, it will not be removed during garbageCollection.
         */
        inline void del( const WedgeIndex& idx );
        WedgeIndex newReference( const WedgeIndex& idx );

        /**
         * Return the wedge data associated with \a idx
         */

        /// Return the wedge (not the data) for in class manipulation.
        /// client code should use getWedgeData only.
        inline const Wedge& getWedge( const WedgeIndex& idx ) const;

        inline const WedgeData& getWedgeData( const WedgeIndex& idx ) const;
        template <typename T>
        inline const T& getWedgeData( const WedgeIndex& idx, const std::string& name ) const;
        template <typename T>
        inline T& getWedgeData( const WedgeIndex& idx, int attribIndex );
        template <typename T>
        inline const T& getWedgeAttrib( const WedgeIndex& idx, const std::string& name ) const;
        template <typename T>
        inline T& getWedgeAttrib( const WedgeIndex& idx, int attribIndex );

        unsigned int getWedgeRefCount( const WedgeIndex& idx ) const;

        /// \see TopologicalMesh::setWedgeData
        inline void setWedgeData( const WedgeIndex& idx, const WedgeData& wd );

        /// change WedgeData member name to value.
        /// wd is moidified accordingly.
        /// \return false if name is not of type T
        /// \retrun true on sucess
        template <typename T>
        inline bool
        setWedgeAttrib( TopologicalMesh::WedgeData& wd, const std::string& name, const T& value );
        template <typename T>
        inline bool
        setWedgeAttrib( const WedgeIndex& idx, const std::string& name, const T& value );
        template <typename T>
        inline void setWedgeAttrib( const TopologicalMesh::WedgeIndex& idx,
                                    const int& attribIndex,
                                    const T& value );

        template <typename T>
        inline int getWedgeAttribIndex( const std::string& name );

        inline bool setWedgePosition( const WedgeIndex& idx, const Vector3& value );

        /// management

        template <typename T>
        inline const std::vector<std::string>& getNameArray() const;

        // name is supposed to be unique within all attribs
        // not checks are performed
        // return the index of the the newly added attrib.
        template <typename T>
        int addAttribName( const std::string& name );

        // add attrib to all wedges with default value value
        template <typename T>
        int addAttrib( const std::string& name, const T& value = {} );

        /// return the offset ot apply to each wedgeindex so that
        /// after garbageCollection all indices are valid and coherent.
        std::vector<int> computeCleanupOffset() const;
        /// \todo removeDuplicateWedge
        /// merge wedges with same data
        /// return old->new index correspondance to update wedgeIndexPph
        /// inline void removeDuplicateWedge

        inline size_t size() const { return m_data.size(); }

        /// remove unreferenced wedge, halfedges need to be reindexed.
        inline void garbageCollection();

        inline void clean();

        // return a new wedgeData with uninit values.
        inline WedgeData newWedgeData() const;
        inline WedgeData newWedgeData( TopologicalMesh::VertexHandle vh,
                                       TopologicalMesh::Point p ) const;

        ///\ todo       private:
        /// attrib names associated to vertex/wedges, getted from CoreMesh, if any,
        std::vector<std::string> m_floatAttribNames;
        std::vector<std::string> m_vector2AttribNames;
        std::vector<std::string> m_vector3AttribNames;
        std::vector<std::string> m_vector4AttribNames;

        /// attrib handle from the CoreMesh given at construction, if any.
        /// used by TopologicalMesh::update()
        std::vector<AttribHandle<Scalar>> m_wedgeFloatAttribHandles;
        std::vector<AttribHandle<Vector2>> m_wedgeVector2AttribHandles;
        std::vector<AttribHandle<Vector3>> m_wedgeVector3AttribHandles;
        std::vector<AttribHandle<Vector4>> m_wedgeVector4AttribHandles;

        template <typename T>
        inline std::vector<std::string>& getNameArray();
        AlignedStdVector<Wedge> m_data;
    };

    // internal function to build Core Mesh attribs correspondance to wedge attribs.
    class InitWedgeAttribsFromMultiIndexedGeometry
    {
      public:
        InitWedgeAttribsFromMultiIndexedGeometry(
            TopologicalMesh* topo,
            const Ra::Core::Geometry::MultiIndexedGeometry& triMesh ) :
            m_topo( topo ), m_triMesh( triMesh ) {}
        void operator()( AttribBase* attr ) const;

      private:
        TopologicalMesh* m_topo;
        const Ra::Core::Geometry::MultiIndexedGeometry& m_triMesh;
    };

    //! [Default command implementation]
    /// \todo add layerKey
    struct DefaultNonManifoldFaceCommand {
        /// \brief details string is printed along with the message
        DefaultNonManifoldFaceCommand( const std::string& details = {} ) : m_details { details } {}
        /// \brief Initalize with input Ra::Core::Geometry::TriangleMesh
        inline void initialize( const Ra::Core::Geometry::MultiIndexedGeometry& ) {}
        /// \brief Process non-manifold face
        inline void process( const std::vector<TopologicalMesh::VertexHandle>& /*face_vhandles*/ ) {
            LOG( logWARNING ) << "Invalid face handle returned : face not added " + m_details;
        }
        /// \brief If needed, apply post-processing on the Ra::Core::Geometry::TopologicalMesh
        inline void postProcess( TopologicalMesh& ) {}
        //! [Default command implementation]
      private:
        std::string m_details;
    };

    WedgeData interpolateWedgeAttributes( const WedgeData&, const WedgeData&, Scalar alpha );

    /// @todo when MultiIndexView will be operational, remove the IndexedGeometry<U> version above
    template <typename T>
    inline void copyAttribToWedgeData( const Ra::Core::Geometry::MultiIndexedGeometry& mesh,
                                       unsigned int vindex,
                                       const std::vector<AttribHandle<T>>& attrHandleVec,
                                       VectorArray<T>* to );

    /// @todo when MultiIndexView will be operational, remove the IndexedGeometry<T> version above
    inline void copyMeshToWedgeData( const Ra::Core::Geometry::MultiIndexedGeometry& mesh,
                                     unsigned int vindex,
                                     const std::vector<AttribHandle<Scalar>>& wprop_float,
                                     const std::vector<AttribHandle<Vector2>>& wprop_vec2,
                                     const std::vector<AttribHandle<Vector3>>& wprop_vec3,
                                     const std::vector<AttribHandle<Vector4>>& wprop_vec4,
                                     TopologicalMesh::WedgeData* wd );

    template <typename T>
    using HandleAndValueVector =
        std::vector<std::pair<AttribHandle<T>, T>,
                    Eigen::aligned_allocator<std::pair<AttribHandle<T>, T>>>;

    void split_copy( EdgeHandle _eh, VertexHandle _vh );
    void split( EdgeHandle _eh, VertexHandle _vh );

    OpenMesh::HPropHandleT<WedgeIndex> m_wedgeIndexPph; /**< Halfedges' Wedge index */
    WedgeCollection m_wedges;                           /**< Wedge data management */

    ///\todo to be deleted/updated
    OpenMesh::HPropHandleT<Index> m_inputTriangleMeshIndexPph;
    OpenMesh::HPropHandleT<Index> m_outputTriangleMeshIndexPph;

    int m_normalsIndex { -1 };
    // vertex handle idx -> face handle idx -> wedge idx with the same normal
    std::vector<std::map<int, std::vector<int>>> m_vertexFaceWedgesWithSameNormals;

    friend class TMOperations;
};

// heplers
void printWedgesInfo( const TopologicalMesh& );

} // namespace Geometry
} // namespace Core
} // namespace Ra

#include <Core/Geometry/TopologicalMesh.inl>
