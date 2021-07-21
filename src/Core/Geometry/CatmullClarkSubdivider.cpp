#include <Core/Geometry/CatmullClarkSubdivider.hpp>

namespace Ra {
namespace Core {
namespace Geometry {

bool CatmullClarkSubdivider::prepare( deprecated::TopologicalMesh& mesh ) {
    mesh.add_property( m_vpPos );
    mesh.add_property( m_epH );
    mesh.add_property( m_fpH );
    mesh.add_property( m_creaseWeights );
    mesh.createAllPropsOnFaces(
        m_normalPropF, m_floatPropsF, m_vec2PropsF, m_vec3PropsF, m_vec4PropsF );
    mesh.add_property( m_hV );
    for ( uint i = 0; i < mesh.n_halfedges(); ++i ) {
        auto h = mesh.halfedge_handle( i );
        if ( !mesh.is_boundary( h ) ) { mesh.property( m_hV, h ) = mesh.to_vertex_handle( h ); }
    }

    // initialize all weights to 0 (= smooth edge)
    for ( auto e_it = mesh.edges_begin(); e_it != mesh.edges_end(); ++e_it )
        mesh.property( m_creaseWeights, *e_it ) = 0.0;

    return true;
}

bool CatmullClarkSubdivider::cleanup( deprecated::TopologicalMesh& mesh ) {
    mesh.remove_property( m_vpPos );
    mesh.remove_property( m_epH );
    mesh.remove_property( m_fpH );
    mesh.remove_property( m_creaseWeights );
    mesh.clearAllProps( m_normalPropF, m_floatPropsF, m_vec2PropsF, m_vec3PropsF, m_vec4PropsF );
    mesh.remove_property( m_hV );
    return true;
}

bool CatmullClarkSubdivider::subdivide( deprecated::TopologicalMesh& mesh,
                                        size_t n,
                                        const bool update_points ) {
    m_oldVertexOps.clear();
    m_newFaceVertexOps.clear();
    m_newEdgeVertexOps.clear();
    m_newEdgePropOps.clear();
    m_newFacePropOps.clear();
    m_oldVertexOps.resize( n );
    m_newFaceVertexOps.resize( n );
    m_newEdgeVertexOps.resize( n );
    m_newEdgePropOps.resize( n );
    m_newFacePropOps.resize( n );
    // Do n subdivisions
    for ( size_t iter = 0; iter < n; ++iter ) {
        // Compute face centroid
        const size_t NV = mesh.n_vertices();
        m_newFaceVertexOps[iter].reserve( mesh.n_faces() );
#pragma omp parallel for
        for ( int i = 0; i < int( mesh.n_faces() ); ++i ) {
            const auto& fh = mesh.face_handle( i );
            // compute centroid
            deprecated::TopologicalMesh::Point centroid;
            mesh.calc_face_centroid( fh, centroid );
            deprecated::TopologicalMesh::VertexHandle vh;
#pragma omp critical
            { vh = mesh.new_vertex( centroid ); }
            mesh.property( m_fpH, fh ) = vh;
            // register operation
            const uint v       = mesh.valence( fh );
            const Scalar inv_v = 1.f / v;
            std::vector<V_OP> ops( v );
            auto heh = mesh.halfedge_handle( fh );
            for ( uint j = 0; j < v; ++j ) {
                ops[j] = V_OP( inv_v, mesh.to_vertex_handle( heh ) );
                heh    = mesh.next_halfedge_handle( heh );
            }
#pragma omp critical
            { m_newFaceVertexOps[iter].push_back( V_OPS( vh, ops ) ); }
            // deal with properties
            mesh.interpolateAllPropsOnFaces(
                fh, m_normalPropF, m_floatPropsF, m_vec2PropsF, m_vec3PropsF, m_vec4PropsF );
        }

        // Compute position for new (edge-) vertices and store them in the edge property
        m_newEdgeVertexOps[iter].reserve( mesh.n_edges() );
#pragma omp parallel for
        for ( int i = 0; i < int( mesh.n_edges() ); ++i ) {
            const auto& eh = mesh.edge_handle( i );
            compute_midpoint( mesh, eh, update_points, iter );
        }

        // position updates activated?
        if ( update_points ) {
            // compute new positions for old vertices
            m_oldVertexOps[iter].reserve( NV );
#pragma omp parallel for
            for ( int i = 0; i < int( NV ); ++i ) {
                const auto& vh = mesh.vertex_handle( i );
                update_vertex( mesh, vh, iter );
            }

            // Commit changes in geometry
#pragma omp parallel for
            for ( int i = 0; i < int( NV ); ++i ) {
                const auto& vh = mesh.vertex_handle( i );
                mesh.set_point( vh, mesh.property( m_vpPos, vh ) );
            }
        }

        // Split each edge at midpoint stored in edge property m_epPos;
        // Attention! Creating new edges, hence make sure the loop ends correctly.
        auto e_end = mesh.edges_end();
        m_newEdgePropOps[iter].reserve( 3 * mesh.n_edges() );
        for ( auto e_itr = mesh.edges_begin(); e_itr != e_end; ++e_itr ) {
            split_edge( mesh, *e_itr, iter );
        }
        m_newEdgePropOps[iter].shrink_to_fit();

        // Commit changes in topology and reconsitute consistency
        // Attention! Creating new faces, hence make sure the loop ends correctly.
        auto f_end = mesh.faces_end();
        m_newFacePropOps[iter].reserve( 2 * 4 * mesh.n_faces() );
        for ( auto f_itr = mesh.faces_begin(); f_itr != f_end; ++f_itr ) {
            split_face( mesh, *f_itr, iter );
        }
        m_newFacePropOps[iter].shrink_to_fit();

        CORE_ASSERT( OpenMesh::Utils::MeshCheckerT<deprecated::TopologicalMesh>( mesh ).check(),
                     "CatmullClarkSubdivision ended with a bad topology." );
    }

    // ###########################################################################
    // FIXME: THIS IS ONLY THERE BECAUSE CORE ONLY MANAGES TRIANGLE MESHES FOR NOW

    m_triangulationPropOps.clear();
    m_triangulationPropOps.reserve( 2 * mesh.n_faces() );
    // triangulate resulting quads
    auto f_end = mesh.faces_end();
    for ( auto f_itr = mesh.faces_begin(); f_itr != f_end; ++f_itr ) {
        // get all needed halfedges
        auto heh1 = mesh.halfedge_handle( *f_itr );
        auto heh2 = mesh.next_halfedge_handle( heh1 );
        auto heh3 = mesh.next_halfedge_handle( heh2 );
        auto heh4 = mesh.next_halfedge_handle( heh3 );
        auto heh5 = mesh.new_edge( mesh.to_vertex_handle( heh1 ), mesh.to_vertex_handle( heh3 ) );
        auto heh6 = mesh.opposite_halfedge_handle( heh5 );

        // triangulate
        auto fnew = mesh.new_face();
        mesh.set_halfedge_handle( fnew, heh2 );
        mesh.set_face_handle( heh5, *f_itr );
        mesh.set_face_handle( heh2, fnew );
        mesh.set_face_handle( heh3, fnew );
        mesh.set_face_handle( heh6, fnew );
        mesh.set_next_halfedge_handle( heh1, heh5 );
        mesh.set_next_halfedge_handle( heh5, heh4 );
        mesh.set_next_halfedge_handle( heh3, heh6 );
        mesh.set_next_halfedge_handle( heh6, heh2 );

        // deal with properties
        mesh.copyAllProps( heh3, heh5 );
        mesh.copyAllProps( heh1, heh6 );
        m_triangulationPropOps.push_back( { heh5, { { 1, heh3 } } } );
        m_triangulationPropOps.push_back( { heh6, { { 1, heh1 } } } );
    }

    return true;
}

void CatmullClarkSubdivider::split_face( deprecated::TopologicalMesh& mesh,
                                         const deprecated::TopologicalMesh::FaceHandle& fh,
                                         size_t iter ) {
    /*
        Split an n-gon into n quads by connecting
        each vertex of fh to vh.

        - fh will remain valid (it will become one of the quads)
        - the halfedge handles of the new quads will point to the old halfedges
    */

    // Since edges are already refined (valence*2)
    size_t valence = mesh.valence( fh ) / 2;

    // Add new mesh vertex from face centroid
    auto vh = mesh.property( m_fpH, fh );

    // init new topology with first face
    auto hend = mesh.halfedge_handle( fh );
    auto hh   = mesh.next_halfedge_handle( hend );
    auto hold = mesh.new_edge( mesh.to_vertex_handle( hend ), vh );
    mesh.set_next_halfedge_handle( hend, hold );
    mesh.set_face_handle( hold, fh );

    // deal with properties for vh
    mesh.copyAllPropsFromFace(
        fh, hold, m_normalPropF, m_floatPropsF, m_vec2PropsF, m_vec3PropsF, m_vec4PropsF );

    // go around new vertex to build topology
    hold = mesh.opposite_halfedge_handle( hold );

    // deal with properties for hold
    mesh.copyAllProps( hend, hold );
    m_newFacePropOps[iter].push_back( { hold, { { 1, hend } } } );

    const Scalar inv_val = Scalar( 1 ) / valence;
    std::vector<P_OP> p_ops( valence );
    p_ops[0] = { inv_val, hh };

    for ( size_t i = 1; i < valence; i++ ) {
        // go to next mid-edge vertex
        auto hnext = mesh.next_halfedge_handle( hh );
        auto hnew  = mesh.new_edge( mesh.to_vertex_handle( hnext ), vh );

        // create new face
        auto fnew = mesh.new_face();
        mesh.set_halfedge_handle( fnew, hh );
        mesh.set_face_handle( hnew, fnew );
        mesh.set_face_handle( hold, fnew );
        mesh.set_face_handle( hh, fnew );
        mesh.set_face_handle( hnext, fnew );
        mesh.set_next_halfedge_handle( hnew, hold );
        mesh.set_next_halfedge_handle( hold, hh );

        // deal with properties for hnew
        mesh.copyAllPropsFromFace(
            fh, hnew, m_normalPropF, m_floatPropsF, m_vec2PropsF, m_vec3PropsF, m_vec4PropsF );

        // prepare for next face
        hh   = mesh.next_halfedge_handle( hnext );
        hold = mesh.opposite_halfedge_handle( hnew );
        mesh.set_next_halfedge_handle( hnext, hnew ); // this has to be done after hh !

        // deal with properties for hold
        mesh.copyAllProps( hnext, hold );
        m_newFacePropOps[iter].push_back( { hold, { { 1, hnext } } } );
        p_ops[i] = { inv_val, hh };
    }

    // finish topology
    mesh.set_next_halfedge_handle( hold, hh );
    mesh.set_next_halfedge_handle( hh, hend );
    hh = mesh.next_halfedge_handle( hend );
    mesh.set_next_halfedge_handle( hh, hold );
    mesh.set_face_handle( hold, fh );
    mesh.set_halfedge_handle( vh, hold );

    // deal with property operations on centroid
    for ( auto vh_iter = mesh.vih_iter( vh ); vh_iter.is_valid(); ++vh_iter ) {
        m_newFacePropOps[iter].push_back( { *vh_iter, p_ops } );
    }
}

void CatmullClarkSubdivider::split_edge( deprecated::TopologicalMesh& mesh,
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
    vh = mesh.property( m_epH, eh );

    // Re-link mesh entities
    if ( mesh.is_boundary( eh ) ) {
        // spin around vh1
        for ( t_heh = heh; mesh.next_halfedge_handle( t_heh ) != opp_heh;
              t_heh = mesh.opposite_halfedge_handle( mesh.next_halfedge_handle( t_heh ) ) )
            ;
    }
    else {
        // spin inside opp_heh's face
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

    if ( mesh.face_handle( opp_heh ).is_valid() ) {
        mesh.set_face_handle( opp_new_heh, mesh.face_handle( opp_heh ) );
        mesh.set_halfedge_handle( mesh.face_handle( opp_new_heh ), opp_new_heh );

        // deal with custom properties
        mesh.interpolateAllProps( t_heh, opp_heh, opp_new_heh, 0.5 );
        m_newEdgePropOps[iter].push_back( { opp_new_heh, { { 0.5, t_heh }, { 0.5, opp_heh } } } );
    }

    if ( mesh.face_handle( heh ).is_valid() ) {
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

void CatmullClarkSubdivider::compute_midpoint( deprecated::TopologicalMesh& mesh,
                                               const deprecated::TopologicalMesh::EdgeHandle& eh,
                                               const bool update_points,
                                               size_t iter ) {
    deprecated::TopologicalMesh::HalfedgeHandle heh     = mesh.halfedge_handle( eh, 0 );
    deprecated::TopologicalMesh::HalfedgeHandle opp_heh = mesh.halfedge_handle( eh, 1 );

    deprecated::TopologicalMesh::Point pos = mesh.point( mesh.to_vertex_handle( heh ) );
    pos += mesh.point( mesh.to_vertex_handle( opp_heh ) );

    // prepare operations
    std::vector<V_OP> ops;

    // boundary edge: just average vertex positions
    // this yields the [1/2 1/2] mask
    if ( mesh.is_boundary( eh ) || !update_points ) {
        pos *= 0.5;
        // register operations
        ops.resize( 2 );
        ops[0] = V_OP( 0.5, mesh.to_vertex_handle( heh ) );
        ops[1] = V_OP( 0.5, mesh.to_vertex_handle( opp_heh ) );
    }
    else // inner edge: add neighbouring Vertices to sum
         // this yields the [1/16 1/16; 3/8 3/8; 1/16 1/16] mask
    {
        pos += mesh.point( mesh.property( m_fpH, mesh.face_handle( heh ) ) );
        pos += mesh.point( mesh.property( m_fpH, mesh.face_handle( opp_heh ) ) );
        pos *= 0.25;
        // register operations
        ops.resize( 4 );
        ops[0] = V_OP( 0.25, mesh.to_vertex_handle( heh ) );
        ops[1] = V_OP( 0.25, mesh.to_vertex_handle( opp_heh ) );
        ops[2] = V_OP( 0.25, mesh.property( m_fpH, mesh.face_handle( heh ) ) );
        ops[3] = V_OP( 0.25, mesh.property( m_fpH, mesh.face_handle( opp_heh ) ) );
    }

#pragma omp critical
    {
        // add new vertex and save into property
        auto vh                    = mesh.new_vertex( pos );
        mesh.property( m_epH, eh ) = vh;
        // register operations
        m_newEdgeVertexOps[iter].push_back( V_OPS( vh, ops ) );
    }
}

void CatmullClarkSubdivider::update_vertex( deprecated::TopologicalMesh& mesh,
                                            const deprecated::TopologicalMesh::VertexHandle& vh,
                                            size_t iter ) {
    deprecated::TopologicalMesh::Point pos( 0.0, 0.0, 0.0 );

    // prepare operations
    std::vector<V_OP> ops;

    // TODO boundary, Extraordinary Vertex and  Creased Surfaces
    // see "A Factored Approach to Subdivision Surfaces"
    // http://faculty.cs.tamu.edu/schaefer/research/tutorial.pdf
    // and http://www.cs.utah.edu/~lacewell/subdeval
    if ( mesh.is_boundary( vh ) ) {
        pos = mesh.point( vh );
        ops.resize( 3 );
        ops[0] = V_OP( 1.f / 3.f, vh );
        int i  = 1;
        for ( auto ve_itr = mesh.ve_iter( vh ); ve_itr.is_valid(); ++ve_itr ) {
            if ( mesh.is_boundary( *ve_itr ) ) {
                pos += mesh.point( mesh.property( m_epH, *ve_itr ) );
                ops[i++] = V_OP( 1.f / 3.f, mesh.property( m_epH, *ve_itr ) );
            }
        }
        pos /= 3.0;
    }
    else // inner vertex
    {
        /* For each (non boundary) vertex V, introduce a new vertex whose
           position is F/n + 2E/n + (n-3)V/n where F is the average of
           the new face vertices of all faces adjacent to the old vertex
           V, E is the average of the midpoints of all edges incident
           on the old vertex V, and n is the number of edges incident on
           the vertex.
       */
        const uint valence  = mesh.valence( vh );
        const Scalar inv_v2 = 1.f / ( valence * valence );
        ops.resize( valence * 2 + 1 );

        int i = 0;
        for ( auto voh_it = mesh.voh_iter( vh ); voh_it.is_valid(); ++voh_it ) {
            pos += mesh.point( mesh.to_vertex_handle( *voh_it ) );
            ops[i++] = V_OP( inv_v2, mesh.to_vertex_handle( *voh_it ) );
        }
        pos *= inv_v2;

        deprecated::TopologicalMesh::Point Q( 0, 0, 0 );
        for ( auto vf_itr = mesh.vf_iter( vh ); vf_itr.is_valid(); ++vf_itr ) {
            Q += mesh.point( mesh.property( m_fpH, *vf_itr ) );
            ops[i++] = V_OP( inv_v2, mesh.property( m_fpH, *vf_itr ) );
        }

        Q *= inv_v2;

        pos += mesh.point( vh ) * ( valence - 2.0 ) / valence + Q;
        ops[i] = V_OP( ( valence - 2.0 ) / valence, vh );
    }

    // don't set yet since would be needed for other vertices;
    mesh.property( m_vpPos, vh ) = pos;

#pragma omp critical
    { m_oldVertexOps[iter].push_back( V_OPS( vh, ops ) ); }
}

void CatmullClarkSubdivider::recompute( const Vector3Array& newCoarseVertices,
                                        const Vector3Array& newCoarseNormals,
                                        Vector3Array& newSubdivVertices,
                                        Vector3Array& newSubdivNormals,
                                        deprecated::TopologicalMesh& mesh ) {
    // update vertices
    auto inTriIndexProp = mesh.getInputTriangleMeshIndexPropHandle();
    auto hNormalProp    = mesh.halfedge_normals_pph();
#pragma omp parallel for
    for ( int i = 0; i < int( mesh.n_halfedges() ); ++i ) {
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
    for ( int i = 0; i < int( m_oldVertexOps.size() ); ++i ) {
        // reapply newFaceVertexOps
#pragma omp parallel for
        for ( int j = 0; j < int( m_newFaceVertexOps[i].size() ); ++j ) {
            Ra::Core::Vector3 pos( 0, 0, 0 );
            const auto& ops = m_newFaceVertexOps[i][j];
            for ( const auto& op : ops.second ) {
                pos += op.first * mesh.point( op.second );
            }
            mesh.set_point( ops.first, pos );
        }
        // reapply newEdgeVertexOps
#pragma omp parallel for
        for ( int j = 0; j < int( m_newEdgeVertexOps[i].size() ); ++j ) {
            Ra::Core::Vector3 pos( 0, 0, 0 );
            const auto& ops = m_newEdgeVertexOps[i][j];
            for ( const auto& op : ops.second ) {
                pos += op.first * mesh.point( op.second );
            }
            mesh.set_point( ops.first, pos );
        }
        // reapply oldVertexOps
        std::vector<Ra::Core::Vector3> pos( m_oldVertexOps[i].size() );
#pragma omp parallel for
        for ( int j = 0; j < int( m_oldVertexOps[i].size() ); ++j ) {
            pos[j]          = Ra::Core::Vector3( 0, 0, 0 );
            const auto& ops = m_oldVertexOps[i][j];
            for ( const auto& op : ops.second ) {
                pos[j] += op.first * mesh.point( op.second );
            }
        }
        // then commit pos for old vertices
#pragma omp parallel for schedule( static )
        for ( int j = 0; j < int( m_oldVertexOps[i].size() ); ++j ) {
            mesh.set_point( m_oldVertexOps[i][j].first, pos[j] );
        }
        // deal with normal on edges centers (other non-static properties can be updated the same
        // way)
        // This loop should not be parallelized!
        for ( int j = 0; j < int( m_newEdgePropOps[i].size() ); ++j ) {
            Ra::Core::Vector3 nor( 0, 0, 0 );
            const auto& ops = m_newEdgePropOps[i][j];
            for ( const auto& op : ops.second ) {
                nor += op.first * mesh.property( hNormalProp, op.second );
            }
            mesh.property( hNormalProp, ops.first ) = nor.normalized();
        }
        // deal with normal on faces centers (other non-static properties can be updated the same
        // way)
#pragma omp parallel for
        for ( int j = 0; j < int( m_newFacePropOps[i].size() ); ++j ) {
            Ra::Core::Vector3 nor( 0, 0, 0 );
            const auto& ops = m_newFacePropOps[i][j];
            for ( const auto& op : ops.second ) {
                nor += op.first * mesh.property( hNormalProp, op.second );
            }
            mesh.property( hNormalProp, ops.first ) = nor.normalized();
        }
    }
    // deal with normals from triangulation (other non-static properties can be updated the same
    // way)
#pragma omp parallel for
    for ( int j = 0; j < int( m_triangulationPropOps.size() ); ++j ) {
        Ra::Core::Vector3 nor( 0, 0, 0 );
        const auto& ops = m_triangulationPropOps[j];
        for ( const auto& op : ops.second ) {
            nor += op.first * mesh.property( hNormalProp, op.second );
        }
        mesh.property( hNormalProp, ops.first ) = nor.normalized();
    }
    // update subdivided TriangleMesh vertices and normals
    auto outTriIndexProp = mesh.getOutputTriangleMeshIndexPropHandle();
#pragma omp parallel for
    for ( int i = 0; i < int( mesh.n_halfedges() ); ++i ) {
        auto h = mesh.halfedge_handle( i );
        if ( !mesh.is_boundary( h ) ) {
            auto idx               = mesh.property( outTriIndexProp, h );
            newSubdivVertices[idx] = mesh.point( mesh.to_vertex_handle( h ) );
            newSubdivNormals[idx]  = mesh.property( hNormalProp, h );
        }
    }
}

} // namespace Geometry
} // namespace Core
} // namespace Ra
