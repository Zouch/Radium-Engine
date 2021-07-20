#include <Core/Geometry/LoopSubdivider.hpp>

namespace Ra {
namespace Core {
namespace Geometry {

bool LoopSubdivider::prepare( deprecated::TopologicalMesh& mesh ) {
    uint maxValence = 0;
    for ( auto v_it = mesh.vertices_begin(); v_it != mesh.vertices_end(); ++v_it )
    {
        if ( mesh.valence( *v_it ) > maxValence ) { maxValence = mesh.valence( *v_it ); }
    }
    init_weights( maxValence + 1 );
    mesh.add_property( m_vpPos );
    mesh.add_property( m_epPos );
    mesh.add_property( m_hV );
    for ( uint i = 0; i < mesh.n_halfedges(); ++i )
    {
        auto h = mesh.halfedge_handle( i );
        if ( !mesh.is_boundary( h ) ) { mesh.property( m_hV, h ) = mesh.to_vertex_handle( h ); }
    }
    return true;
}

bool LoopSubdivider::cleanup( deprecated::TopologicalMesh& mesh ) {
    mesh.remove_property( m_vpPos );
    mesh.remove_property( m_epPos );
    mesh.remove_property( m_hV );
    return true;
}

bool LoopSubdivider::subdivide( deprecated::TopologicalMesh& mesh,
                                size_t n,
                                const bool updatePoints ) {
    m_oldVertexOps.clear();
    m_newVertexOps.clear();
    m_newEdgePropOps.clear();
    m_newFacePropOps.clear();
    m_oldVertexOps.resize( n );
    m_newVertexOps.resize( n );
    m_newEdgePropOps.resize( n );
    m_newFacePropOps.resize( n );

    deprecated::TopologicalMesh::FaceIter fit, f_end;
    deprecated::TopologicalMesh::EdgeIter eit, e_end;

    // Do n subdivisions
    for ( uint iter = 0; iter < n; ++iter )
    {
        const size_t NV = mesh.n_vertices();
        if ( updatePoints )
        {
            // compute new positions for old vertices
            m_oldVertexOps[iter].reserve( NV );
#pragma omp parallel for
            for ( int i = 0; i < int( NV ); ++i )
            {
                const auto& vh = mesh.vertex_handle( i );
                smooth( mesh, vh, iter );
            }
        }

        // Compute position for new vertices and store them in the edge property
        m_newVertexOps[iter].reserve( mesh.n_edges() );
#pragma omp parallel for
        for ( int i = 0; i < int( mesh.n_edges() ); ++i )
        {
            const auto& eh = mesh.edge_handle( i );
            compute_midpoint( mesh, eh, iter );
        }

        // Split each edge at midpoint and store precomputed positions (stored in
        // edge property ep_pos_) in the vertex property vp_pos_;
        // Attention! Creating new edges, hence make sure the loop ends correctly.
        e_end = mesh.edges_end();
        m_newEdgePropOps[iter].reserve( 3 * mesh.n_edges() );
        for ( eit = mesh.edges_begin(); eit != e_end; ++eit )
        { split_edge( mesh, *eit, iter ); }
        m_newEdgePropOps[iter].shrink_to_fit();

        // Commit changes in topology and reconsitute consistency
        // Attention! Creating new faces, hence make sure the loop ends correctly.
        f_end = mesh.faces_end();
        m_newFacePropOps[iter].reserve( 6 * mesh.n_faces() );
        for ( fit = mesh.faces_begin(); fit != f_end; ++fit )
        { split_face( mesh, *fit, iter ); }
        m_newFacePropOps[iter].shrink_to_fit();

        if ( updatePoints )
        {
            // Commit changes in geometry
#pragma omp parallel for
            for ( int i = 0; i < int( NV ); ++i )
            {
                const auto& vh = mesh.vertex_handle( i );
                mesh.set_point( vh, mesh.property( m_vpPos, vh ) );
            }
        }

        // Now we have an consistent mesh!
        CORE_ASSERT( OpenMesh::Utils::MeshCheckerT<deprecated::TopologicalMesh>( mesh ).check(),
                     "LoopSubdivision ended with a bad topology." );
    }

    return true;
}

void LoopSubdivider::split_face( deprecated::TopologicalMesh& mesh,
                                 const deprecated::TopologicalMesh::FaceHandle& fh,
                                 size_t iter ) {
    using HeHandle = deprecated::TopologicalMesh::HalfedgeHandle;
    // get where to cut
    HeHandle heh1( mesh.halfedge_handle( fh ) );
    HeHandle heh2( mesh.next_halfedge_handle( mesh.next_halfedge_handle( heh1 ) ) );
    HeHandle heh3( mesh.next_halfedge_handle( mesh.next_halfedge_handle( heh2 ) ) );

    // Cutting off every corner of the 6_gon
    corner_cutting( mesh, heh1, iter );
    corner_cutting( mesh, heh2, iter );
    corner_cutting( mesh, heh3, iter );
}

void LoopSubdivider::corner_cutting( deprecated::TopologicalMesh& mesh,
                                     const deprecated::TopologicalMesh::HalfedgeHandle& he,
                                     size_t iter ) {
    using HeHandle = deprecated::TopologicalMesh::HalfedgeHandle;
    using VHandle  = deprecated::TopologicalMesh::VertexHandle;
    using FHandle  = deprecated::TopologicalMesh::FaceHandle;

    // Define Halfedge Handles
    HeHandle heh1( he );
    HeHandle heh5( heh1 );
    HeHandle heh6( mesh.next_halfedge_handle( heh1 ) );

    // Cycle around the polygon to find correct Halfedge
    for ( ; mesh.next_halfedge_handle( mesh.next_halfedge_handle( heh5 ) ) != heh1;
          heh5 = mesh.next_halfedge_handle( heh5 ) )
        ;

    VHandle vh1 = mesh.to_vertex_handle( heh1 );
    VHandle vh2 = mesh.to_vertex_handle( heh5 );

    HeHandle heh2( mesh.next_halfedge_handle( heh5 ) );
    HeHandle heh3( mesh.new_edge( vh1, vh2 ) );
    HeHandle heh4( mesh.opposite_halfedge_handle( heh3 ) );

    /* Intermediate result
     *
     *            *
     *         5 /|\
     *          /_  \
     *    vh2> *     *
     *        /|\3   |\
     *       /_  \|4   \
     *      *----\*----\*
     *          1 ^   6
     *            vh1 (adjust_outgoing halfedge!)
     */

    // Old and new Face
    FHandle fh_old( mesh.face_handle( heh6 ) );
    FHandle fh_new( mesh.new_face() );

    // Re-Set Handles around old Face
    mesh.set_next_halfedge_handle( heh4, heh6 );
    mesh.set_next_halfedge_handle( heh5, heh4 );

    mesh.set_face_handle( heh4, fh_old );
    mesh.set_face_handle( heh5, fh_old );
    mesh.set_face_handle( heh6, fh_old );
    mesh.set_halfedge_handle( fh_old, heh4 );

    // Re-Set Handles around new Face
    mesh.set_next_halfedge_handle( heh1, heh3 );
    mesh.set_next_halfedge_handle( heh3, heh2 );

    mesh.set_face_handle( heh1, fh_new );
    mesh.set_face_handle( heh2, fh_new );
    mesh.set_face_handle( heh3, fh_new );

    mesh.set_halfedge_handle( fh_new, heh1 );

    // deal with custom properties
    mesh.copyAllProps( heh1, heh4 );
    mesh.copyAllProps( heh5, heh3 );
    m_newFacePropOps[iter].push_back( { heh4, { { 1, heh1 } } } );
    m_newFacePropOps[iter].push_back( { heh3, { { 1, heh5 } } } );
}

void LoopSubdivider::split_edge( deprecated::TopologicalMesh& mesh,
                                 const deprecated::TopologicalMesh::EdgeHandle& eh,
                                 size_t iter ) {
    using HeHandle = deprecated::TopologicalMesh::HalfedgeHandle;
    using VHandle  = deprecated::TopologicalMesh::VertexHandle;

    // prepare data
    HeHandle heh     = mesh.halfedge_handle( eh, 0 );
    HeHandle opp_heh = mesh.halfedge_handle( eh, 1 );
    HeHandle new_heh;
    HeHandle opp_new_heh;
    HeHandle t_heh;
    VHandle vh;
    VHandle vh1( mesh.to_vertex_handle( heh ) );

    // new vertex
    vh = mesh.property( m_epPos, eh );

    // Re-link mesh entities
    if ( mesh.is_boundary( eh ) )
    {
        for ( t_heh = heh; mesh.next_halfedge_handle( t_heh ) != opp_heh;
              t_heh = mesh.opposite_halfedge_handle( mesh.next_halfedge_handle( t_heh ) ) )
            ;
    }
    else
    {
        for ( t_heh = mesh.next_halfedge_handle( opp_heh );
              mesh.next_halfedge_handle( t_heh ) != opp_heh;
              t_heh = mesh.next_halfedge_handle( t_heh ) )
            ;
    }

    new_heh     = mesh.new_edge( vh, vh1 );
    opp_new_heh = mesh.opposite_halfedge_handle( new_heh );
    mesh.set_vertex_handle( heh, vh );

    mesh.set_next_halfedge_handle( t_heh, opp_new_heh );
    mesh.set_next_halfedge_handle( new_heh, mesh.next_halfedge_handle( heh ) );
    mesh.set_next_halfedge_handle( heh, new_heh );
    mesh.set_next_halfedge_handle( opp_new_heh, opp_heh );

    if ( mesh.face_handle( opp_heh ).is_valid() )
    {
        mesh.set_face_handle( opp_new_heh, mesh.face_handle( opp_heh ) );
        mesh.set_halfedge_handle( mesh.face_handle( opp_new_heh ), opp_new_heh );

        // deal with custom properties
        mesh.interpolateAllProps( t_heh, opp_heh, opp_new_heh, 0.5 );
        m_newEdgePropOps[iter].push_back( { opp_new_heh, { { 0.5, t_heh }, { 0.5, opp_heh } } } );
    }

    if ( mesh.face_handle( heh ).is_valid() )
    {
        mesh.set_face_handle( new_heh, mesh.face_handle( heh ) );
        mesh.set_halfedge_handle( mesh.face_handle( heh ), heh );

        // deal with custom properties
        mesh.copyAllProps( heh, new_heh );
        m_newEdgePropOps[iter].push_back( { new_heh, { { 1, heh } } } );
        HeHandle heh_prev = mesh.prev_halfedge_handle( heh );
        mesh.interpolateAllProps( heh_prev, new_heh, heh, 0.5 );
        m_newEdgePropOps[iter].push_back( { heh, { { 0.5, heh_prev }, { 0.5, new_heh } } } );
    }

    mesh.set_halfedge_handle( vh, new_heh );
    mesh.set_halfedge_handle( vh1, opp_new_heh );

    // Never forget this, when playing with the topology
    mesh.adjust_outgoing_halfedge( vh );
    mesh.adjust_outgoing_halfedge( vh1 );
}

void LoopSubdivider::compute_midpoint( deprecated::TopologicalMesh& mesh,
                                       const deprecated::TopologicalMesh::EdgeHandle& eh,
                                       size_t iter ) {
    deprecated::TopologicalMesh::HalfedgeHandle heh     = mesh.halfedge_handle( eh, 0 );
    deprecated::TopologicalMesh::HalfedgeHandle opp_heh = mesh.halfedge_handle( eh, 1 );

    deprecated::TopologicalMesh::Point pos = mesh.point( mesh.to_vertex_handle( heh ) );
    pos += mesh.point( mesh.to_vertex_handle( opp_heh ) );

    std::vector<V_OP> ops;

    // boundary edge: just average vertex positions
    if ( mesh.is_boundary( eh ) )
    {
        pos *= 0.5;
        ops.resize( 2 );
        ops[0] = V_OP( 0.5, mesh.to_vertex_handle( heh ) );
        ops[1] = V_OP( 0.5, mesh.to_vertex_handle( opp_heh ) );
    }
    else // inner edge: add neighbouring Vertices to sum
    {
        pos *= 3.0;
        pos += mesh.point( mesh.to_vertex_handle( mesh.next_halfedge_handle( heh ) ) );
        pos += mesh.point( mesh.to_vertex_handle( mesh.next_halfedge_handle( opp_heh ) ) );
        pos *= 1.0 / 8.0;
        ops.resize( 4 );
        ops[0] = V_OP( 3.f / 8.f, mesh.to_vertex_handle( heh ) );
        ops[1] = V_OP( 3.f / 8.f, mesh.to_vertex_handle( opp_heh ) );
        ops[2] = V_OP( 1.f / 8.f, mesh.to_vertex_handle( mesh.next_halfedge_handle( heh ) ) );
        ops[3] = V_OP( 1.f / 8.f, mesh.to_vertex_handle( mesh.next_halfedge_handle( opp_heh ) ) );
    }

#pragma omp critical
    {
        auto vh                      = mesh.add_vertex( pos );
        mesh.property( m_epPos, eh ) = vh;
        m_newVertexOps[iter].push_back( V_OPS( vh, ops ) );
    }
}

void LoopSubdivider::smooth( deprecated::TopologicalMesh& mesh,
                             const deprecated::TopologicalMesh::VertexHandle& vh,
                             size_t iter ) {
    using VHandle = deprecated::TopologicalMesh::VertexHandle;

    deprecated::TopologicalMesh::Point pos( 0.0, 0.0, 0.0 );

    std::vector<V_OP> ops;

    if ( mesh.is_boundary( vh ) ) // if boundary: Point 1-6-1
    {
        deprecated::TopologicalMesh::HalfedgeHandle heh;
        deprecated::TopologicalMesh::HalfedgeHandle prev_heh;
        heh = mesh.halfedge_handle( vh );

        assert( mesh.is_boundary( mesh.edge_handle( heh ) ) );

        prev_heh = mesh.prev_halfedge_handle( heh );

        VHandle to_vh   = mesh.to_vertex_handle( heh );
        VHandle from_vh = mesh.from_vertex_handle( prev_heh );

        // ( v_l + 6 v + v_r ) / 8
        pos = mesh.point( vh );
        pos *= 6.0;
        pos += mesh.point( to_vh );
        pos += mesh.point( from_vh );
        pos *= 1.0 / 8.0;

        ops.resize( 3 );
        ops[2] = V_OP( 6.f / 8.f, vh );
        ops[0] = V_OP( 1.f / 8.f, to_vh );
        ops[1] = V_OP( 1.f / 8.f, from_vh );
    }
    else // inner vertex: (1-a) * p + a/n * Sum q, q in one-ring of p
    {
        deprecated::TopologicalMesh::VertexVertexIter vvit;
        const uint valence = mesh.valence( vh );
        ops.resize( valence + 1 );
        int i = 0;

        // Calculate Valence and sum up neighbour points
        for ( vvit = mesh.cvv_iter( vh ); vvit.is_valid(); ++vvit )
        {
            pos += mesh.point( *vvit );
            ops[i++] = V_OP( m_weights[valence].second, *vvit );
        }
        pos *= m_weights[valence].second; // alpha(n)/n * Sum q, q in one-ring of p
        pos += m_weights[valence].first * mesh.point( vh ); // + (1-a)*p
        ops[i] = V_OP( m_weights[valence].first, vh );
    }

    mesh.property( m_vpPos, vh ) = pos;

#pragma omp critical
    { m_oldVertexOps[iter].push_back( V_OPS( vh, ops ) ); }
}

void LoopSubdivider::recompute( const Vector3Array& newCoarseVertices,
                                const Vector3Array& newCoarseNormals,
                                Vector3Array& newSubdivVertices,
                                Vector3Array& newSubdivNormals,
                                deprecated::TopologicalMesh& mesh ) {
    // update vertices
    auto inTriIndexProp = mesh.getInputTriangleMeshIndexPropHandle();
    auto hNormalProp    = mesh.halfedge_normals_pph();
#pragma omp parallel for
    for ( int i = 0; i < int( mesh.n_halfedges() ); ++i )
    {
        auto h = mesh.halfedge_handle( i );
        // set position on coarse mesh vertices
        auto vh = mesh.property( m_hV, h );
        if ( vh.idx() != -1 ) // avoid both boundary and non-coarse halfedges
        {
            auto idx = mesh.property( inTriIndexProp, h );
            mesh.set_point( vh, newCoarseVertices[idx] );
            mesh.property( hNormalProp, h ) = newCoarseNormals[idx];
        }
    }
    // for each subdiv step
    for ( int i = 0; i < int( m_oldVertexOps.size() ); ++i )
    {
        // first update new vertices
#pragma omp parallel for schedule( static )
        for ( int j = 0; j < int( m_newVertexOps[i].size() ); ++j )
        {
            Ra::Core::Vector3 pos( 0, 0, 0 );
            const auto& ops = m_newVertexOps[i][j];
            for ( const auto& op : ops.second )
            { pos += op.first * mesh.point( op.second ); }
            mesh.set_point( ops.first, pos );
        }
        // then compute old vertices
        std::vector<Ra::Core::Vector3> pos( m_oldVertexOps[i].size() );
#pragma omp parallel for
        for ( int j = 0; j < int( m_oldVertexOps[i].size() ); ++j )
        {
            pos[j]          = Ra::Core::Vector3( 0, 0, 0 );
            const auto& ops = m_oldVertexOps[i][j];
            for ( const auto& op : ops.second )
            { pos[j] += op.first * mesh.point( op.second ); }
        }
        // then commit pos for old vertices
#pragma omp parallel for schedule( static )
        for ( int j = 0; j < int( m_oldVertexOps[i].size() ); ++j )
        { mesh.set_point( m_oldVertexOps[i][j].first, pos[j] ); }
        // deal with normal on edge centers (other non-static properties can be updated the same
        // way)
        // This loop should not be parallelized!
        for ( int j = 0; j < int( m_newEdgePropOps[i].size() ); ++j )
        {
            Ra::Core::Vector3 nor( 0, 0, 0 );
            const auto& ops = m_newEdgePropOps[i][j];
            for ( const auto& op : ops.second )
            { nor += op.first * mesh.property( hNormalProp, op.second ); }
            mesh.property( hNormalProp, ops.first ) = nor.normalized();
        }
        // deal with normal on faces centers (other non-static properties can be updated the same
        // way)
#pragma omp parallel for
        for ( int j = 0; j < int( m_newFacePropOps[i].size() ); ++j )
        {
            Ra::Core::Vector3 nor( 0, 0, 0 );
            const auto& ops = m_newFacePropOps[i][j];
            for ( const auto& op : ops.second )
            { nor += op.first * mesh.property( hNormalProp, op.second ); }
            mesh.property( hNormalProp, ops.first ) = nor.normalized();
        }
    }
    // update subdivided TriangleMesh vertices and normals
    auto outTriIndexProp = mesh.getOutputTriangleMeshIndexPropHandle();
#pragma omp parallel for
    for ( int i = 0; i < int( mesh.n_halfedges() ); ++i )
    {
        auto h = mesh.halfedge_handle( i );
        if ( !mesh.is_boundary( h ) )
        {
            auto idx               = mesh.property( outTriIndexProp, h );
            newSubdivVertices[idx] = mesh.point( mesh.to_vertex_handle( h ) );
            newSubdivNormals[idx]  = mesh.property( hNormalProp, h );
        }
    }
}

} // namespace Geometry
} // namespace Core
} // namespace Ra
