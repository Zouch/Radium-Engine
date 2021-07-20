#include <Core/Geometry/deprecated/TopologicalMesh.hpp>

#include <Core/RaCore.hpp>
#include <Core/Utils/Log.hpp>

#include <Eigen/StdVector>

#include <unordered_map>
#include <utility>
#include <vector>

namespace Ra {
namespace Core {
namespace Geometry {
namespace deprecated {

using namespace Utils; // log, AttribXXX

///////////////// HELPERS ///////////////

template <typename P, typename T>
void addAttribPairToCore( TriangleMesh& triMesh,
                          const TopologicalMesh* topoMesh,
                          OpenMesh::HPropHandleT<T> oh,
                          std::vector<P>& vprop ) {
    AttribHandle<T> h = triMesh.addAttrib<T>( topoMesh->property( oh ).name() );
    vprop.push_back( std::make_pair( h, oh ) );
}

template <typename T>
using HandleAndValueVector = std::vector<std::pair<AttribHandle<T>, T>,
                                         Eigen::aligned_allocator<std::pair<AttribHandle<T>, T>>>;

template <typename P, typename T>
void copyAttribToCoreVertex( HandleAndValueVector<T>& data,
                             const TopologicalMesh* topoMesh,
                             const std::vector<P>& vprop,
                             TopologicalMesh::HalfedgeHandle heh ) {
    for ( auto pp : vprop ) {
        data.push_back( std::make_pair( pp.first, topoMesh->property( pp.second, heh ) ) );
    }
}

template <typename T>
void copyAttribToCore( TriangleMesh& triMesh, const HandleAndValueVector<T>& data ) {

    for ( auto pp : data ) {
        auto& attr     = triMesh.getAttrib( pp.first );
        auto& attrData = attr.getDataWithLock();
        attrData.push_back( pp.second );
        attr.unlock();
    }
}

//! [Default command implementation]
struct DefaultNonManifoldFaceCommand {
    /// \brief details string is printed along with the message
    DefaultNonManifoldFaceCommand( const std::string& details = {} ) : m_details { details } {}
    /// \brief Initalize with input Ra::Core::Geometry::TriangleMesh
    inline void initialize( const TriangleMesh& /*triMesh*/ ) {}
    /// \brief Process non-manifold face
    inline void process( const std::vector<TopologicalMesh::VertexHandle>& /*face_vhandles*/ ) {
        LOG( logWARNING ) << "Invalid face handle returned : face not added " + m_details;
        /// TODO memorize invalid faces for post processing ...
        ///  see
        ///  https://www.graphics.rwth-aachen.de/media/openflipper_static/Daily-Builds/Doc/Free/Developer/OBJImporter_8cc_source.html
        /// for an exemple of loading
    }
    /// \brief If needed, apply post-processing on the TopologicalMesh
    inline void postProcess( TopologicalMesh& /*tm*/ ) {}
    //! [Default command implementation]
  private:
    std::string m_details;
};

TopologicalMesh::TopologicalMesh( const TriangleMesh& triMesh ) :
    TopologicalMesh( triMesh, DefaultNonManifoldFaceCommand( "[default ctor (props)]" ) ) {}

TopologicalMesh::TopologicalMesh() {
    add_property( m_inputTriangleMeshIndexPph );
}

TriangleMesh TopologicalMesh::toTriangleMesh() {
    struct VertexDataInternal {
        Vector3 _vertex;
        Vector3 _normal;

        HandleAndValueVector<Scalar> _float;
        HandleAndValueVector<Vector2> _vec2;
        HandleAndValueVector<Vector3> _vec3;
        HandleAndValueVector<Vector4> _vec4;

        EIGEN_MAKE_ALIGNED_OPERATOR_NEW

        bool operator==( const VertexDataInternal& lhs ) const {
            return _vertex == lhs._vertex && _normal == lhs._normal && _float == lhs._float &&
                   _vec2 == lhs._vec2 && _vec3 == lhs._vec3 && _vec4 == lhs._vec4;
        }
    };

    struct hash_vec {
        size_t operator()( const VertexDataInternal& lvalue ) const {
            size_t hx = std::hash<Scalar>()( lvalue._vertex[0] );
            size_t hy = std::hash<Scalar>()( lvalue._vertex[1] );
            size_t hz = std::hash<Scalar>()( lvalue._vertex[2] );
            return ( hx ^ ( hy << 1 ) ) ^ hz;
        }
    };

    TriangleMesh out;

    using VertexMap = std::unordered_map<VertexDataInternal, int, hash_vec>;

    VertexMap vertexHandles;

    if ( !get_property_handle( m_outputTriangleMeshIndexPph, "OutputTriangleMeshIndices" ) ) {
        add_property( m_outputTriangleMeshIndexPph, "OutputTriangleMeshIndices" );
    }
    std::vector<PropPair<Scalar>> vprop_float;
    std::vector<PropPair<Vector2>> vprop_vec2;
    std::vector<PropPair<Vector3>> vprop_vec3;
    std::vector<PropPair<Vector4>> vprop_vec4;

    // loop over all attribs and build correspondance pair
    vprop_float.reserve( m_floatPph.size() );
    for ( auto oh : m_floatPph )
        addAttribPairToCore( out, this, oh, vprop_float );
    vprop_vec2.reserve( m_vec2Pph.size() );
    for ( auto oh : m_vec2Pph )
        addAttribPairToCore( out, this, oh, vprop_vec2 );
    vprop_vec3.reserve( m_vec3Pph.size() );
    for ( auto oh : m_vec3Pph )
        addAttribPairToCore( out, this, oh, vprop_vec3 );
    vprop_vec4.reserve( m_vec4Pph.size() );
    for ( auto oh : m_vec4Pph )
        addAttribPairToCore( out, this, oh, vprop_vec4 );

    // iterator over all faces
    unsigned int vertexIndex = 0;

    // out will have at least n_vertices vertices and normals.
    TriangleMesh::PointAttribHandle::Container vertices;
    TriangleMesh::NormalAttribHandle::Container normals;
    TriangleMesh::IndexContainerType indices;

    vertices.reserve( n_vertices() );
    normals.reserve( n_vertices() );
    indices.reserve( n_faces() );

    for ( TopologicalMesh::FaceIter f_it = faces_sbegin(); f_it != faces_end(); ++f_it ) {
        int tindices[3];
        int i = 0;

        // iterator over vertex (through halfedge to get access to halfedge normals)
        for ( TopologicalMesh::ConstFaceHalfedgeIter fh_it = cfh_iter( *f_it ); fh_it.is_valid();
              ++fh_it ) {
            VertexDataInternal v;
            CORE_ASSERT( i < 3, "Non-triangular face found." );
            v._vertex = point( to_vertex_handle( *fh_it ) );

            if ( has_halfedge_normals() ) {
                v._normal = normal( to_vertex_handle( *fh_it ), *f_it );
            }
            copyAttribToCoreVertex( v._float, this, vprop_float, *fh_it );
            copyAttribToCoreVertex( v._vec2, this, vprop_vec2, *fh_it );
            copyAttribToCoreVertex( v._vec3, this, vprop_vec3, *fh_it );
            copyAttribToCoreVertex( v._vec4, this, vprop_vec4, *fh_it );

            int vi;
            VertexMap::iterator vtr = vertexHandles.find( v );
            if ( vtr == vertexHandles.end() ) {
                vi = int( vertexIndex++ );
                vertexHandles.insert( vtr, VertexMap::value_type( v, vi ) );
                vertices.push_back( v._vertex );
                if ( has_halfedge_normals() ) { normals.push_back( v._normal ); }
                copyAttribToCore( out, v._float );
                copyAttribToCore( out, v._vec2 );
                copyAttribToCore( out, v._vec3 );
                copyAttribToCore( out, v._vec4 );
            }
            else {
                vi = vtr->second;
            }
            tindices[i]                                      = vi;
            property( m_outputTriangleMeshIndexPph, *fh_it ) = vi;
            i++;
        }
        indices.emplace_back( tindices[0], tindices[1], tindices[2] );
    }
    out.setVertices( std::move( vertices ) );
    if ( has_halfedge_normals() ) { out.setNormals( std::move( normals ) ); }
    out.setIndices( std::move( indices ) );
    CORE_ASSERT( vertexIndex == vertices.size(),
                 "Inconsistent number of faces in generated TriangleMesh." );

    return out;
}

bool TopologicalMesh::splitEdge( TopologicalMesh::EdgeHandle eh, Scalar f ) {
    // Global schema of operation
    /*
               TRIANGLES ONLY
         before                after
               A                       A
            / F0 \                / F2 | F0 \
           /      \              /     |     \
          /h1    h0\            /h1  e2|e0  h0\
         /    he0   \          /  he2  |  he0  \
        V1 -------- V0       V1 ------ V ------ V0
         \    he1   /          \  he3  |  he1  /
          \o1    o0/            \o1  e3|e1  o0/
           \      /              \     |     /
            \ F1 /                \ F3 | F1 /
               B                       B

    */

    // incorrect factor
    if ( f < 0 || f > 1 ) { return false; }

    // get existing topology data
    HalfedgeHandle he0 = halfedge_handle( eh, 0 );
    HalfedgeHandle he1 = halfedge_handle( eh, 1 );
    VertexHandle v0    = to_vertex_handle( he0 );
    VertexHandle v1    = to_vertex_handle( he1 );
    FaceHandle F0      = face_handle( he0 );
    FaceHandle F1      = face_handle( he1 );

    // not triangles or holes
    if ( ( !is_boundary( he0 ) && valence( F0 ) != 3 ) ||
         ( !is_boundary( he1 ) && valence( F1 ) != 3 ) ) {
        return false;
    }

    // add the new vertex
    const Point p  = Point( f * point( v0 ) + ( Scalar( 1. ) - f ) * point( v1 ) );
    VertexHandle v = add_vertex( p );

    // create the new faces and reconnect the topology
    HalfedgeHandle he3 = new_edge( v, v1 );
    HalfedgeHandle he2 = opposite_halfedge_handle( he3 );
    set_halfedge_handle( v, he0 );
    set_vertex_handle( he1, v );

    // does F0 exist
    if ( !is_boundary( he0 ) ) {
        HalfedgeHandle h0 = next_halfedge_handle( he0 );
        HalfedgeHandle h1 = next_halfedge_handle( h0 );
        // create new edge
        VertexHandle A    = to_vertex_handle( h0 );
        HalfedgeHandle e2 = new_edge( v, A );
        HalfedgeHandle e0 = opposite_halfedge_handle( e2 );
        // split F0
        FaceHandle F2 = new_face();
        set_halfedge_handle( F0, he0 );
        set_halfedge_handle( F2, h1 );
        // update F0
        set_face_handle( h0, F0 );
        set_face_handle( e0, F0 );
        set_face_handle( he0, F0 );
        set_next_halfedge_handle( he0, h0 );
        set_next_halfedge_handle( h0, e0 );
        set_next_halfedge_handle( e0, he0 );
        // update F2
        set_face_handle( h1, F2 );
        set_face_handle( he2, F2 );
        set_face_handle( e2, F2 );
        set_next_halfedge_handle( e2, h1 );
        set_next_halfedge_handle( h1, he2 );
        set_next_halfedge_handle( he2, e2 );
        // deal with custom properties
        // interpolate at he2
        interpolateAllProps( h1, he0, he2, 0.5 );
        // copy at e0, and e2
        copyAllProps( he2, e0 );
    }
    else {
        HalfedgeHandle h1 = prev_halfedge_handle( he0 );
        set_next_halfedge_handle( h1, he2 );
        set_next_halfedge_handle( he2, he0 );
        // next halfedge handle of he0 already is h0
        // halfedge handle of V already is he0
    }

    // does F1 exist
    if ( !is_boundary( he1 ) ) {
        HalfedgeHandle o1 = next_halfedge_handle( he1 );
        HalfedgeHandle o0 = next_halfedge_handle( o1 );
        // create new edge
        VertexHandle B    = to_vertex_handle( o1 );
        HalfedgeHandle e1 = new_edge( v, B );
        HalfedgeHandle e3 = opposite_halfedge_handle( e1 );
        // split F1
        FaceHandle F3 = new_face();
        set_halfedge_handle( F3, o1 );
        set_halfedge_handle( F1, he1 );
        // update F1
        set_face_handle( o1, F3 );
        set_face_handle( e3, F3 );
        set_face_handle( he3, F3 );
        set_next_halfedge_handle( he3, o1 );
        set_next_halfedge_handle( o1, e3 );
        set_next_halfedge_handle( e3, he3 );
        // update F3
        set_face_handle( o0, F1 );
        set_face_handle( he1, F1 );
        set_face_handle( e1, F1 );
        set_next_halfedge_handle( he1, e1 );
        set_next_halfedge_handle( e1, o0 );
        set_next_halfedge_handle( o0, he1 );
        // deal with custom properties
        // first copy at he3
        copyAllProps( he1, he3 );
        // interpolate at he1
        interpolateAllProps( o0, he3, he1, 0.5 );
        // copy at e1, and e3
        copyAllProps( he1, e3 );
        copyAllProps( o1, e1 );
    }
    else {
        HalfedgeHandle o1 = next_halfedge_handle( he1 );
        // next halfedge handle of o0 already is he1
        set_next_halfedge_handle( he1, he3 );
        set_next_halfedge_handle( he3, o1 );
        // halfedge handle of V already is he0
    }

    // ensure consistency at v1
    if ( halfedge_handle( v1 ) == he0 ) { set_halfedge_handle( v1, he2 ); }

    return true;
}

//-----------------------------------------------------------------------------
// from /OpenMesh/Core/Mesh/TriConnectivity.cc
void TopologicalMesh::split( EdgeHandle _eh, VertexHandle _vh ) {
    HalfedgeHandle h0 = halfedge_handle( _eh, 0 );
    HalfedgeHandle o0 = halfedge_handle( _eh, 1 );

    VertexHandle v2 = to_vertex_handle( o0 );

    HalfedgeHandle e1 = new_edge( _vh, v2 );
    HalfedgeHandle t1 = opposite_halfedge_handle( e1 );

    FaceHandle f0 = face_handle( h0 );
    FaceHandle f3 = face_handle( o0 );

    set_halfedge_handle( _vh, h0 );
    set_vertex_handle( o0, _vh );

    if ( !is_boundary( h0 ) ) {
        HalfedgeHandle h1 = next_halfedge_handle( h0 );
        HalfedgeHandle h2 = next_halfedge_handle( h1 );

        VertexHandle v1 = to_vertex_handle( h1 );

        HalfedgeHandle e0 = new_edge( _vh, v1 );
        HalfedgeHandle t0 = opposite_halfedge_handle( e0 );

        FaceHandle f1 = new_face();
        set_halfedge_handle( f0, h0 );
        set_halfedge_handle( f1, h2 );

        set_face_handle( h1, f0 );
        set_face_handle( t0, f0 );
        set_face_handle( h0, f0 );

        set_face_handle( h2, f1 );
        set_face_handle( t1, f1 );
        set_face_handle( e0, f1 );

        set_next_halfedge_handle( h0, h1 );
        set_next_halfedge_handle( h1, t0 );
        set_next_halfedge_handle( t0, h0 );

        set_next_halfedge_handle( e0, h2 );
        set_next_halfedge_handle( h2, t1 );
        set_next_halfedge_handle( t1, e0 );
    }
    else {
        set_next_halfedge_handle( prev_halfedge_handle( h0 ), t1 );
        set_next_halfedge_handle( t1, h0 );
        // halfedge handle of _vh already is h0
    }

    if ( !is_boundary( o0 ) ) {
        HalfedgeHandle o1 = next_halfedge_handle( o0 );
        HalfedgeHandle o2 = next_halfedge_handle( o1 );

        VertexHandle v3 = to_vertex_handle( o1 );

        HalfedgeHandle e2 = new_edge( _vh, v3 );
        HalfedgeHandle t2 = opposite_halfedge_handle( e2 );

        FaceHandle f2 = new_face();
        set_halfedge_handle( f2, o1 );
        set_halfedge_handle( f3, o0 );

        set_face_handle( o1, f2 );
        set_face_handle( t2, f2 );
        set_face_handle( e1, f2 );

        set_face_handle( o2, f3 );
        set_face_handle( o0, f3 );
        set_face_handle( e2, f3 );

        set_next_halfedge_handle( e1, o1 );
        set_next_halfedge_handle( o1, t2 );
        set_next_halfedge_handle( t2, e1 );

        set_next_halfedge_handle( o0, e2 );
        set_next_halfedge_handle( e2, o2 );
        set_next_halfedge_handle( o2, o0 );
    }
    else {
        set_next_halfedge_handle( e1, next_halfedge_handle( o0 ) );
        set_next_halfedge_handle( o0, e1 );
        set_halfedge_handle( _vh, e1 );
    }

    if ( halfedge_handle( v2 ) == h0 ) set_halfedge_handle( v2, t1 );
}

//-----------------------------------------------------------------------------

void TopologicalMesh::split_copy( EdgeHandle _eh, VertexHandle _vh ) {
    const VertexHandle v0 = to_vertex_handle( halfedge_handle( _eh, 0 ) );
    const VertexHandle v1 = to_vertex_handle( halfedge_handle( _eh, 1 ) );

    const int nf = n_faces();

    // Split the halfedge ( handle will be preserved)
    split( _eh, _vh );

    // Copy the properties of the original edge to all neighbor edges that
    // have been created
    for ( VEIter ve_it = ve_iter( _vh ); ve_it.is_valid(); ++ve_it )
        copy_all_properties( _eh, *ve_it, true );

    for ( auto vh : { v0, v1 } ) {
        // get the halfedge pointing from new vertex to old vertex
        const HalfedgeHandle h = find_halfedge( _vh, vh );
        // for boundaries there are no faces whose properties need to be copied
        if ( !is_boundary( h ) ) {
            FaceHandle fh0 = face_handle( h );
            FaceHandle fh1 = face_handle( opposite_halfedge_handle( prev_halfedge_handle( h ) ) );
            // is fh0 the new face?
            if ( fh0.idx() >= nf ) std::swap( fh0, fh1 );

            // copy properties from old face to new face
            copy_all_properties( fh0, fh1, true );
        }
    }
}

} // namespace deprecated
} // namespace Geometry
} // namespace Core
} // namespace Ra
