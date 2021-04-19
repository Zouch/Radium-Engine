#include <Core/Geometry/TopologicalMesh.hpp>

#include <Core/RaCore.hpp>
#include <Core/Utils/Log.hpp>

#include <Eigen/StdVector>

#include <utility>
#include <vector>

namespace Ra {
namespace Core {
namespace Geometry {

using namespace Utils; // log, AttribXXX

///////////////// HELPERS ///////////////

std::string wedgeInfo( const Ra::Core::Geometry::TopologicalMesh& topo,
                       const Ra::Core::Geometry::TopologicalMesh::WedgeIndex& idx ) {

    std::stringstream buffer;
    if ( !idx.isValid() )
    {
        buffer << "wedge (invalid) ";
        return buffer.str();
    }

    const Ra::Core::Geometry::TopologicalMesh::WedgeData& wd = topo.getWedgeData( idx );

    buffer << "wedge (" << idx << "," << topo.getWedgeRefCount( idx ) << "), ";
    auto& floatAttrNames = topo.getFloatAttribNames();
    for ( size_t i = 0; i < floatAttrNames.size(); ++i )
    {
        buffer << floatAttrNames[i];
        buffer << "[";
        buffer << wd.m_floatAttrib[i];
        buffer << "], ";
    }
    auto vec2AttrNames = topo.getVec2AttribNames();
    for ( size_t i = 0; i < vec2AttrNames.size(); ++i )
    {
        buffer << vec2AttrNames[i];
        buffer << "[";
        buffer << wd.m_vector2Attrib[i].transpose();
        buffer << "], ";
    }
    auto vec3AttrNames = topo.getVec3AttribNames();
    for ( size_t i = 0; i < vec3AttrNames.size(); ++i )
    {
        buffer << vec3AttrNames[i];
        buffer << "[";
        buffer << wd.m_vector3Attrib[i].transpose();
        buffer << "], ";
    }

    auto vec4AttrNames = topo.getVec4AttribNames();
    for ( size_t i = 0; i < vec4AttrNames.size(); ++i )
    {
        buffer << vec4AttrNames[i];
        buffer << "[";
        buffer << wd.m_vector4Attrib[i].transpose();
        buffer << "], ";
    }

    return buffer.str();
}

bool TopologicalMesh::isManifold( VertexHandle vh ) const {
    return is_manifold( vh );
}

bool TopologicalMesh::checkIntegrity() const {
    std::vector<unsigned int> count( m_wedges.size(), 0 );
    bool ret = true;
    for ( auto he_itr {halfedges_begin()}; he_itr != halfedges_end(); ++he_itr )
    {
        auto widx = property( m_wedgeIndexPph, *he_itr );

        if ( status( *he_itr ).deleted() ) { continue; }

        if ( is_boundary( *he_itr ) != widx.isInvalid() )
        {
            LOG( logWARNING ) << "topological mesh wedge inconsistency, boundary he ("
                              << ( is_boundary( *he_itr ) ? "true," : "false," ) << he_itr->idx()
                              << ") != invalid Wedge (" << ( widx.isInvalid() ? "true," : "false," )
                              << widx << ") ref "
                              << ( widx.isValid() ? m_wedges.getWedge( widx ).getRefCount() : 0 );
            ret = false;
        }

        if ( widx.isValid() ) // i.e. non boudnary
        {
            count[widx]++;

            if ( m_wedges.getWedgeData( widx ).m_position != point( to_vertex_handle( *he_itr ) ) )
            {
                LOG( logWARNING ) << "topological mesh wedge inconsistency, wedge and to position "
                                     "differ for widx "
                                  << widx << ", have ("
                                  << m_wedges.getWedgeData( widx ).m_position.transpose()
                                  << ") instead of ("
                                  << point( to_vertex_handle( *he_itr ) ).transpose() << ")";
                ret = false;
            }
        }
    }

    for ( int widx = 0; widx < int( m_wedges.size() ); ++widx )
    {
        if ( m_wedges.getWedge( WedgeIndex {widx} ).getRefCount() != count[widx] )
        {
            LOG( logWARNING ) << "topological mesh wedge count inconsistency, topo count [ "
                              << count[widx] << " ] != wedge count [ "
                              << m_wedges.getWedge( WedgeIndex {widx} ).getRefCount()
                              << " ] for id " << widx;
            ret = false;
        }
    }
    return ret;
}

void TopologicalMesh::triangulate() {

    auto fix = [this]( HalfedgeHandle next_he, const std::vector<HalfedgeHandle>& old_heh ) {
        // tagged if already fixed
        auto to_vh = to_vertex_handle( next_he );
        // find ref in old_he to copy wedge idx

        auto ref = std::find_if(
            old_heh.begin(), old_heh.end(), [this, to_vh]( const HalfedgeHandle& he ) {
                return to_vertex_handle( he ) == to_vh;
            } );
        if ( ref != old_heh.end() )
        {
            property( m_wedgeIndexPph, next_he ) =
                m_wedges.newReference( property( m_wedgeIndexPph, *ref ) );
        }
        else
        { LOG( logERROR ) << "triangulate::fix reference halfedge not found"; }
        status( next_he ).set_tagged( true );
    };

    FaceIter f_it( faces_begin() ), f_end( faces_end() );
    for ( ; f_it != f_end; ++f_it )
    {
        // save original halfedge of the face
        std::vector<HalfedgeHandle> old_heh;
        ConstFaceHalfedgeIter fh_itr = cfh_iter( *f_it );
        for ( ; fh_itr.is_valid(); ++fh_itr )
        {
            old_heh.push_back( *fh_itr );
        }
        auto size = old_heh.size();
        //   if ( size <= 3 ) continue;

        // base openmesh triangulate
        base::triangulate( *f_it );

        // fix newly created he
        for ( size_t i = 0; i < size; ++i )
        {
            auto next_he = next_halfedge_handle( old_heh[i] );
            // if next_he is not the same as next in old_heh, then it's a new one.
            // fix tag halfedge so that it is not fixed two times (in case opposite halfedge is also
            // parsed in this loop.
            if ( !status( next_he ).tagged() && next_he != old_heh[( i + 1 ) % size] )
            {
                fix( next_he, old_heh );
                fix( opposite_halfedge_handle( next_he ), old_heh );
            }
        }
    }
    // untag everything
    for ( auto& he : halfedges() )
    {
        status( he ).set_tagged( false );
    }
}

void printWedgesInfo( const Ra::Core::Geometry::TopologicalMesh& topo ) {
    using namespace Ra::Core;

    for ( auto itr = topo.vertices_sbegin(); itr != topo.vertices_end(); ++itr )
    {
        LOG( Utils::logINFO ) << "vertex " << *itr;
        auto wedges = topo.getVertexWedges( *itr );
        for ( auto wedgeIndex : wedges )
        {
            LOG( Utils::logINFO ) << wedgeInfo( topo, wedgeIndex );
        }
    }

    for ( auto itr = topo.halfedges_sbegin(); itr != topo.halfedges_end(); ++itr )
    {
        LOG( Utils::logINFO ) << "he " << *itr
                              << ( topo.is_boundary( *itr ) ? " boundary " : " inner " );
        LOG( Utils::logINFO ) << wedgeInfo( topo, topo.property( topo.getWedgeIndexPph(), *itr ) );
    }
}

TopologicalMesh::TopologicalMesh() {
    add_property( m_inputTriangleMeshIndexPph );
    add_property( m_wedgeIndexPph );
}

template <typename T>
void copyWedgeDataToAttribContainer( AlignedStdVector<typename Attrib<T>::Container>& c,
                                     const VectorArray<T>& wd ) {
    for ( size_t i = 0; i < wd.size(); ++i )
    {
        c[i].push_back( wd[i] );
    }
}

template <typename T, typename U>
void moveContainerToMesh( IndexedGeometry<U>& out,
                          const std::vector<std::string>& names,
                          AlignedStdVector<typename Attrib<T>::Container>& wedgeAttribData ) {
    for ( size_t i = 0; i < wedgeAttribData.size(); ++i )
    {
        auto attrHandle = out.template addAttrib<T>( names[i] );
        out.getAttrib( attrHandle ).setData( std::move( wedgeAttribData[i] ) );
    }
}

TriangleMesh TopologicalMesh::toTriangleMesh() {
    // first cleanup deleted element
    garbage_collection();

    TriangleMesh out;
    TriangleMesh::IndexContainerType indices;

    /// add attribs to out
    TriangleMesh::PointAttribHandle::Container wedgePosition;
    AlignedStdVector<Attrib<float>::Container> wedgeFloatAttribData(
        m_wedges.m_floatAttribNames.size() );
    AlignedStdVector<Attrib<Vector2>::Container> wedgeVector2AttribData(
        m_wedges.m_vector2AttribNames.size() );
    AlignedStdVector<Attrib<Vector3>::Container> wedgeVector3AttribData(
        m_wedges.m_vector3AttribNames.size() );
    AlignedStdVector<Attrib<Vector4>::Container> wedgeVector4AttribData(
        m_wedges.m_vector4AttribNames.size() );

    /// Wedges are output vertices !
    for ( WedgeIndex widx {0}; widx < WedgeIndex {m_wedges.size()}; ++widx )
    {
        const auto& wd = m_wedges.getWedgeData( widx );
        wedgePosition.push_back( wd.m_position );
        copyWedgeDataToAttribContainer( wedgeFloatAttribData, wd.m_floatAttrib );
        copyWedgeDataToAttribContainer( wedgeVector2AttribData, wd.m_vector2Attrib );
        copyWedgeDataToAttribContainer( wedgeVector3AttribData, wd.m_vector3Attrib );
        copyWedgeDataToAttribContainer( wedgeVector4AttribData, wd.m_vector4Attrib );
    }

    out.setVertices( std::move( wedgePosition ) );
    moveContainerToMesh<float>( out, m_wedges.m_floatAttribNames, wedgeFloatAttribData );
    moveContainerToMesh<Vector2>( out, m_wedges.m_vector2AttribNames, wedgeVector2AttribData );
    moveContainerToMesh<Vector3>( out, m_wedges.m_vector3AttribNames, wedgeVector3AttribData );
    moveContainerToMesh<Vector4>( out, m_wedges.m_vector4AttribNames, wedgeVector4AttribData );

    for ( TopologicalMesh::FaceIter f_it = faces_sbegin(); f_it != faces_end(); ++f_it )
    {
        int tindices[3];
        int i = 0;

        for ( TopologicalMesh::ConstFaceHalfedgeIter fh_it = cfh_iter( *f_it ); fh_it.is_valid();
              ++fh_it )
        {
            CORE_ASSERT( i < 3, "Non-triangular face found." );
            tindices[i] = property( m_wedgeIndexPph, *fh_it );
            i++;
        }
        indices.emplace_back( tindices[0], tindices[1], tindices[2] );
    }

    out.setIndices( std::move( indices ) );

    return out;
}

LineMesh TopologicalMesh::toLineMesh() {
    // first cleanup deleted element
    garbage_collection();

    LineMesh out;
    LineMesh::IndexContainerType indices;

    TriangleMesh::PointAttribHandle::Container wedgePosition;
    AlignedStdVector<Attrib<float>::Container> wedgeFloatAttribData(
        m_wedges.m_floatAttribNames.size() );
    AlignedStdVector<Attrib<Vector2>::Container> wedgeVector2AttribData(
        m_wedges.m_vector2AttribNames.size() );
    AlignedStdVector<Attrib<Vector3>::Container> wedgeVector3AttribData(
        m_wedges.m_vector3AttribNames.size() );
    AlignedStdVector<Attrib<Vector4>::Container> wedgeVector4AttribData(
        m_wedges.m_vector4AttribNames.size() );

    /// Wedges are output vertices !
    for ( WedgeIndex widx {0}; widx < WedgeIndex( m_wedges.size() ); ++widx )
    {
        const auto& wd = m_wedges.getWedgeData( widx );
        wedgePosition.push_back( wd.m_position );
        copyWedgeDataToAttribContainer( wedgeFloatAttribData, wd.m_floatAttrib );
        copyWedgeDataToAttribContainer( wedgeVector2AttribData, wd.m_vector2Attrib );
        copyWedgeDataToAttribContainer( wedgeVector3AttribData, wd.m_vector3Attrib );
        copyWedgeDataToAttribContainer( wedgeVector4AttribData, wd.m_vector4Attrib );
    }

    out.setVertices( std::move( wedgePosition ) );
    moveContainerToMesh<float>( out, m_wedges.m_floatAttribNames, wedgeFloatAttribData );
    moveContainerToMesh<Vector2>( out, m_wedges.m_vector2AttribNames, wedgeVector2AttribData );
    moveContainerToMesh<Vector3>( out, m_wedges.m_vector3AttribNames, wedgeVector3AttribData );
    moveContainerToMesh<Vector4>( out, m_wedges.m_vector4AttribNames, wedgeVector4AttribData );

    for ( TopologicalMesh::EdgeIter e_it = edges_sbegin(); e_it != edges_end(); ++e_it )
    {
        int tindices[2];

        // take care of boundaries
        auto he0 = halfedge_handle( *e_it, 0 );
        if ( OpenMesh::ArrayKernel::is_boundary( he0 ) )
        { he0 = prev_halfedge_handle( opposite_halfedge_handle( he0 ) ); }
        if ( OpenMesh::ArrayKernel::is_boundary( he0 ) ) continue;
        auto he1 = halfedge_handle( *e_it, 1 );
        if ( OpenMesh::ArrayKernel::is_boundary( he1 ) )
        { he1 = prev_halfedge_handle( opposite_halfedge_handle( he1 ) ); }
        if ( OpenMesh::ArrayKernel::is_boundary( he1 ) ) continue;

        tindices[0] = property( m_wedgeIndexPph, he0 );
        tindices[1] = property( m_wedgeIndexPph, he1 );

        indices.emplace_back( tindices[0], tindices[1] );
    }

    out.setIndices( std::move( indices ) );

    return out;
}
PolyMesh TopologicalMesh::toPolyMesh() {
    // first cleanup deleted element
    garbage_collection();

    PolyMesh out;
    PolyMesh::IndexContainerType indices;

    /// add attribs to out
    std::vector<AttribHandle<float>> wedgeFloatAttribHandles;
    std::vector<AttribHandle<Vector2>> wedgeVector2AttribHandles;
    std::vector<AttribHandle<Vector3>> wedgeVector3AttribHandles;
    std::vector<AttribHandle<Vector4>> wedgeVector4AttribHandles;

    TriangleMesh::PointAttribHandle::Container wedgePosition;
    AlignedStdVector<Attrib<float>::Container> wedgeFloatAttribData(
        m_wedges.m_floatAttribNames.size() );
    AlignedStdVector<Attrib<Vector2>::Container> wedgeVector2AttribData(
        m_wedges.m_vector2AttribNames.size() );
    AlignedStdVector<Attrib<Vector3>::Container> wedgeVector3AttribData(
        m_wedges.m_vector3AttribNames.size() );
    AlignedStdVector<Attrib<Vector4>::Container> wedgeVector4AttribData(
        m_wedges.m_vector4AttribNames.size() );

    /// Wedges are output vertices !
    for ( WedgeIndex widx {0}; widx < WedgeIndex( m_wedges.size() ); ++widx )
    {
        const auto& wd = m_wedges.getWedgeData( widx );
        wedgePosition.push_back( wd.m_position );
        copyWedgeDataToAttribContainer( wedgeFloatAttribData, wd.m_floatAttrib );
        copyWedgeDataToAttribContainer( wedgeVector2AttribData, wd.m_vector2Attrib );
        copyWedgeDataToAttribContainer( wedgeVector3AttribData, wd.m_vector3Attrib );
        copyWedgeDataToAttribContainer( wedgeVector4AttribData, wd.m_vector4Attrib );
    }

    out.setVertices( std::move( wedgePosition ) );
    moveContainerToMesh<float>( out, m_wedges.m_floatAttribNames, wedgeFloatAttribData );
    moveContainerToMesh<Vector2>( out, m_wedges.m_vector2AttribNames, wedgeVector2AttribData );
    moveContainerToMesh<Vector3>( out, m_wedges.m_vector3AttribNames, wedgeVector3AttribData );
    moveContainerToMesh<Vector4>( out, m_wedges.m_vector4AttribNames, wedgeVector4AttribData );

    for ( TopologicalMesh::FaceIter f_it = faces_sbegin(); f_it != faces_end(); ++f_it )
    {
        int i = 0;
        PolyMesh::IndexType faceIndices( valence( *f_it ) );
        // iterator over vertex (through halfedge to get access to halfedge normals)
        for ( TopologicalMesh::ConstFaceHalfedgeIter fh_it = cfh_iter( *f_it ); fh_it.is_valid();
              ++fh_it )
        {
            faceIndices( i ) = property( m_wedgeIndexPph, *fh_it );
            i++;
        }
        // LOG( logDEBUG ) << "add polymesh face " << faceIndices.transpose();
        indices.push_back( faceIndices );
    }

    out.setIndices( std::move( indices ) );

    return out;
}

void TopologicalMesh::updateTriangleMesh( Ra::Core::Geometry::TriangleMesh& out ) {
    TriangleMesh::PointAttribHandle::Container wedgePosition;
    AlignedStdVector<Attrib<float>::Container> wedgeFloatAttribData(
        m_wedges.m_floatAttribNames.size() );
    AlignedStdVector<Attrib<Vector2>::Container> wedgeVector2AttribData(
        m_wedges.m_vector2AttribNames.size() );
    AlignedStdVector<Attrib<Vector3>::Container> wedgeVector3AttribData(
        m_wedges.m_vector3AttribNames.size() );
    AlignedStdVector<Attrib<Vector4>::Container> wedgeVector4AttribData(
        m_wedges.m_vector4AttribNames.size() );

    /// Wedges are output vertices !
    for ( WedgeIndex widx {0}; widx < WedgeIndex( m_wedges.size() ); ++widx )
    {
        const auto& wd = m_wedges.getWedgeData( widx );
        wedgePosition.push_back( wd.m_position );
        copyWedgeDataToAttribContainer( wedgeFloatAttribData, wd.m_floatAttrib );
        copyWedgeDataToAttribContainer( wedgeVector2AttribData, wd.m_vector2Attrib );
        copyWedgeDataToAttribContainer( wedgeVector3AttribData, wd.m_vector3Attrib );
        copyWedgeDataToAttribContainer( wedgeVector4AttribData, wd.m_vector4Attrib );
    }

    out.setVertices( std::move( wedgePosition ) );
    moveContainerToMesh<float>( out, m_wedges.m_floatAttribNames, wedgeFloatAttribData );
    moveContainerToMesh<Vector2>( out, m_wedges.m_vector2AttribNames, wedgeVector2AttribData );
    moveContainerToMesh<Vector3>( out, m_wedges.m_vector3AttribNames, wedgeVector3AttribData );
    moveContainerToMesh<Vector4>( out, m_wedges.m_vector4AttribNames, wedgeVector4AttribData );
}

void TopologicalMesh::updateTriangleMeshNormals(
    AttribArrayGeometry::NormalAttribHandle::Container& normals ) {
    if ( !has_halfedge_normals() )
    {
        LOG( logERROR ) << "TopologicalMesh has no normals, nothing set";
        return;
    }

    for ( unsigned int widx = 0; widx < m_wedges.size(); ++widx )
    {
        normals[widx] = m_wedges.getWedgeData<Normal>( widx, m_normalsIndex );
    }
}

void TopologicalMesh::updateTriangleMeshNormals( Ra::Core::Geometry::TriangleMesh& out ) {
    auto& normals = out.normalsWithLock();
    updateTriangleMeshNormals( normals );
    out.normalsUnlock();
}

void TopologicalMesh::update( const Ra::Core::Geometry::TriangleMesh& triMesh ) {
    for ( size_t i = 0; i < triMesh.vertices().size(); ++i )
    {
        WedgeData wd;
        wd.m_position = triMesh.vertices()[i];
        copyMeshToWedgeData( triMesh,
                             i,
                             m_wedges.m_wedgeFloatAttribHandles,
                             m_wedges.m_wedgeVector2AttribHandles,
                             m_wedges.m_wedgeVector3AttribHandles,
                             m_wedges.m_wedgeVector4AttribHandles,
                             &wd );
        m_wedges.setWedgeData( i, wd );
    }
    // update positions
    for ( auto itr = halfedges_begin(), stop = halfedges_end(); itr != stop; ++itr )
    {
        point( to_vertex_handle( *itr ) ) =
            m_wedges.getWedgeData( getWedgeIndex( *itr ) ).m_position;
    }
}

void TopologicalMesh::updatePositions( const Ra::Core::Geometry::TriangleMesh& triMesh ) {
    updatePositions( triMesh.vertices() );
}

void TopologicalMesh::updatePositions(
    const AttribArrayGeometry::PointAttribHandle::Container& vertices ) {

    for ( size_t i = 0; i < vertices.size(); ++i )
    {
        m_wedges.m_data[i].getWedgeData().m_position              = vertices[i];
        point( m_wedges.m_data[i].getWedgeData().m_vertexHandle ) = vertices[i];
    }
}

void TopologicalMesh::updateNormals( const Ra::Core::Geometry::TriangleMesh& triMesh ) {
    auto& normals = triMesh.normals();

    for ( size_t i = 0; i < triMesh.vertices().size(); ++i )
    {
        m_wedges.setWedgeAttrib<Normal>( i, m_normalsIndex, normals[i] );
    }
}

template <typename T>
void interpolate( const VectorArray<T>& in1,
                  const VectorArray<T>& in2,
                  VectorArray<T>& out,
                  const Scalar alpha ) {
    for ( size_t i = 0; i < in1.size(); ++i )
    {
        out.push_back( ( 1_ra - alpha ) * in1[i] + alpha * in2[i] );
    }
}

TopologicalMesh::WedgeData
TopologicalMesh::interpolateWedgeAttributes( const TopologicalMesh::WedgeData& w1,
                                             const TopologicalMesh::WedgeData& w2,
                                             Scalar alpha ) {
    WedgeData ret;
    interpolate( w1.m_floatAttrib, w2.m_floatAttrib, ret.m_floatAttrib, alpha );
    interpolate( w1.m_vector2Attrib, w2.m_vector2Attrib, ret.m_vector2Attrib, alpha );
    interpolate( w1.m_vector3Attrib, w2.m_vector3Attrib, ret.m_vector3Attrib, alpha );
    interpolate( w1.m_vector4Attrib, w2.m_vector4Attrib, ret.m_vector4Attrib, alpha );
    return ret;
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

    if ( !is_boundary( h0 ) )
    {
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
    else
    {
        set_next_halfedge_handle( prev_halfedge_handle( h0 ), t1 );
        set_next_halfedge_handle( t1, h0 );
        // halfedge handle of _vh already is h0
    }

    if ( !is_boundary( o0 ) )
    {
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
    else
    {
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

    for ( auto vh : {v0, v1} )
    {
        // get the halfedge pointing from new vertex to old vertex
        const HalfedgeHandle h = find_halfedge( _vh, vh );
        // for boundaries there are no faces whose properties need to be copied
        if ( !is_boundary( h ) )
        {
            FaceHandle fh0 = face_handle( h );
            FaceHandle fh1 = face_handle( opposite_halfedge_handle( prev_halfedge_handle( h ) ) );
            // is fh0 the new face?
            if ( fh0.idx() >= nf ) std::swap( fh0, fh1 );

            // copy properties from old face to new face
            copy_all_properties( fh0, fh1, true );
        }
    }
}

bool TopologicalMesh::splitEdge( TopologicalMesh::EdgeHandle eh, Scalar f ) {
    // Global schema of operation
    /*
               TRIANGLES ONLY
         before                after

            /    \                /    |    \
           /      \              /     |     \
          /h2    h1\            /r2  r1|s2  s1\
         /    h0 -->\          /  r0   | s0=h0 \
        V0 -------- V1        V0-(1-f)-V---f---V1
         \<-- o0    /          \  u0   | t0=o0 /
          \o1    o2/            \u1  u2|t1  t2/
           \      /              \     |     /
            \    /                \    |    /


    */

    // incorrect factor
    if ( f < 0 || f > 1 ) { return false; }

    const auto h0 = halfedge_handle( eh, 0 );
    const auto o0 = halfedge_handle( eh, 1 );

    const auto v0 = to_vertex_handle( o0 );
    const auto v1 = to_vertex_handle( h0 );

    const auto h1 = next_halfedge_handle( h0 );
    const auto h2 = next_halfedge_handle( h1 );
    const auto o1 = next_halfedge_handle( o0 );
    const auto o2 = next_halfedge_handle( o1 );

    // add the new point
    const Point p   = Point( f * point( v1 ) + ( 1_ra - f ) * point( v0 ) );
    VertexHandle vh = add_vertex( p );

    // compute interpolated wedge, or the two wedges if not the same wedges
    // around the two vertices of the edge (we always compute for two wedges,
    // even if add will return the same wedge.
    WedgeIndex ovwidx;
    WedgeIndex hvwidx;
    if ( !is_boundary( h0 ) )
    {

        const auto hw0idx = property( m_wedgeIndexPph, h2 );
        const auto hw1idx = property( m_wedgeIndexPph, h0 );
        const auto hw0    = m_wedges.getWedgeData( hw0idx );
        const auto hw1    = m_wedges.getWedgeData( hw1idx );
        auto hvw          = interpolateWedgeAttributes( hw0, hw1, f );
        hvw.m_position    = p;
        hvwidx            = m_wedges.add( hvw );
    }
    if ( !is_boundary( o0 ) )
    {

        const auto ow0idx = property( m_wedgeIndexPph, o2 );
        const auto ow1idx = property( m_wedgeIndexPph, o0 );
        const auto ow0    = m_wedges.getWedgeData( ow0idx );
        const auto ow1    = m_wedges.getWedgeData( ow1idx );
        auto ovw          = interpolateWedgeAttributes( ow1, ow0, f );
        ovw.m_position    = p;
        ovwidx            = m_wedges.add( ovw );
    }

    split_copy( eh, vh );

    auto r0 = find_halfedge( v0, vh );
    auto s0 = find_halfedge( vh, v1 );
    auto t0 = find_halfedge( v1, vh );
    auto u0 = find_halfedge( vh, v0 );

    auto r1 = next_halfedge_handle( r0 );
    auto r2 = next_halfedge_handle( r1 );

    auto s1 = next_halfedge_handle( s0 );
    auto s2 = next_halfedge_handle( s1 );

    auto t1 = next_halfedge_handle( t0 );
    auto t2 = next_halfedge_handle( t1 );

    auto u1 = next_halfedge_handle( u0 );
    auto u2 = next_halfedge_handle( u1 );

    CORE_ASSERT( s0 == h0, "TopologicalMesh: splitEdgeWedge inconsistency" );
    CORE_ASSERT( t0 == o0, "TopologicalMesh: splitEdgeWedge inconsistency" );

    auto updateWedgeIndex1 = [this]( WedgeIndex widx_,
                                     HalfedgeHandle r0_,
                                     HalfedgeHandle r1_,
                                     HalfedgeHandle r2_,
                                     HalfedgeHandle h1_,
                                     HalfedgeHandle h2_ ) {
        CORE_UNUSED( r2_ );
        CORE_UNUSED( h2_ );

        if ( !is_boundary( r0_ ) )
        {
            CORE_ASSERT( r2_ == h2_, "TopologicalMesh: splitEdgeWedge inconsistency" );

            // Increment here, the first reference is for the other he
            property( this->m_wedgeIndexPph, r0_ ) = this->m_wedges.newReference( widx_ );
            property( this->m_wedgeIndexPph, r1_ ) =
                this->m_wedges.newReference( property( this->m_wedgeIndexPph, h1_ ) );
        }
        else
        { property( this->m_wedgeIndexPph, r0_ ) = WedgeIndex {}; }
    };

    auto updateWedgeIndex2 = [this]( WedgeIndex widx_,
                                     HalfedgeHandle s0_,
                                     HalfedgeHandle s1_,
                                     HalfedgeHandle s2_,
                                     HalfedgeHandle h0_,
                                     HalfedgeHandle h1_ ) {
        CORE_UNUSED( s1_ );
        CORE_UNUSED( h1_ );

        if ( !is_boundary( s0_ ) )
        {
            CORE_ASSERT( s1_ == h1_, "TopologicalMesh: splitEdgeWedge inconsistency" );
            // do not increment here, since add has set ref to 1
            property( this->m_wedgeIndexPph, s2_ ) = widx_;
            // "steal" ref from previous he (actually s0 is h0, u0 is the steal
            // from o0.
            property( this->m_wedgeIndexPph, s0_ ) = property( this->m_wedgeIndexPph, h0_ );
        }
        else
        { property( this->m_wedgeIndexPph, s0_ ) = WedgeIndex {}; }
    };

    // this update read from o0, must be done before t which reads from o0
    updateWedgeIndex2( ovwidx, u0, u1, u2, o0, o1 );
    // those update might be in any order
    updateWedgeIndex2( hvwidx, s0, s1, s2, h0, h1 );
    updateWedgeIndex1( hvwidx, r0, r1, r2, h1, h2 );
    updateWedgeIndex1( ovwidx, t0, t1, t2, o1, o2 );

    return true;
}

//-----------------------------------------------------------------------------
void TopologicalMesh::collapse( HalfedgeHandle _hh, bool keepFrom ) {
    HalfedgeHandle h0 = _hh;
    HalfedgeHandle h1 = next_halfedge_handle( h0 );
    HalfedgeHandle o0 = opposite_halfedge_handle( h0 );
    HalfedgeHandle o1 = next_halfedge_handle( o0 );

    // remove edge
    collapse_edge( h0, keepFrom );

    // remove loops
    if ( next_halfedge_handle( next_halfedge_handle( h1 ) ) == h1 )
        collapse_loop( next_halfedge_handle( h1 ) );
    if ( next_halfedge_handle( next_halfedge_handle( o1 ) ) == o1 ) collapse_loop( o1 );
}

//-----------------------------------------------------------------------------
void TopologicalMesh::collapse_edge( HalfedgeHandle _hh, bool keepFrom ) {
    HalfedgeHandle h  = _hh;
    HalfedgeHandle hn = next_halfedge_handle( h );
    HalfedgeHandle hp = prev_halfedge_handle( h );

    HalfedgeHandle o   = opposite_halfedge_handle( h );
    HalfedgeHandle on  = next_halfedge_handle( o );
    HalfedgeHandle ono = opposite_halfedge_handle( on );
    HalfedgeHandle op  = prev_halfedge_handle( o );

    FaceHandle fh = face_handle( h );
    FaceHandle fo = face_handle( o );

    VertexHandle vh = to_vertex_handle( h );
    VertexHandle vo = to_vertex_handle( o );

    auto widx = getWedgeIndex( h );
    if ( widx.isInvalid() ) // i.e. h is boundary
        widx = getWedgeIndex( op );
    auto otherWidx = getWedgeIndex( op );
    if ( otherWidx.isInvalid() ) // i.e. h is boundary
        otherWidx = getWedgeIndex( h );

    // halfedge -> vertex

    // manual iter for from fixup
    auto currentWidx     = widx;
    auto ringWidx        = WedgeIndex {};
    int phase            = 0;
    HalfedgeHandle start = prev_halfedge_handle( opposite_halfedge_handle( hp ) );
    HalfedgeHandle vih   = start;
    do
    {
        set_vertex_handle( vih, vh );
        if ( !is_boundary( vih ) )
        {
            if ( !keepFrom )
            {
                if ( phase == 0 )
                {
                    CORE_ASSERT( ringWidx.isInvalid(), "" );
                    phase    = 1;
                    ringWidx = getWedgeIndex( vih );
                }
                if ( phase == 1 && ringWidx != getWedgeIndex( vih ) )
                {
                    CORE_ASSERT( ringWidx.isValid(), "" );
                    CORE_ASSERT( getWedgeIndex( vih ).isValid(), "" );
                    phase       = 2;
                    currentWidx = otherWidx;
                }
                replaceWedgeIndex( vih, currentWidx );
            }
            else
            { m_wedges.setWedgePosition( getWedgeIndex( vih ), point( vh ) ); }
        }
        vih = prev_halfedge_handle( opposite_halfedge_handle( vih ) );
    } while ( vih != start );
    // Reference version from  openmesh
    //    for ( VertexIHalfedgeIter vih_it( vih_iter( vo ) ); vih_it.is_valid(); ++vih_it )
    //    {
    //        set_vertex_handle( *vih_it, vh );
    //        if ( !is_boundary( *vih_it ) )
    //        {
    //            if ( !keepFrom ) { replaceWedgeIndex( *vih_it, widx ); }
    //            else
    //            { m_wedges.setWedgePosition( getWedgeIndex( *vih_it ), point( vh ) ); }
    //        }
    //    }

    // halfedge -> halfedge
    set_next_halfedge_handle( hp, hn );
    if ( !is_boundary( hp ) )
        if ( !keepFrom ) replaceWedgeIndex( hp, widx );

    set_next_halfedge_handle( op, on );

    if ( keepFrom )
    {
        if ( !is_boundary( op ) ) replaceWedgeIndex( op, getWedgeIndex( ono ) );
    }
    // face -> halfedge
    if ( fh.is_valid() ) set_halfedge_handle( fh, hn );
    if ( fo.is_valid() ) set_halfedge_handle( fo, on );

    // vertex -> halfedge
    if ( halfedge_handle( vh ) == o ) set_halfedge_handle( vh, hn );
    adjust_outgoing_halfedge( vh );
    set_isolated( vo );

    // delete stuff
    status( edge_handle( h ) ).set_deleted( true );

    status( vo ).set_deleted( true );

    m_wedges.del( getWedgeIndex( h ) );
    m_wedges.del( getWedgeIndex( o ) );

    if ( has_halfedge_status() )
    {
        status( h ).set_deleted( true );
        status( o ).set_deleted( true );
    }
}

//-----------------------------------------------------------------------------
void TopologicalMesh::collapse_loop( HalfedgeHandle _hh ) {
    HalfedgeHandle h0 = _hh;
    HalfedgeHandle h1 = next_halfedge_handle( h0 );

    HalfedgeHandle o0 = opposite_halfedge_handle( h0 );
    HalfedgeHandle o1 = opposite_halfedge_handle( h1 );

    VertexHandle v0 = to_vertex_handle( h0 );
    VertexHandle v1 = to_vertex_handle( h1 );

    FaceHandle fh = face_handle( h0 );
    FaceHandle fo = face_handle( o0 );

    // is it a loop ?
    assert( ( next_halfedge_handle( h1 ) == h0 ) && ( h1 != o0 ) );

    // halfedge -> halfedge
    set_next_halfedge_handle( h1, next_halfedge_handle( o0 ) );
    replaceWedgeIndex( h1, getWedgeIndex( o0 ) );
    set_next_halfedge_handle( prev_halfedge_handle( o0 ), h1 );

    // halfedge -> face
    set_face_handle( h1, fo );

    // vertex -> halfedge
    set_halfedge_handle( v0, h1 );
    adjust_outgoing_halfedge( v0 );
    set_halfedge_handle( v1, o1 );
    adjust_outgoing_halfedge( v1 );

    // face -> halfedge
    if ( fo.is_valid() && halfedge_handle( fo ) == o0 ) { set_halfedge_handle( fo, h1 ); }

    // delete stuff
    if ( fh.is_valid() )
    {
        set_halfedge_handle( fh, InvalidHalfedgeHandle );
        status( fh ).set_deleted( true );
    }
    status( edge_handle( h0 ) ).set_deleted( true );

    m_wedges.del( getWedgeIndex( h0 ) );
    m_wedges.del( getWedgeIndex( o0 ) );

    if ( has_halfedge_status() )
    {
        status( h0 ).set_deleted( true );
        status( o0 ).set_deleted( true );
    }
}

void TopologicalMesh::collapseWedge( TopologicalMesh::HalfedgeHandle heh, bool keepFromWedges ) {
    collapse( heh, keepFromWedges );
}

void TopologicalMesh::garbage_collection() {
    // Wedge Ref count is already up to date, do not del again !

    auto offset = m_wedges.computeCleanupOffset();
    for ( HalfedgeIter he_it = halfedges_begin(); he_it != halfedges_end(); ++he_it )
    {
        if ( !status( *he_it ).deleted() )
        {
            auto index = property( m_wedgeIndexPph, *he_it );
            if ( index.isValid() ) { property( m_wedgeIndexPph, *he_it ) = index - offset[index]; }
        }
    }
    m_wedges.garbageCollection();
    base::garbage_collection();

    for ( HalfedgeIter he_it = halfedges_begin(); he_it != halfedges_end(); ++he_it )
    {
        ON_ASSERT( auto idx = property( m_wedgeIndexPph, *he_it ); );
        CORE_ASSERT( !idx.isValid() || !m_wedges.getWedge( idx ).isDeleted(),
                     "references deleted wedge remains after garbage collection" );
    }
    for ( size_t i = 0; i < m_wedges.size(); ++i )
    {
        CORE_ASSERT( !m_wedges.getWedge( WedgeIndex {i} ).isDeleted(),
                     "deleted wedge remains after garbage collection" );
    }
}

void TopologicalMesh::delete_face( FaceHandle _fh, bool _delete_isolated_vertices ) {
    for ( auto itr = fh_begin( _fh ); itr.is_valid(); ++itr )
    {
        auto idx = property( m_wedgeIndexPph, *itr );
        if ( idx.isInvalid() )
        {
            LOG( logWARNING )
                << "[TopologicalMesh::delete_face] halfedge has an invalid wedge index";
        }
        else
        { m_wedges.del( idx ); }
        // set an invalid index for the boundary halfedges
        property( m_wedgeIndexPph, *itr ) = WedgeIndex {};
    }
    base::delete_face( _fh, _delete_isolated_vertices );
}

/////////////// WEDGES RELATED STUFF /////////////////

TopologicalMesh::WedgeIndex
TopologicalMesh::WedgeCollection::add( const TopologicalMesh::WedgeData& wd ) {
    WedgeIndex idx;
    auto itr = std::find( m_data.begin(), m_data.end(), Wedge {wd} );

    if ( itr == m_data.end() )
    {
        idx = m_data.size();
        m_data.emplace_back( wd );
    }
    else
    {
        itr->incrementRefCount();
        idx = std::distance( m_data.begin(), itr );
    }
    return idx;
}

std::vector<int> TopologicalMesh::WedgeCollection::computeCleanupOffset() const {
    std::vector<int> ret( m_data.size(), 0 );
    int currentOffset = 0;
    for ( size_t i = 0; i < m_data.size(); ++i )
    {
        if ( m_data[i].isDeleted() )
        {
            ++currentOffset;
            ret[i] = -1;
        }
        else
        { ret[i] = currentOffset; }
    }
    return ret;
}

} // namespace Geometry
} // namespace Core
} // namespace Ra
