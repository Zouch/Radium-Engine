#pragma once

#include <Core/Geometry/deprecated/TopologicalMesh.hpp>
#include <Core/Math/LinearAlgebra.hpp> // Math::pi
#include <OpenMesh/Tools/Subdivider/Uniform/SubdividerT.hh>

namespace Ra {
namespace Core {
namespace Geometry {

/**
 * This class implements the Loop subdivision algorithm
 *
 * This class extends OpenMesh's LoopT subdivider to handle attributes.
 * \note We here consider that boundary halfedges do not store attributes.
 */
class RA_CORE_API LoopSubdivider
    : public OpenMesh::Subdivider::Uniform::SubdividerT<deprecated::TopologicalMesh, Scalar>
{

    using base    = OpenMesh::Subdivider::Uniform::SubdividerT<deprecated::TopologicalMesh, Scalar>;
    using Weight  = std::pair<Scalar, Scalar>;
    using Weights = std::vector<Weight>;
    using V_OP    = std::pair<Scalar, deprecated::TopologicalMesh::VertexHandle>;
    using V_OPS   = std::pair<deprecated::TopologicalMesh::VertexHandle, std::vector<V_OP>>;
    using SV_OPS  = std::vector<V_OPS>;
    using P_OP    = std::pair<Scalar, deprecated::TopologicalMesh::HalfedgeHandle>;
    using P_OPS   = std::pair<deprecated::TopologicalMesh::HalfedgeHandle, std::vector<P_OP>>;
    using SP_OPS  = std::vector<P_OPS>;

  public:
    LoopSubdivider() : base() {}

    explicit LoopSubdivider( deprecated::TopologicalMesh& mesh ) : base() { attach( mesh ); }

    ~LoopSubdivider() { detach(); }

  public:
    const char* name( void ) const override { return "LoopSubdivider"; }

    /// In the case one wants to apply the subdivision on the same mesh topology,
    /// but with a different geometry (e.g. for an animated character),
    /// one may want to just reapply the subdivision operations instead
    /// for performance reasons.
    /// This can be achieved with the following code:
    // clang-format off
    /// \code
    /// // 1- apply subdivision once
    /// Ra::Core::Geometry::TriangleMesh triangleMesh;
    /// Ra::Core::Geometry::deprecated::TopologicalMesh topoMesh( triangleMesh );
    /// Ra::Core::LoopSubdivider subdiv( topoMesh );
    /// subdiv( 2 );
    /// // get back to TriangleMesh (mandatory before re-applying)
    /// TriangleMesh subdividedMesh = topoMesh.toTriangleMesh();
    ///
    /// // 2- re-apply operations on new geometry (new_vertices, new_normals)
    /// m_subdivider.recompute( new_vertices, new_normals, subdividedMesh.vertices(),
    ///                         subdividedMesh.normals(), topoMesh );
    /// \endcode
    // clang-format on
    void recompute( const Vector3Array& newCoarseVertices,
                    const Vector3Array& newCoarseNormals,
                    Vector3Array& newSubdivVertices,
                    Vector3Array& newSubdivNormals,
                    deprecated::TopologicalMesh& mesh );

  protected:
    /// Pre-compute weights.
    void init_weights( size_t max_valence ) {
        m_weights.resize( max_valence );
        std::generate( m_weights.begin(), m_weights.end(), compute_weight() );
    }

    bool prepare( deprecated::TopologicalMesh& mesh ) override;

    bool cleanup( deprecated::TopologicalMesh& mesh ) override;

    bool subdivide( deprecated::TopologicalMesh& mesh,
                    size_t n,
                    const bool updatePoints = true ) override;

  private:
    /// Helper functor to compute weights for Loop-subdivision.
    struct compute_weight {
        compute_weight() : m_valence( -1 ) {}
        Weight operator()( void ) {
            //              1
            // alpha(n) = ---- * (40 - ( 3 + 2 cos( 2 Pi / n ) )^2 )
            //             64
            if ( ++m_valence ) {
                double inv_v = 1.0 / double( m_valence );
                double t     = ( 3.0 + 2.0 * std::cos( 2.0 * Math::Pi * inv_v ) );
                double alpha = ( 40.0 - t * t ) / 64.0;
                return Weight( 1.0 - alpha, inv_v * alpha );
            }
            return Weight( 0, 0 );
        }
        int m_valence;
    };

  private:
    // topological modifiers

    /// Face recomposition
    void split_face( deprecated::TopologicalMesh& mesh,
                     const deprecated::TopologicalMesh::FaceHandle& fh,
                     size_t iter );

    /// Face corner recomposition
    void corner_cutting( deprecated::TopologicalMesh& mesh,
                         const deprecated::TopologicalMesh::HalfedgeHandle& he,
                         size_t iter );

    /// Edge recomposition
    void split_edge( deprecated::TopologicalMesh& mesh,
                     const deprecated::TopologicalMesh::EdgeHandle& eh,
                     size_t iter );

    // geometry helpers

    /// compute edge midpoint
    void compute_midpoint( deprecated::TopologicalMesh& mesh,
                           const deprecated::TopologicalMesh::EdgeHandle& eh,
                           size_t iter );

    /// smooth input vertices
    void smooth( deprecated::TopologicalMesh& mesh,
                 const deprecated::TopologicalMesh::VertexHandle& vh,
                 size_t iter );

  private:
    /// old vertex new position
    OpenMesh::VPropHandleT<deprecated::TopologicalMesh::Point> m_vpPos;

    /// new edge midpoint position
    OpenMesh::EPropHandleT<deprecated::TopologicalMesh::VertexHandle> m_epPos;

    /// precomputed weights
    Weights m_weights;

    /// list of vertices computations
    std::vector<SV_OPS> m_oldVertexOps;
    std::vector<SV_OPS> m_newVertexOps;
    std::vector<SP_OPS> m_newEdgePropOps;
    std::vector<SP_OPS> m_newFacePropOps;

    /// old vertex halfedges
    OpenMesh::HPropHandleT<deprecated::TopologicalMesh::VertexHandle> m_hV;
};

} // namespace Geometry
} // namespace Core
} // namespace Ra
