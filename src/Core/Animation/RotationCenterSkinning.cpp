#include <Core/Animation/RotationCenterSkinning.hpp>

#include <array>
#include <unordered_map>

#include <Core/Animation/DualQuaternionSkinning.hpp>
#include <Core/Animation/HandleWeight.hpp>
#include <Core/Animation/LinearBlendSkinning.hpp>
#include <Core/Animation/Pose.hpp>
#include <Core/Animation/SkinningData.hpp>
#include <Core/Geometry/TopologicalMesh.hpp>
#include <Core/Geometry/TriangleOperation.hpp> // triangleArea
#include <Core/Utils/Log.hpp>

namespace Ra {
namespace Core {
namespace Animation {

using namespace Utils; // log

Scalar weightSimilarity( const Eigen::SparseVector<Scalar>& v1w,
                         const Eigen::SparseVector<Scalar>& v2w,
                         Scalar sigma ) {
    const Scalar sigmaSq = sigma * sigma;

    Scalar result = 0;
    // Iterating over non zero coefficients
    for ( Eigen::SparseVector<Scalar>::InnerIterator it1( v1w ); it1; ++it1 )
    {
        const uint j      = it1.index();
        const Scalar& W1j = it1.value();              // This one is necessarily non zero
        const Scalar& W2j = v2w.coeff( it1.index() ); // This one may be 0.

        if ( W2j > 0 )
        {
            for ( Eigen::SparseVector<Scalar>::InnerIterator it2( v2w ); it2; ++it2 )
            {
                const uint k      = it2.index();
                const Scalar& W1k = v1w.coeff( it2.index() );
                const Scalar& W2k = it2.value();
                if ( j != k && W1k > 0 )
                {
                    const Scalar diff =
                        std::exp( -Math::ipow<2>( ( W1j * W2k ) - ( W1k * W2j ) ) / ( sigmaSq ) );
                    result += W1j * W1k * W2j * W2k * diff;
                }
            }
        }
    }
    return result;
}

void computeCoR( SkinningRefData& dataInOut, Scalar sigma, Scalar weightEpsilon ) {
    LOG( logDEBUG ) << "Precomputing CoRs";

    //
    // First step : subdivide the original mesh until weights are sufficiently close enough.
    //

    // convert the mesh to TopologicalMesh for easy processing.
    Geometry::TriangleMesh triMesh;
    triMesh.copy( dataInOut.m_referenceMesh );
    Geometry::TopologicalMesh topoMesh( triMesh );

    // hashing function for Vector3
    struct hash_vec {
        size_t operator()( const Ra::Core::Vector3& lvalue ) const {
            size_t hx = std::hash<Scalar>()( lvalue[0] );
            size_t hy = std::hash<Scalar>()( lvalue[1] );
            size_t hz = std::hash<Scalar>()( lvalue[2] );
            return ( hx ^ ( hy << 1 ) ) ^ hz;
        }
    };

    // fill map from vertex position to handle index, used to access the weight
    // matrix from initial mesh vertices
    std::unordered_map<Ra::Core::Vector3, int, hash_vec> mapV2I;
    for ( auto vit = topoMesh.vertices_begin(); vit != topoMesh.vertices_end(); ++vit )
    {
        mapV2I[topoMesh.point( *vit )] = vit->idx();
    }

    // Squash weight matrix to fit TopologicalMesh (access through handle indices)
    // Store the weights as row major here because we are going to query the per-vertex weights.
    Eigen::SparseMatrix<Scalar, Eigen::RowMajor> subdivW;
    const int numCols = dataInOut.m_weights.cols();
    subdivW.resize( topoMesh.n_vertices(), numCols );
    const auto& V = triMesh.vertices();
    for ( std::size_t i = 0; i < V.size(); ++i )
    {
        subdivW.row( mapV2I[V[i]] ) = dataInOut.m_weights.row( int( i ) );
    }

    // The mesh will be subdivided by repeated edge-split, so that adjacent vertices
    // weights are distant of at most `weightEpsilon`.
    // New vertices created by the edge splitting and their new weights are computed
    // and appended to the existing vertices.
    const Scalar wEps2       = weightEpsilon * weightEpsilon;
    Scalar maxWeightDistance = 0;
    do
    {
        maxWeightDistance = 0;

        // Stores the edges to split
        std::vector<Geometry::TopologicalMesh::EdgeHandle> edgesToSplit;

        // Compute all weights distances for all edges.
        for ( auto e_it = topoMesh.edges_begin(); e_it != topoMesh.edges_end(); ++e_it )
        {
            const auto& he0 = topoMesh.halfedge_handle( *e_it, 0 );
            const auto& he1 = topoMesh.halfedge_handle( *e_it, 1 );
            int v0          = topoMesh.to_vertex_handle( he0 ).idx();
            int v1          = topoMesh.to_vertex_handle( he1 ).idx();

            Scalar weightDistance = ( subdivW.row( v0 ) - subdivW.row( v1 ) ).squaredNorm();

            maxWeightDistance = std::max( maxWeightDistance, weightDistance );
            if ( weightDistance > wEps2 ) { edgesToSplit.push_back( *e_it ); }
        }
        LOG( logDEBUG ) << "Max weight distance is " << sqrt( maxWeightDistance );

        // sort edges to split according to growing weightDistance to avoid
        // creating edges larger than weightDistance
        std::sort(
            edgesToSplit.begin(),
            edgesToSplit.end(),
            [&topoMesh, &subdivW]( const auto& a, const auto& b ) {
                const auto& a0 = topoMesh.to_vertex_handle( topoMesh.halfedge_handle( a, 0 ) );
                const auto& a1 = topoMesh.to_vertex_handle( topoMesh.halfedge_handle( a, 1 ) );
                Scalar la = ( subdivW.row( a0.idx() ) - subdivW.row( a1.idx() ) ).squaredNorm();
                const auto& b0 = topoMesh.to_vertex_handle( topoMesh.halfedge_handle( b, 0 ) );
                const auto& b1 = topoMesh.to_vertex_handle( topoMesh.halfedge_handle( b, 1 ) );
                Scalar lb = ( subdivW.row( b0.idx() ) - subdivW.row( b1.idx() ) ).squaredNorm();
                return ( la > lb );
            } );

        // We found some edges over the limit, so we split them.
        if ( !edgesToSplit.empty() )
        {
            LOG( logDEBUG ) << "Splitting " << edgesToSplit.size() << " edges";
            int startIndex = subdivW.rows();

            Eigen::SparseMatrix<Scalar, Eigen::RowMajor> newWeights(
                startIndex + edgesToSplit.size(), numCols );

            newWeights.topRows( startIndex ) = subdivW;
            subdivW                          = newWeights;

            int i = 0;
            // Split ALL the edges !
            for ( const auto& edge : edgesToSplit )
            {
                int v0 = topoMesh.to_vertex_handle( topoMesh.halfedge_handle( edge, 0 ) ).idx();
                int v1 = topoMesh.to_vertex_handle( topoMesh.halfedge_handle( edge, 1 ) ).idx();

                topoMesh.splitEdge( edge, 0.5f );

                subdivW.row( startIndex + i ) = 0.5f * ( subdivW.row( v0 ) + subdivW.row( v1 ) );
                ++i;
            }
        }
    } while ( maxWeightDistance > wEps2 );

    CORE_ASSERT( topoMesh.n_vertices() == size_t( subdivW.rows() ),
                 "Weights and vertices don't match" );

    //
    // Second step : evaluate the integrals over all triangles for all vertices.
    //

    // naive implementation : iterate over all the triangles (of the subdivided mesh)
    // for all vertices (of the original mesh).
    const uint nVerts = V.size();
    dataInOut.m_CoR.clear();
    dataInOut.m_CoR.resize( nVerts, Vector3::Zero() );

    // first precompute triangle data
    std::map<Geometry::TopologicalMesh::FaceHandle,
             std::tuple<Vector3, Scalar, Eigen::SparseVector<Scalar>>>
        triangleData;
    for ( auto f_it = topoMesh.faces_begin(); f_it != topoMesh.faces_end(); ++f_it )
    {
        // get needed data
        const auto& he0        = topoMesh.halfedge_handle( *f_it );
        const auto& he1        = topoMesh.next_halfedge_handle( he0 );
        const auto& he2        = topoMesh.next_halfedge_handle( he1 );
        const auto& v0         = topoMesh.to_vertex_handle( he0 );
        const auto& v1         = topoMesh.to_vertex_handle( he1 );
        const auto& v2         = topoMesh.to_vertex_handle( he2 );
        const auto& p0         = topoMesh.point( v0 );
        const auto& p1         = topoMesh.point( v1 );
        const auto& p2         = topoMesh.point( v2 );
        const Vector3 centroid = ( p0 + p1 + p2 ) / 3.f;
        const Scalar area      = Geometry::triangleArea( p0, p1, p2 );
        const Eigen::SparseVector<Scalar> triWeight =
            ( 1 / 3.f ) *
            ( subdivW.row( v0.idx() ) + subdivW.row( v1.idx() ) + subdivW.row( v2.idx() ) );
        triangleData[*f_it] = std::make_tuple( centroid, area, triWeight );
    }

#pragma omp parallel for
    for ( int i = 0; i < int( nVerts ); ++i )
    {
        Vector3 cor( 0, 0, 0 );
        Scalar sumweight                     = 0;
        const Eigen::SparseVector<Scalar> Wi = subdivW.row( mapV2I[V[i]] );

        // Sum the cor and weights over all triangles of the subdivided mesh.
        for ( auto f_it = topoMesh.faces_begin(); f_it != topoMesh.faces_end(); ++f_it )
        {
            const auto& triData     = triangleData[*f_it];
            const Vector3& centroid = std::get<0>( triData );
            Scalar area             = std::get<1>( triData );
            const auto& triWeight   = std::get<2>( triData );
            const Scalar s          = weightSimilarity( Wi, triWeight, sigma );
            cor += s * area * centroid;
            sumweight += s * area;
        }

        // Avoid division by 0
        if ( sumweight > 0 ) { dataInOut.m_CoR[i] = cor / sumweight; }

#if defined CORE_DEBUG
        if ( i % 100 == 0 ) { LOG( logDEBUG ) << "CoR: " << i << " / " << nVerts; }
#endif // CORE_DEBUG
    }
}

void centerOfRotationSkinning( const SkinningRefData& refData,
                               const Vector3Array& tangents,
                               const Vector3Array& bitangents,
                               SkinningFrameData& frameData ) {
    CORE_ASSERT( refData.m_CoR.size() == frameData.m_currentPosition.size(),
                 "Invalid center of rotations" );

    const auto& W        = refData.m_weights;
    const auto& vertices = refData.m_referenceMesh.vertices();
    const auto& normals  = refData.m_referenceMesh.normals();
    const auto& CoR      = refData.m_CoR;
    auto pose            = frameData.m_skeleton.getPose( HandleArray::SpaceType::MODEL );

    // prepare the pose w.r.t. the bind matrices
#pragma omp parallel for
    for ( int i = 0; i < int( frameData.m_skeleton.size() ); ++i )
    {
        pose[i] = refData.m_meshTransformInverse * pose[i] * refData.m_bindMatrices[i];
    }
    // Compute the dual quaternions
    const auto DQ = computeDQ( pose, W );

    // Do LBS on the COR with weights of their associated vertices
#pragma omp parallel for
    for ( int i = 0; i < int( frameData.m_currentPosition.size() ); ++i )
    {
        frameData.m_currentPosition[i] = Vector3::Zero();
    }
    for ( int k = 0; k < W.outerSize(); ++k )
    {
        const int nonZero = W.col( k ).nonZeros();
        WeightMatrix::InnerIterator it0( W, k );
#pragma omp parallel for
        for ( int nz = 0; nz < nonZero; ++nz )
        {
            WeightMatrix::InnerIterator it = it0 + Eigen::Index( nz );
            const uint i                   = it.row();
            const uint j                   = it.col();
            const Scalar w                 = it.value();
            frameData.m_currentPosition[i] += w * ( pose[j] * CoR[i] );
        }
    }

    // Compute final transformation
#pragma omp parallel for
    for ( int i = 0; i < int( frameData.m_currentPosition.size() ); ++i )
    {
        frameData.m_currentPosition[i] += DQ[i].rotate( vertices[i] - CoR[i] );
        frameData.m_currentNormal[i]    = DQ[i].rotate( normals[i] );
        frameData.m_currentTangent[i]   = DQ[i].rotate( tangents[i] );
        frameData.m_currentBitangent[i] = DQ[i].rotate( bitangents[i] );
    }
}

} // namespace Animation
} // namespace Core
} // namespace Ra
