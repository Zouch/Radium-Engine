#include <Core/Geometry/MeshPrimitives.hpp>
#include <Core/Geometry/TopologicalMesh.hpp>
#include <Core/Geometry/TriangleMesh.hpp>
#include <catch2/catch.hpp>

#include <OpenMesh/Tools/Subdivider/Uniform/CatmullClarkT.hh>

using namespace Ra::Core;
using namespace Ra::Core::Utils;
using namespace Ra::Core::Geometry;

bool isSameMesh( const Ra::Core::Geometry::TriangleMesh& meshOne,
                 const Ra::Core::Geometry::TriangleMesh& meshTwo,
                 bool expected = true ) {

    bool result = true;
    int i       = 0;
    // Check length
    if ( meshOne.vertices().size() != meshTwo.vertices().size() )
    {
        if ( expected )
        {
            LOG( logINFO ) << "isSameMesh failed vertices.size()" << meshOne.vertices().size()
                           << " " << meshTwo.vertices().size();
        }
        return false;
    }
    if ( meshOne.normals().size() != meshTwo.normals().size() )
    {
        if ( expected ) { LOG( logINFO ) << "isSameMesh failed normals.size()"; }
        return false;
    }

    if ( meshOne.getIndices().size() != meshTwo.getIndices().size() )
    {
        if ( expected ) { LOG( logINFO ) << "isSameMesh failed getIndices().size()"; }
        return false;
    }

    // Check triangles
    std::vector<Vector3> stackVertices;
    std::vector<Vector3> stackNormals;

    bool hasNormals = meshOne.normals().size() > 0;

    i = 0;
    while ( result && i < int( meshOne.getIndices().size() ) )
    {
        std::vector<Ra::Core::Vector3>::iterator it;
        stackVertices.clear();
        stackVertices.push_back( meshOne.vertices()[meshOne.getIndices()[i][0]] );
        stackVertices.push_back( meshOne.vertices()[meshOne.getIndices()[i][1]] );
        stackVertices.push_back( meshOne.vertices()[meshOne.getIndices()[i][2]] );

        if ( hasNormals )
        {
            stackNormals.clear();
            stackNormals.push_back( meshOne.normals()[meshOne.getIndices()[i][0]] );
            stackNormals.push_back( meshOne.normals()[meshOne.getIndices()[i][1]] );
            stackNormals.push_back( meshOne.normals()[meshOne.getIndices()[i][2]] );
        }
        for ( int j = 0; j < 3; ++j )
        {
            it = find( stackVertices.begin(),
                       stackVertices.end(),
                       meshTwo.vertices()[meshTwo.getIndices()[i][j]] );
            if ( it != stackVertices.end() ) { stackVertices.erase( it ); }
            else
            {

                if ( expected ) { LOG( logINFO ) << "isSameMesh failed face not found"; }

                result = false;
            }
        }

        if ( hasNormals )
        {
            for ( int j = 0; j < 3; ++j )
            {
                it = find( stackNormals.begin(),
                           stackNormals.end(),
                           meshTwo.normals()[meshTwo.getIndices()[i][j]] );
                if ( it != stackNormals.end() ) { stackNormals.erase( it ); }
                else
                {

                    if ( expected ) { LOG( logINFO ) << "isSameMesh failed normal not found"; }

                    result = false;
                }
            }
        }
        ++i;
    }
    return result;
}

class WedgeDataAndIdx
{
  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    TopologicalMesh::WedgeData m_data;

    size_t m_idx;

    bool operator<( const WedgeDataAndIdx& lhs ) const { return m_data < lhs.m_data; }
    bool operator==( const WedgeDataAndIdx& lhs ) const { return !( m_data != lhs.m_data ); }
    bool operator!=( const WedgeDataAndIdx& lhs ) const { return !( *this == lhs ); }
};

#define COPY_TO_WEDGES_VECTOR_HELPER( UPTYPE, REALTYPE )                                       \
    if ( attr->is##UPTYPE() )                                                                  \
    {                                                                                          \
        auto data =                                                                            \
            meshOne.getAttrib( meshOne.template getAttribHandle<REALTYPE>( attr->getName() ) ) \
                .data();                                                                       \
        for ( size_t i = 0; i < size; ++i )                                                    \
        {                                                                                      \
            wedgesMeshOne[i].m_data.getAttribArray<REALTYPE>().push_back( data[i] );           \
        }                                                                                      \
    }

template <typename T>
void copyToWedgesVector( size_t size,
                         const IndexedGeometry<T>& meshOne,
                         AlignedStdVector<WedgeDataAndIdx>& wedgesMeshOne,
                         AttribBase* attr ) {

    if ( attr->getSize() != meshOne.vertices().size() )
    {
        LOG( logWARNING ) << "[TopologicalMesh test] Skip badly sized attribute "
                          << attr->getName();
    }
    else if ( attr->getName() != std::string( "in_position" ) )
    {
        {
            auto data = meshOne.vertices();
            for ( size_t i = 0; i < size; ++i )
            {
                wedgesMeshOne[i].m_data.m_position = data[i];
            }
        }

        COPY_TO_WEDGES_VECTOR_HELPER( Float, float );
        COPY_TO_WEDGES_VECTOR_HELPER( Vector2, Vector2 );
        COPY_TO_WEDGES_VECTOR_HELPER( Vector3, Vector3 );
        COPY_TO_WEDGES_VECTOR_HELPER( Vector4, Vector4 );
    }
}
#undef COPY_TO_WEDGES_VECTOR_HELPER

template <typename T>
bool isSameMeshWedge( const Ra::Core::Geometry::IndexedGeometry<T>& meshOne,
                      const Ra::Core::Geometry::IndexedGeometry<T>& meshTwo ) {

    using namespace Ra::Core;
    using namespace Ra::Core::Geometry;

    // Check length
    // LOG( logDEBUG ) << meshOne.vertices().size() << " / " << meshTwo.vertices().size();
    // LOG( logDEBUG ) << meshOne.normals().size() << " / " << meshTwo.normals().size();
    // LOG( logDEBUG ) << meshOne.getIndices().size() << " / " << meshTwo.getIndices().size();

    if ( meshOne.vertices().size() != meshTwo.vertices().size() ) return false;
    if ( meshOne.normals().size() != meshTwo.normals().size() ) return false;
    if ( meshOne.getIndices().size() != meshTwo.getIndices().size() ) return false;

    AlignedStdVector<WedgeDataAndIdx> wedgesMeshOne;
    AlignedStdVector<WedgeDataAndIdx> wedgesMeshTwo;

    auto size = meshOne.vertices().size();
    for ( size_t i = 0; i < size; ++i )
    {
        WedgeDataAndIdx wd;
        wd.m_idx = i;
        wedgesMeshOne.push_back( wd );
        wedgesMeshTwo.push_back( wd );
    }
    using namespace std::placeholders;
    auto f1 = std::bind(
        copyToWedgesVector<T>, size, std::cref( meshOne ), std::ref( wedgesMeshOne ), _1 );
    meshOne.vertexAttribs().for_each_attrib( f1 );

    auto f2 = std::bind(
        copyToWedgesVector<T>, size, std::cref( meshTwo ), std::ref( wedgesMeshTwo ), _1 );
    meshTwo.vertexAttribs().for_each_attrib( f2 );

    std::sort( wedgesMeshOne.begin(), wedgesMeshOne.end() );
    std::sort( wedgesMeshTwo.begin(), wedgesMeshTwo.end() );

    if ( wedgesMeshOne != wedgesMeshTwo )
    {
        // LOG( logDEBUG ) << "not same wedges";
        return false;
    }

    std::vector<int> newMeshOneIdx( wedgesMeshOne.size() );
    std::vector<int> newMeshTwoIdx( wedgesMeshOne.size() );

    size_t curIdx = 0;

    newMeshOneIdx[wedgesMeshOne[0].m_idx] = 0;
    newMeshTwoIdx[wedgesMeshTwo[0].m_idx] = 0;

    for ( size_t i = 1; i < wedgesMeshOne.size(); ++i )
    {
        if ( wedgesMeshOne[i] != wedgesMeshOne[i - 1] ) ++curIdx;
        newMeshOneIdx[wedgesMeshOne[i].m_idx] = curIdx;
        // std::cout << wedgesMeshOne[i].m_idx << " : " << curIdx << "\n";
    }
    // std::cout << "***\n";
    curIdx = 0;
    for ( size_t i = 1; i < wedgesMeshTwo.size(); ++i )
    {
        if ( wedgesMeshTwo[i] != wedgesMeshTwo[i - 1] ) ++curIdx;
        newMeshTwoIdx[wedgesMeshTwo[i].m_idx] = curIdx;
        // std::cout << wedgesMeshTwo[i].m_idx << " : " << curIdx << "\n";
    }

    typename Ra::Core::Geometry::IndexedGeometry<T>::IndexContainerType indices1 =
        meshOne.getIndices();
    typename Ra::Core::Geometry::IndexedGeometry<T>::IndexContainerType indices2 =
        meshTwo.getIndices();

    for ( auto& face : indices1 )
    {
        // std::cout << "face ";
        for ( int i = 0; i < face.size(); ++i )
        {
            face( i ) = newMeshOneIdx[face( i )];
            // std::cout << face( i ) << " ";
        }
        // std::cout << "\n";
    }
    // std::cout << "***\n";
    for ( auto& face : indices2 )
    {

        // std::cout << "face ";
        for ( int i = 0; i < face.size(); ++i )
        {
            face( i ) = newMeshTwoIdx[face( i )];
            // std::cout << face( i ) << " ";
        }
        // std::cout << "\n";
    }
    if ( indices1 != indices2 )
    {
        // LOG( logDEBUG ) << "not same indices";
        return false;
    }
    return true;
}

TEST_CASE( "Core/Geometry/TopologicalMesh", "[Core][Core/Geometry][TopologicalMesh]" ) {
    using Ra::Core::Vector3;
    using Ra::Core::Geometry::TopologicalMesh;
    using Ra::Core::Geometry::TriangleMesh;

    auto testConverter = []( const TriangleMesh& mesh ) {
        auto topologicalMesh = TopologicalMesh( mesh );
        auto newMesh         = topologicalMesh.toTriangleMesh();
        REQUIRE( isSameMesh( mesh, newMesh ) );
        REQUIRE( topologicalMesh.checkIntegrity() );
    };

    SECTION( "Closed mesh" ) {
        testConverter( Ra::Core::Geometry::makeBox() );
        testConverter( Ra::Core::Geometry::makeSharpBox() );
    }

    SECTION( "Mesh with boundaries" ) {
        testConverter( Ra::Core::Geometry::makePlaneGrid( 2, 2 ) );
    }

    SECTION( "With user def attribs" ) {
        using Vector5 = Eigen::Matrix<Scalar, 5, 1>;
        VectorArray<Vector5> array5 {{0_ra, 0_ra, 0_ra, 0_ra, 1_ra},
                                     {0_ra, 0_ra, 0_ra, 0_ra, 1_ra},
                                     {0_ra, 0_ra, 0_ra, 0_ra, 1_ra},
                                     {0_ra, -1_ra, 0_ra, 0_ra, 0_ra},
                                     {0_ra, 0_ra, 0_ra, 0_ra, 1_ra},
                                     {0_ra, 0_ra, 0_ra, 0_ra, 1_ra},
                                     {0_ra, 0_ra, 0_ra, 0_ra, 1_ra},
                                     {0_ra, -1_ra, 0_ra, 0_ra, 0_ra}};
        VectorArray<Vector4> array4 {{0_ra, 0_ra, 0_ra, 1_ra},
                                     {0_ra, 0_ra, 0_ra, 1_ra},
                                     {0_ra, 0_ra, 0_ra, 1_ra},
                                     {-1_ra, 0_ra, 0_ra, 0_ra},
                                     {0_ra, 0_ra, 0_ra, 1_ra},
                                     {0_ra, 0_ra, 0_ra, 1_ra},
                                     {0_ra, 0_ra, 0_ra, 1_ra},
                                     {-1_ra, 0_ra, 0_ra, 0_ra}};
        VectorArray<Vector2> array2 {{0_ra, 1_ra},
                                     {0_ra, 1_ra},
                                     {0_ra, 1_ra},
                                     {0_ra, 0_ra},
                                     {0_ra, 1_ra},
                                     {0_ra, 1_ra},
                                     {0_ra, 1_ra},
                                     {0_ra, 0_ra}};

        auto mesh     = Ra::Core::Geometry::makeBox();
        auto handle2  = mesh.addAttrib<Vector2>( "vector2_attrib" );
        auto handle4  = mesh.addAttrib<Vector4>( "vector4_attrib" );
        auto handle5  = mesh.addAttrib<Vector5>( "vector5_attrib" );
        auto ehandle2 = mesh.addAttrib<Vector2>( "evector2_attrib" );
        auto ehandle4 = mesh.addAttrib<Vector4>( "evector4_attrib" );
        auto ehandle5 = mesh.addAttrib<Vector5>( "evector5_attrib" );

        auto& attrib2 = mesh.getAttrib( handle2 );
        auto& attrib4 = mesh.getAttrib( handle4 );
        auto& attrib5 = mesh.getAttrib( handle5 );
        auto& buf2    = attrib2.getDataWithLock();
        auto& buf4    = attrib4.getDataWithLock();
        auto& buf5    = attrib5.getDataWithLock();
        buf2          = array2;
        buf4          = array4;
        buf5          = array5;
        attrib2.unlock();
        attrib4.unlock();
        attrib5.unlock();

        auto topologicalMesh = TopologicalMesh( mesh );
        auto newMesh         = topologicalMesh.toTriangleMesh();
        REQUIRE( isSameMesh( mesh, newMesh ) );
        REQUIRE( topologicalMesh.checkIntegrity() );

        // oversize attrib not suported
        REQUIRE( !newMesh.hasAttrib( "vector5_attrib" ) );

        REQUIRE( newMesh.hasAttrib( "vector2_attrib" ) );

        REQUIRE( newMesh.hasAttrib( "vector4_attrib" ) );

        // empty attrib not converted
        REQUIRE( !newMesh.hasAttrib( "evector2_attrib" ) );
        REQUIRE( !newMesh.hasAttrib( "evector4_attrib" ) );
        REQUIRE( !newMesh.hasAttrib( "evector5_attrib" ) );
    }

    SECTION( "Edit topo mesh" ) {
        auto mesh = Ra::Core::Geometry::makeCylinder( Vector3( 0, 0, 0 ), Vector3( 0, 0, 1 ), 1 );

        auto topologicalMesh = TopologicalMesh( mesh );
        auto newMesh         = topologicalMesh.toTriangleMesh();
        topologicalMesh.setWedgeData(
            TopologicalMesh::WedgeIndex {0}, "in_normal", Vector3( 0, 0, 0 ) );
        auto newMeshModified = topologicalMesh.toTriangleMesh();

        REQUIRE( isSameMesh( mesh, newMesh ) );
        REQUIRE( isSameMeshWedge( mesh, newMesh ) );
        REQUIRE( !isSameMeshWedge( mesh, newMeshModified ) );
        REQUIRE( topologicalMesh.checkIntegrity() );
    }

    SECTION( "Test skip empty attributes" ) {
        auto mesh = Ra::Core::Geometry::makeCylinder( Vector3( 0, 0, 0 ), Vector3( 0, 0, 1 ), 1 );
        mesh.addAttrib<float>( "empty" );
        auto topologicalMesh = TopologicalMesh( mesh );
        auto newMesh         = topologicalMesh.toTriangleMesh();
        REQUIRE( !newMesh.hasAttrib( "empty" ) );
        REQUIRE( topologicalMesh.checkIntegrity() );
    }

    SECTION( "Test normals" ) {
        auto mesh            = Ra::Core::Geometry::makeBox();
        auto topologicalMesh = TopologicalMesh( mesh );

        for ( TopologicalMesh::ConstVertexIter v_it = topologicalMesh.vertices_begin();
              v_it != topologicalMesh.vertices_end();
              ++v_it )
        {
            topologicalMesh.set_normal(
                *v_it, TopologicalMesh::Normal( Scalar( 1. ), Scalar( 0. ), Scalar( 0. ) ) );
        }

        for ( TopologicalMesh::ConstVertexIter v_it = topologicalMesh.vertices_begin();
              v_it != topologicalMesh.vertices_end();
              ++v_it )
        {
            topologicalMesh.propagate_normal_to_wedges( *v_it );
        }

        auto newMesh = topologicalMesh.toTriangleMesh();
        bool check1  = true;
        bool check2  = true;
        for ( auto n : newMesh.normals() )
        {
            if ( !Ra::Core::Math::areApproxEqual(
                     n.dot( Vector3( Scalar( 1. ), Scalar( 0. ), Scalar( 0. ) ) ), Scalar( 1. ) ) )
            { check1 = false; }
            if ( n.dot( Vector3( Scalar( 0.5 ), Scalar( 0. ), Scalar( 0. ) ) ) > Scalar( 0.8 ) )
            { check2 = false; }
        }
        REQUIRE( check1 );
        REQUIRE( check2 );
        REQUIRE( topologicalMesh.checkIntegrity() );
    }

    SECTION( "Test without normals" ) {
        VectorArray<Vector3> vertices = {
            {0_ra, 0_ra, 0_ra}, {0_ra, 1_ra, 0_ra}, {1_ra, 1_ra, 0_ra}, {1_ra, 0_ra, 0_ra}};
        VectorArray<Vector3ui> indices {{0, 2, 1}, {0, 3, 2}};
        // well formed mesh

        TriangleMesh mesh;
        mesh.setVertices( std::move( vertices ) );
        mesh.setIndices( std::move( indices ) );
        TopologicalMesh topo1 {mesh};
        REQUIRE( topo1.checkIntegrity() );
        TriangleMesh mesh1 = topo1.toTriangleMesh();

        // there is no normals at all.
        REQUIRE( !topo1.has_halfedge_normals() );
        REQUIRE( !topo1.has_face_normals() );
        for ( auto vitr = topo1.vertices_begin(), vend = topo1.vertices_end(); vitr != vend;
              ++vitr )
        {
            topo1.propagate_normal_to_wedges( *vitr );
            REQUIRE( !topo1.has_halfedge_normals() );
            REQUIRE( !topo1.has_face_normals() );
        }

        // nor on faces nor if we try to create them
        REQUIRE( !topo1.has_face_normals() );
        OpenMesh::FPropHandleT<TopologicalMesh::Normal> fProp;

        REQUIRE( mesh.vertexAttribs().hasSameAttribs( mesh1.vertexAttribs() ) );
        REQUIRE( isSameMesh( mesh, mesh1 ) );

        REQUIRE( mesh1.normals().size() == 0 );
    }
}

void test_split( TopologicalMesh& topo, TopologicalMesh::EdgeHandle eh, float f ) {

    auto he0 = topo.halfedge_handle( eh, 0 );
    auto he1 = topo.halfedge_handle( eh, 1 );
    auto v0  = topo.from_vertex_handle( he0 ); // i.e. to_vertex_handle(he1)
    REQUIRE( v0 == topo.to_vertex_handle( he1 ) );
    auto v1  = topo.to_vertex_handle( he0 );
    auto p0  = topo.point( v0 );
    float f0 = topo.getWedgeData( *( topo.getVertexWedges( v0 ) ).begin() ).m_floatAttrib[0];
    auto p1  = topo.point( v1 );
    float f1 = topo.getWedgeData( *( topo.getVertexWedges( v1 ) ).begin() ).m_floatAttrib[0];
    topo.splitEdge( eh, f );

    // check validity
    REQUIRE( topo.is_valid_handle( he0 ) );
    REQUIRE( topo.is_valid_handle( he1 ) );

    // he0 is untouched
    REQUIRE( v1 == topo.to_vertex_handle( he0 ) );
    REQUIRE( Math::areApproxEqual( ( p1 - topo.point( v1 ) ).squaredNorm(), 0_ra ) );

    // he1 point to inserted vertex
    auto vsplit = topo.to_vertex_handle( he1 ); // i.e. from_vertex_handle(he0)
    REQUIRE( vsplit == topo.from_vertex_handle( he0 ) );

    auto psplit = topo.point( vsplit );
    auto vcheck = ( f * p1 + ( 1.f - f ) * p0 );
    REQUIRE( Math::areApproxEqual( ( psplit - vcheck ).squaredNorm(), 0.f ) );

    auto wedges = topo.getVertexWedges( vsplit );
    REQUIRE( wedges.size() == 1 );

    auto wd     = topo.getWedgeData( *wedges.begin() );
    auto fsplit = wd.m_floatAttrib[0];
    auto fcheck = ( f * f1 + ( 1.f - f ) * f0 );
    REQUIRE( Math::areApproxEqual( fsplit, fcheck ) );
    REQUIRE( Math::areApproxEqual( ( psplit - wd.m_position ).squaredNorm(), 0.f ) );
}

void test_poly() {
    Ra::Core::Geometry::PolyMesh polyMesh;
    polyMesh.setVertices( {
        // quad
        {-1.1_ra, -0_ra, 0_ra},
        {1.1_ra, -0_ra, 0_ra},
        {1_ra, 1_ra, 0_ra},
        {-1_ra, 1_ra, 0_ra},
        // hepta
        {2_ra, 2_ra, 0_ra},
        {2_ra, 3_ra, 0_ra},
        {0_ra, 4_ra, 0_ra},
        {-2_ra, 3_ra, 0_ra},
        {-2_ra, 2_ra, 0_ra},
        // degen
        {-1.1_ra, -2_ra, 0_ra},
        {-0.5_ra, -2_ra, 0_ra},
        {-0.3_ra, -2_ra, 0_ra},
        {0.0_ra, -2_ra, 0_ra},
        {0.001_ra, -2_ra, 0_ra},
        {0.3_ra, -2_ra, 0_ra},
        {0.5_ra, -2_ra, 0_ra},
        {1.1_ra, -2_ra, 0_ra},
        // degen2
        {-1_ra, -3_ra, 0_ra},
        {1_ra, -3_ra, 0_ra},

    } );

    Vector3Array normals;
    normals.resize( polyMesh.vertices().size() );
    std::transform(
        polyMesh.vertices().cbegin(),
        polyMesh.vertices().cend(),
        normals.begin(),
        []( const Vector3& v ) { return ( v + Vector3( 0_ra, 0_ra, 1_ra ) ).normalized(); } );
    polyMesh.setNormals( normals );

    auto quad = VectorNui( 4 );
    quad << 0, 1, 2, 3;
    auto hepta = VectorNui( 7 );
    hepta << 3, 2, 4, 5, 6, 7, 8;
    auto degen = VectorNui( 10 );
    degen << 1, 0, 9, 10, 11, 12, 13, 14, 15, 16;
    auto degen2 = VectorNui( 10 );
    degen2 << 14, 13, 12, 11, 10, 9, 17, 18, 16, 15;
    polyMesh.setIndices( {quad, hepta, degen, degen2} );

    TopologicalMesh topologicalMesh;
    topologicalMesh.initWithWedge( polyMesh );
    auto newMesh = topologicalMesh.toPolyMesh();
    REQUIRE( isSameMeshWedge( newMesh, polyMesh ) );
}

TEST_CASE( "Core/Geometry/TopologicalMesh/PolyMesh",
           "[Core][Core/Geometry][TopologicalMesh][PolyMesh]" ) {

    test_poly();
}

/// \todo TEST_CASE( "Core/Geometry/TopologicalMesh/Subdivider",
/// "[Core][Core/Geometry][TopologicalMesh]" ) {
// using Catmull =
//     OpenMesh::Subdivider::Uniform::CatmullClarkT<Ra::Core::Geometry::TopologicalMesh>;
// using Loop = OpenMesh::Subdivider::Uniform::LoopT<Ra::Core::Geometry::TopologicalMesh>;
// using Decimater = OpenMesh::Decimater::DecimaterT<Ra::Core::Geometry::TopologicalMesh>;
// using HModQuadric =
//     OpenMesh::Decimater::ModQuadricT<Ra::Core::Geometry::TopologicalMesh>::Handle;
//}

TEST_CASE( "Core/Geometry/TopologicalMesh/EdgeSplit", "[Core][Core/Geometry][TopologicalMesh]" ) {
    using Ra::Core::Vector3;
    using Ra::Core::Geometry::TopologicalMesh;
    using Ra::Core::Geometry::TriangleMesh;

    // create a triangle mesh with 4 vertices
    TriangleMesh meshSplit;
    meshSplit.setVertices( {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}} );
    meshSplit.setNormals( {{-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}} );
    meshSplit.setIndices( {Vector3ui( 0, 1, 2 ), Vector3ui( 0, 2, 3 )} );
    // add a float attrib
    auto handle = meshSplit.addAttrib<float>( "test", {0.f, 1.f, 2.f, 3.f} );
    CORE_UNUSED( handle ); // until unit test is finished.

    // convert to topomesh
    TopologicalMesh topo = TopologicalMesh( meshSplit );

    // split middle edge
    TopologicalMesh::EdgeHandle eh;
    // iterate over all to find the inner one
    int innerEdgeCount = 0;
    for ( TopologicalMesh::EdgeIter e_it = topo.edges_begin(); e_it != topo.edges_end(); ++e_it )
    {
        if ( !topo.is_boundary( *e_it ) )
        {
            eh = *e_it;
            ++innerEdgeCount;
        }
    }

    REQUIRE( innerEdgeCount == 1 );
    float f = .3f;

    test_split( topo, eh, f );
    /// \todo : split boundary edge.
}

TEST_CASE( "Core/Geometry/TopologicalMesh/Manifold", "[Core][Core/Geometry][TopologicalMesh]" ) {
    SECTION( "Non manifold faces" ) {
        struct MyNonManifoldCommand {
            explicit inline MyNonManifoldCommand( int target ) : targetNonManifoldFaces( target ) {}
            inline void initialize( const IndexedGeometry<TriangleMesh::IndexType>& ) {}
            inline void process( const std::vector<TopologicalMesh::VertexHandle>& ) {
                LOG( logINFO ) << "Non Manifold face found";
                nonManifoldFaces++;
            }
            inline void postProcess( TopologicalMesh& ) {
                REQUIRE( nonManifoldFaces == targetNonManifoldFaces );
                LOG( logINFO ) << "Process non-manifold faces";
            }

            int nonManifoldFaces {0};
            const int targetNonManifoldFaces;
        };

        auto buildMesh = []( const VectorArray<Vector3>& v,
                             const VectorArray<Vector3>& n,
                             const VectorArray<Vector3ui>& i ) {
            TriangleMesh m;
            m.setVertices( v );
            m.setNormals( n );
            auto& idx = m.getIndicesWithLock();
            std::copy( i.begin(), i.end(), std::back_inserter( idx ) );
            m.indicesUnlock();

            LOG( logINFO ) << " Built a mesh with " << m.vertices().size() << " vertices, "
                           << m.normals().size() << " normals and " << m.getIndices().size()
                           << " indices.";

            return m;
        };

        // test if candidateMesh  -> TopologicalMesh -> TriangleMesh isSameMesh than referenceMesh,
        // with and without the command.
        auto testConverter = []( const TriangleMesh& referenceMesh,
                                 const TriangleMesh& candidateMesh,
                                 MyNonManifoldCommand command ) {
            // test with functor
            TopologicalMesh topoWithCommand {candidateMesh, command};
            auto convertedMeshWithCommand = topoWithCommand.toTriangleMesh();
            REQUIRE( isSameMesh( referenceMesh, convertedMeshWithCommand ) );
            // test without functor
            TopologicalMesh topoWithoutCommand {candidateMesh};
            auto convertedMeshWithoutCommand = topoWithoutCommand.toTriangleMesh();
            REQUIRE( isSameMesh( referenceMesh, convertedMeshWithoutCommand ) );
            return convertedMeshWithoutCommand;
        };

        VectorArray<Vector3> vertices = {
            {0_ra, 0_ra, 0_ra}, {0_ra, 1_ra, 0_ra}, {1_ra, 1_ra, 0_ra}, {1_ra, 0_ra, 0_ra}};
        VectorArray<Vector3> normals {
            {0_ra, 0_ra, 1_ra}, {0_ra, 0_ra, 1_ra}, {0_ra, 0_ra, 1_ra}, {0_ra, 0_ra, 1_ra}};
        VectorArray<Vector3ui> indices {{0, 2, 1}, {0, 3, 2}};

        VectorArray<Vector3> vertices_2 = {{0_ra, 0_ra, 0_ra},
                                           {0_ra, 1_ra, 0_ra},
                                           {1_ra, 1_ra, 0_ra},
                                           {1_ra, 0_ra, 0_ra},
                                           {1_ra, 0_ra, 1_ra}};
        VectorArray<Vector3> normals_2 {
            {0_ra, 0_ra, 1_ra},
            {0_ra, 0_ra, 1_ra},
            {0_ra, 0_ra, 1_ra},
            {0_ra, 0_ra, 1_ra},
            {0_ra, -1_ra, 0_ra},
        };

        VectorArray<Vector3ui> indices_2 {{0, 2, 1}, {0, 3, 2}, {0, 2, 4}};

        using Vector5 = Eigen::Matrix<Scalar, 5, 1>;
        VectorArray<Vector5> attrib_array {
            {0_ra, 0_ra, 0_ra, 0_ra, 1_ra},
            {0_ra, 0_ra, 0_ra, 0_ra, 1_ra},
            {0_ra, 0_ra, 0_ra, 0_ra, 1_ra},
            {0_ra, -1_ra, 0_ra, 0_ra, 0_ra},
        };

        // well formed mesh
        auto mesh = buildMesh( vertices, normals, indices );

        // edge shared by three faces
        LOG( logINFO ) << "Test with edge shared by three faces";
        auto mesh2 = buildMesh( vertices_2, normals_2, indices_2 );

        testConverter(
            mesh, mesh2, MyNonManifoldCommand( 1 ) ); // we should find 1 non-manifold face

        // test with unsupported attribute type
        LOG( logINFO ) << "Test with unsupported attribute (all faces are manifold)";
        auto mesh3 {mesh}, mesh4 {mesh};
        auto handle  = mesh3.addAttrib<Vector5>( "vector5_attrib" );
        auto& attrib = mesh3.getAttrib( handle );
        auto& buf    = attrib.getDataWithLock();
        buf          = attrib_array;
        attrib.unlock();

        REQUIRE( mesh4.vertexAttribs().hasSameAttribs( mesh.vertexAttribs() ) );
        REQUIRE( mesh.vertexAttribs().hasSameAttribs( mesh4.vertexAttribs() ) );
        REQUIRE( !mesh4.vertexAttribs().hasSameAttribs( mesh3.vertexAttribs() ) );
        REQUIRE( !mesh3.vertexAttribs().hasSameAttribs( mesh4.vertexAttribs() ) );
        mesh4 = testConverter(
            mesh, mesh3, MyNonManifoldCommand( 0 ) ); // we should find 0 non-manifold face
        REQUIRE( mesh4.vertexAttribs().hasSameAttribs( mesh.vertexAttribs() ) );
        REQUIRE( mesh.vertexAttribs().hasSameAttribs( mesh4.vertexAttribs() ) );
        REQUIRE( !mesh4.vertexAttribs().hasSameAttribs( mesh3.vertexAttribs() ) );
        REQUIRE( !mesh3.vertexAttribs().hasSameAttribs( mesh4.vertexAttribs() ) );

        // TODO : build a functor that add the faces as independant faces in the topomesh and
        // define a manifold mesh that is similar to the result of processing of this non manifold.
        //
    }
    SECTION( "Non manifold vertex : Bow tie" ) {
        VectorArray<Vector3> vertices = {
            {-1_ra, -1_ra, 0_ra},
            {-1_ra, 1_ra, 0_ra},
            {0_ra, 0_ra, 0_ra}, // non manifold vertex
            {1_ra, -1_ra, 0_ra},
            {1_ra, 1_ra, 0_ra},
        };

        VectorArray<Vector3ui> indices {{0, 2, 1}, {2, 3, 4}};
        TriangleMesh mesh;
        // do not move vertices, we need to compare afterward
        mesh.setVertices( vertices );
        mesh.setIndices( std::move( indices ) );

        TopologicalMesh topo {mesh};

        for ( auto itr = topo.vertices_begin(); itr != topo.vertices_end(); ++itr )
        {
            if ( Ra::Core::Math::areApproxEqual( ( topo.point( *itr ) - vertices[2] ).squaredNorm(),
                                                 0_ra ) )
            { REQUIRE( !topo.isManifold( *itr ) ); }
            else
            { REQUIRE( topo.isManifold( *itr ) ); }
        }
    }
    SECTION( "Non manifold vertex : Double pyramid" ) {

        struct MyNonManifoldCommand {
            explicit inline MyNonManifoldCommand(
                std::vector<std::vector<TopologicalMesh::VertexHandle>>& faulty ) :
                m_faulty( faulty ) {}
            inline void initialize( const IndexedGeometry<TriangleMesh::IndexType>& ) {}
            inline void process( const std::vector<TopologicalMesh::VertexHandle>& face_vhandles ) {
                m_faulty.push_back( face_vhandles );
                nonManifoldFaces++;
            }
            inline void postProcess( TopologicalMesh& ) {}
            std::vector<std::vector<TopologicalMesh::VertexHandle>>& m_faulty;
            int nonManifoldFaces {0};
        };

        VectorArray<Vector3> vertices = {{0_ra, 1_ra, 1_ra},
                                         {1_ra, 1_ra, 1_ra},
                                         {0.5_ra, 1_ra, 0_ra},
                                         {0.5_ra, 0.5_ra, 0.5_ra}, // non manifold vertex
                                         {0_ra, 0_ra, 0_ra},
                                         {1_ra, 0_ra, 0_ra},
                                         {0.5_ra, 0_ra, 1_ra}};
        VectorArray<Vector3ui> indices {
            {0, 1, 2}, {2, 1, 3}, {1, 0, 3}, {0, 2, 3}, {4, 5, 6}, {5, 4, 3}, {4, 6, 3}, {6, 5, 3}};

        TriangleMesh mesh;
        mesh.setVertices( std::move( vertices ) );
        mesh.setIndices( std::move( indices ) );
        std::vector<std::vector<TopologicalMesh::VertexHandle>> faulty;

        MyNonManifoldCommand command {faulty};
        TopologicalMesh topo {mesh, command};

        for ( auto itr = faulty.begin(); itr != faulty.end(); ++itr )
        {
            int cpt = 0;
            for ( auto pitr = itr->begin(); pitr != itr->end(); ++pitr )
            {
                // vertex handle is part of the mesh
                REQUIRE( topo.is_valid_handle( *pitr ) );

                // each of the faulty face has one time the non manifold vertex
                if ( Ra::Core::Math::areApproxEqual(
                         ( topo.point( *pitr ) - vertices[3] ).squaredNorm(), 0_ra ) )
                {
                    cpt++;
                    // this vertex is not a boundary (since the faulty face is complex)
                    REQUIRE( !topo.is_boundary( *pitr ) );
                }
            }
            REQUIRE( cpt == 1 );
        }

        for ( auto itr = topo.vertices_begin(); itr != topo.vertices_end(); ++itr )
        {
            REQUIRE( topo.isManifold( *itr ) );
        }
    }
}

TEST_CASE( "Core/Geometry/TopologicalMesh/Initialization",
           "[Core][Core/Geometry][TopologicalMesh]" ) {
    TopologicalMesh topo;
    TopologicalMesh::VertexHandle vhandle[3];
    TopologicalMesh::FaceHandle fhandle;

    vhandle[0] = topo.add_vertex( TopologicalMesh::Point( 1, -1, -1 ) );
    vhandle[1] = topo.add_vertex( TopologicalMesh::Point( 1, -1, 1 ) );
    vhandle[2] = topo.add_vertex( TopologicalMesh::Point( -1, -1, 1 ) );

    std::vector<TopologicalMesh::VertexHandle> face_vhandles;
    face_vhandles.push_back( vhandle[0] );
    face_vhandles.push_back( vhandle[1] );
    face_vhandles.push_back( vhandle[2] );
    fhandle = topo.add_face( face_vhandles );

    // newly created face have invalid wedges on halfedges
    auto heh = topo.halfedge_handle( fhandle );
    REQUIRE( topo.property( topo.getWedgeIndexPph(), heh ).isInvalid() );
    heh = topo.next_halfedge_handle( heh );
    REQUIRE( topo.property( topo.getWedgeIndexPph(), heh ).isInvalid() );
    heh = topo.next_halfedge_handle( heh );
    REQUIRE( topo.property( topo.getWedgeIndexPph(), heh ).isInvalid() );
    REQUIRE( topo.n_faces() == 1 );

    topo.request_face_status();
    topo.delete_face( fhandle, false );
    topo.garbage_collection();
    REQUIRE( topo.n_faces() == 0 );
}

TEST_CASE( "Core/Geometry/TopologicalMesh/MergeWedges", "[Core][Core/Geometry][TopologicalMesh]" ) {

    auto mesh = Ra::Core::Geometry::makeSharpBox();
    auto topo = TopologicalMesh {mesh};

    std::set<TopologicalMesh::WedgeIndex> wedgesIndices;
    for ( auto itr = topo.halfedges_begin(), stop = topo.halfedges_end(); itr != stop; ++itr )
    {
        wedgesIndices.insert( topo.getWedgeIndex( *itr ) );
    }
    // each 8 vertices of the cube has 3 wedges
    REQUIRE( wedgesIndices.size() == 8 * 3 );
    REQUIRE( topo.checkIntegrity() );
    auto wdRef = topo.getWedgeData( topo.getWedgeIndex( *topo.halfedges_begin() ) );
    for ( auto itr = topo.halfedges_begin(), stop = topo.halfedges_end(); itr != stop; ++itr )
    {
        auto wdCur       = topo.getWedgeData( topo.getWedgeIndex( *itr ) );
        auto wdNew       = wdRef;
        wdNew.m_position = wdCur.m_position;
        topo.setWedgeData( topo.getWedgeIndex( *itr ), wdNew );
    }

    wedgesIndices.clear();
    for ( auto itr = topo.halfedges_begin(), stop = topo.halfedges_end(); itr != stop; ++itr )
    {
        wedgesIndices.insert( topo.getWedgeIndex( *itr ) );
    }
    // each 8 vertices of the cube still has 3 wedges
    REQUIRE( wedgesIndices.size() == 8 * 3 );
    REQUIRE( topo.checkIntegrity() );

    topo.mergeEqualWedges();
    wedgesIndices.clear();
    for ( auto itr = topo.halfedges_begin(), stop = topo.halfedges_end(); itr != stop; ++itr )
    {
        wedgesIndices.insert( topo.getWedgeIndex( *itr ) );
    }
    // after merge, each vertex has only on wedge
    REQUIRE( wedgesIndices.size() == 8 );
    REQUIRE( topo.checkIntegrity() );
}

template <typename T>
void testAttrib( const IndexedGeometry<T>& mesh, const std::string& name, float value ) {

    auto attribHandle = mesh.template getAttribHandle<float>( name );
    REQUIRE( attribHandle.idx().isValid() );
    auto& attrib = mesh.getAttrib( attribHandle );
    for ( const auto& v : attrib.data() )
    {
        REQUIRE( v == value );
    }
}

TEST_CASE( "Core/Geometry/TopologicalMesh/Triangulate", "[Core][Core/Geometry][TopologicalMesh]" ) {
    TopologicalMesh topo {};
    TopologicalMesh::VertexHandle vhandle[4];
    TopologicalMesh::FaceHandle fhandle;

    vhandle[0] = topo.add_vertex( TopologicalMesh::Point( -1, -1, 1 ) );
    vhandle[1] = topo.add_vertex( TopologicalMesh::Point( 1, -1, 1 ) );
    vhandle[2] = topo.add_vertex( TopologicalMesh::Point( 1, 1, 1 ) );
    vhandle[3] = topo.add_vertex( TopologicalMesh::Point( -1, 1, 1 ) );

    std::vector<TopologicalMesh::VertexHandle> face_vhandles;
    face_vhandles.push_back( vhandle[0] );
    face_vhandles.push_back( vhandle[1] );
    face_vhandles.push_back( vhandle[2] );
    face_vhandles.push_back( vhandle[3] );
    fhandle = topo.add_face( face_vhandles );

    REQUIRE( topo.n_faces() == 1 );

    auto index1 = topo.addWedgeAttrib<float>( "test1", 1.f );
    REQUIRE( index1 == 0 );
    REQUIRE( topo.getFloatAttribNames().size() == 1 );
    REQUIRE( topo.getFloatAttribNames()[0] == "test1" );

    for ( const auto& he : topo.halfedges() )
    {
        if ( topo.is_boundary( he ) ) continue;

        auto wd = topo.newWedgeData();

        // need to set position and vertex handle for new wedges
        wd.m_vertexHandle = topo.to_vertex_handle( he );
        wd.m_position     = topo.point( wd.m_vertexHandle );

        REQUIRE( wd.m_floatAttrib.size() == 1 );
        wd.m_floatAttrib[index1] = 2.f;
        topo.replaceWedge( he, wd );
    }
    REQUIRE( topo.checkIntegrity() );

    auto index2 = topo.addWedgeAttrib<float>( "test2", 2.f );
    REQUIRE( index2 == 1 );
    for ( const auto& he : topo.halfedges() )
    {
        if ( topo.is_boundary( he ) ) continue;
        // our we require the wedge to be already set for this he
        auto wd = topo.newWedgeData( he );

        REQUIRE( wd.m_vertexHandle == topo.to_vertex_handle( he ) );
        REQUIRE( wd.m_position == topo.point( wd.m_vertexHandle ) );

        REQUIRE( wd.m_floatAttrib.size() == 2 );
        wd.m_floatAttrib[index2] = 3.f;
        topo.replaceWedge( he, wd );
    }

    auto index3 = topo.addWedgeAttrib<float>( "test3", 3.f );
    REQUIRE( index3 == 2 );
    for ( const auto& he : topo.halfedges() )
    {
        if ( topo.is_boundary( he ) ) continue;

        auto wedgeIndex = topo.getWedgeIndex( he );
        REQUIRE( topo.getWedgeRefCount( wedgeIndex ) == 1 );
        auto wedgeData = topo.getWedgeData( wedgeIndex );

        REQUIRE( wedgeData.m_floatAttrib[index1] == 0.f );
        REQUIRE( wedgeData.m_floatAttrib[index2] == 3.f );
        REQUIRE( wedgeData.m_floatAttrib[index3] == 3.f );
    }

    auto poly = topo.toPolyMesh();
    REQUIRE( poly.vertices().size() == 4 );
    REQUIRE( poly.getIndices().size() == 1 );
    REQUIRE( poly.getIndices()[0].size() == 4 );
    testAttrib( poly, "test1", 0.f );
    testAttrib( poly, "test2", 3.f );
    testAttrib( poly, "test3", 3.f );

    topo.triangulate();
    topo.checkIntegrity();
    auto tri = topo.toTriangleMesh();
    REQUIRE( tri.vertices().size() == 4 );
    REQUIRE( tri.getIndices().size() == 2 );
    REQUIRE( tri.getIndices()[0].size() == 3 );
    REQUIRE( tri.getIndices()[1].size() == 3 );
    testAttrib( tri, "test1", 0.f );
    testAttrib( tri, "test2", 3.f );
    testAttrib( tri, "test3", 3.f );
}

optional<TopologicalMesh::HalfedgeHandle>
findHalfedge( TopologicalMesh& topo, const Vector3& from, const Vector3& to ) {
    bool found;
    TopologicalMesh::HalfedgeHandle he;
    for ( auto he_iter = topo.halfedges_begin(); he_iter != topo.halfedges_end(); ++he_iter )
    {

        if ( topo.point( topo.to_vertex_handle( he_iter ) ) == from &&
             topo.point( topo.from_vertex_handle( he_iter ) ) == to )
        {
            found = true;
            he    = *he_iter;
        }
    }
    if ( found ) return he;
    return {};
}

TEST_CASE( "Core/TopologicalMesh/CollapseWedge" ) {
    using namespace Ra::Core;
    using namespace Ra::Core::Utils;
    using namespace Ra::Core::Geometry;
    auto findHalfedge = []( TopologicalMesh& topo,
                            const Vector3& from,
                            const Vector3& to ) -> optional<TopologicalMesh::HalfedgeHandle> {
        bool found;
        TopologicalMesh::HalfedgeHandle he;
        for ( auto he_iter = topo.halfedges_begin(); he_iter != topo.halfedges_end(); ++he_iter )
        {

            if ( topo.point( topo.to_vertex_handle( he_iter ) ) == to &&
                 topo.point( topo.from_vertex_handle( he_iter ) ) == from )
            {
                found = true;
                he    = *he_iter;
            }
        }
        if ( found ) return he;
        return {};
    };

    Vector3Array points1 {
        {00._ra, 00._ra, 00._ra},
        {10._ra, 00._ra, 00._ra},
        {05._ra, 05._ra, 00._ra},
        {05._ra, 10._ra, 00._ra},
        {15._ra, 05._ra, 00._ra},
        {10._ra, 08._ra, 00._ra},
        {10._ra, 12._ra, 00._ra},
        {15._ra, 10._ra, 00._ra},
    };
    Vector3Array points2 = {points1[0], points1[0], points1[1], points1[1], points1[1], points1[2],
                            points1[2], points1[2], points1[2], points1[3], points1[3], points1[3],
                            points1[4], points1[4], points1[5], points1[5], points1[5], points1[5],
                            points1[5], points1[5], points1[6], points1[6], points1[7], points1[7]};

    Vector4Array colors1 = {
        {0_ra, 0_ra, 0_ra, 1_ra},    {1_ra, 1_ra, 1_ra, 1_ra},    {2_ra, 2_ra, 2_ra, 1_ra},
        {3_ra, 3_ra, 3_ra, 1_ra},    {4_ra, 4_ra, 4_ra, 1_ra},    {5_ra, 5_ra, 5_ra, 1_ra},
        {6_ra, 6_ra, 6_ra, 1_ra},    {7_ra, 7_ra, 7_ra, 1_ra},    {8_ra, 8_ra, 8_ra, 1_ra},
        {9_ra, 9_ra, 9_ra, 1_ra},    {10_ra, 10_ra, 10_ra, 1_ra}, {11_ra, 11_ra, 11_ra, 1_ra},
        {12_ra, 12_ra, 12_ra, 1_ra}, {13_ra, 13_ra, 13_ra, 1_ra}, {14_ra, 14_ra, 14_ra, 1_ra},
        {15_ra, 15_ra, 15_ra, 1_ra}, {16_ra, 16_ra, 16_ra, 1_ra}, {17_ra, 17_ra, 17_ra, 1_ra},
        {18_ra, 18_ra, 18_ra, 1_ra}, {19_ra, 19_ra, 19_ra, 1_ra}, {20_ra, 20_ra, 20_ra, 1_ra},
        {21_ra, 21_ra, 21_ra, 1_ra}, {22_ra, 22_ra, 22_ra, 1_ra}, {23_ra, 23_ra, 23_ra, 1_ra},
    };

    Vector3uArray indices1 {
        {0, 2, 1}, {0, 3, 2}, {1, 2, 5}, {2, 3, 5}, {1, 5, 4}, {3, 6, 5}, {5, 6, 7}, {4, 5, 7}};

    Vector3uArray indices3 = {{0, 2, 1}, {1, 2, 5}, {1, 5, 4}, {3, 6, 5}, {5, 6, 7}, {4, 5, 7}};

    Vector3uArray indices4 = {
        {0, 2, 5}, {3, 14, 6}, {4, 12, 15}, {11, 18, 20}, {17, 22, 21}, {16, 13, 23}};

    Vector3uArray indices2 {{0, 5, 2},
                            {1, 9, 8},
                            {3, 6, 14},
                            {7, 10, 19},
                            {4, 15, 12},
                            {11, 20, 18},
                            {17, 21, 22},
                            {16, 23, 13}};
    Vector4Array colors2 {24, Color::White()};
    for ( const auto& face : indices2 )
    {
        colors2[face[0]] = colors1[face[0]];
        colors2[face[1]] = colors1[face[0]];
        colors2[face[2]] = colors1[face[0]];
    }

    Vector4Array colors3 {24, Color::White()};
    std::vector<int> topFaceIndices {1, 3, 5, 6};
    std::vector<int> bottomFaceIndices {0, 2, 4, 7};

    for ( const auto& faceIndex : topFaceIndices )
    {
        colors3[indices2[faceIndex][0]] = colors1[0];
        colors3[indices2[faceIndex][1]] = colors1[0];
        colors3[indices2[faceIndex][2]] = colors1[0];
    }
    for ( const auto& faceIndex : bottomFaceIndices )
    {
        colors3[indices2[faceIndex][0]] = colors1[1];
        colors3[indices2[faceIndex][1]] = colors1[1];
        colors3[indices2[faceIndex][2]] = colors1[1];
    }

    Vector4Array colors4 {24, Color::White()};

    std::vector<std::vector<int>> splitContinuousWedges {// 0
                                                         {0},
                                                         {1},
                                                         // 1
                                                         {2, 3, 4},
                                                         // 2
                                                         {8, 7},
                                                         {5, 6},
                                                         // 3
                                                         {9, 10, 11},
                                                         // 4
                                                         {12, 13},
                                                         // 5
                                                         {14, 15, 16},
                                                         {17, 18, 19},
                                                         // 6
                                                         {20, 21},
                                                         // 7
                                                         {22, 23}};

    for ( size_t i = 0; i < splitContinuousWedges.size(); ++i )
    {
        for ( const auto& widx : splitContinuousWedges[i] )
        {
            colors4[widx] = colors1[i];
        }
    }

    ///\todo add more checks on resulting topology and wedges after collapse
    auto addMergeScene = [findHalfedge]( const Vector3Array& points,
                                         const Vector4Array& colors,
                                         const Vector3uArray& indices,
                                         Vector3 from,
                                         Vector3 to ) {
        TriangleMesh mesh1;
        TopologicalMesh topo1;
        optional<TopologicalMesh::HalfedgeHandle> optHe;

        mesh1.setVertices( points );
        mesh1.addAttrib( "color", Vector4Array {colors.begin(), colors.begin() + points.size()} );
        mesh1.setIndices( indices );

        topo1 = TopologicalMesh {mesh1};
        topo1.mergeEqualWedges();
        topo1.garbage_collection();

        topo1.checkIntegrity();
        optHe = findHalfedge( topo1, from, to );
        REQUIRE( optHe );

        topo1.collapse( *optHe );
        REQUIRE( topo1.checkIntegrity() );

        topo1 = TopologicalMesh {mesh1};
        optHe = findHalfedge( topo1, from, to );
        REQUIRE( optHe );

        topo1.collapse( *optHe, true );
        REQUIRE( topo1.checkIntegrity() );

        std::swap( from, to );

        topo1 = TopologicalMesh {mesh1};
        topo1.mergeEqualWedges();
        topo1.garbage_collection();
        optHe = findHalfedge( topo1, from, to );
        REQUIRE( optHe );

        topo1.collapse( *optHe );
        REQUIRE( topo1.checkIntegrity() );

        topo1 = TopologicalMesh {mesh1};
        optHe = findHalfedge( topo1, from, to );
        REQUIRE( optHe );

        topo1.collapse( *optHe, true );
        REQUIRE( topo1.checkIntegrity() );
    };

    SECTION( "With continuous wedges." ) {
        addMergeScene( points1, colors1, indices1, points1[5], points1[2] );
    }

    SECTION( "with top/bottom wedges" ) {
        addMergeScene( points2, colors3, indices2, points1[5], points1[2] );
    }

    SECTION( "with continuous top/bottom wedges" ) {
        addMergeScene( points2, colors4, indices2, points1[5], points1[2] );
    }
    SECTION( "with flat face wedges" ) {
        addMergeScene( points2, colors2, indices2, points1[5], points1[2] );
    }
    SECTION( "boundary  With continuous wedges." ) {
        addMergeScene( points1, colors1, indices3, points1[5], points1[2] );
    }
    SECTION( "boundary with top/bottom wedges" ) {
        addMergeScene( points2, colors3, indices4, points1[5], points1[2] );
    }
    SECTION( "boundary with continuous top/bottom wedges" ) {
        addMergeScene( points2, colors4, indices4, points1[5], points1[2] );
    }
    SECTION( "boundary with flat face wedges" ) {
        addMergeScene( points2, colors2, indices4, points1[5], points1[2] );
    }

    auto addSplitScene = [findHalfedge]( const Vector3Array& points,
                                         const Vector4Array& colors,
                                         const Vector3uArray& indices,
                                         Vector3 from,
                                         Vector3 to ) {
        TriangleMesh mesh;
        TopologicalMesh topo;
        optional<TopologicalMesh::HalfedgeHandle> optHe;

        mesh.setVertices( points );
        mesh.addAttrib( "color", Vector4Array {colors.begin(), colors.begin() + points.size()} );
        mesh.setIndices( indices );

        topo = TopologicalMesh {mesh};
        topo.mergeEqualWedges();
        topo.garbage_collection();

        for ( int i = 0; i < 2; ++i )
        {
            for ( auto f : {0.25_ra, 0.5_ra, 0.75_ra} )
            {
                topo = TopologicalMesh {mesh};
                topo.mergeEqualWedges();
                optHe   = findHalfedge( topo, from, to );
                auto eh = topo.edge_handle( *optHe );

                auto he0         = topo.halfedge_handle( eh, 0 );
                auto he0boundary = topo.is_boundary( he0 );
                auto he1         = topo.halfedge_handle( eh, 1 );
                auto he1boundary = topo.is_boundary( he1 );
                auto v0          = topo.from_vertex_handle( he0 ); // i.e. to_vertex_handle(he1)
                REQUIRE( v0 == topo.to_vertex_handle( he1 ) );
                auto v1 = topo.to_vertex_handle( he0 );
                auto p0 = topo.point( v0 );
                auto p1 = topo.point( v1 );

                auto widx01 = topo.getWedgeIndex( he0 );
                auto widx00 = topo.getWedgeIndex( topo.prev_halfedge_handle( he0 ) );
                auto widx10 = topo.getWedgeIndex( he1 );
                auto widx11 = topo.getWedgeIndex( topo.prev_halfedge_handle( he1 ) );

                topo.splitEdge( eh, f );

                auto vsplit = topo.to_vertex_handle( he1 ); // i.e. from_vertex_handle(he0)
                REQUIRE( vsplit == topo.from_vertex_handle( he0 ) );

                auto psplit = topo.point( vsplit );
                auto vcheck = ( f * p1 + ( 1.f - f ) * p0 );
                REQUIRE( Math::areApproxEqual( ( psplit - vcheck ).squaredNorm(), 0.f ) );
                REQUIRE( he0boundary == topo.is_boundary( he0 ) );
                REQUIRE( he1boundary == topo.is_boundary( he1 ) );

                ///\todo factorize code
                if ( !he0boundary )
                {
                    auto wd0 = topo.getWedgeData( widx00 );
                    auto wd1 = topo.getWedgeData( widx01 );
                    auto f0  = topo.getWedgeData( widx00 ).m_vector4Attrib[0];
                    auto f1  = topo.getWedgeData( widx01 ).m_vector4Attrib[0];
                    auto wd =
                        topo.getWedgeData( topo.getWedgeIndex( topo.prev_halfedge_handle( he0 ) ) );
                    auto fsplit = wd.m_vector4Attrib[0];
                    auto fcheck = ( f * f1 + ( 1.f - f ) * f0 );
                    REQUIRE( Math::areApproxEqual( ( fsplit - fcheck ).squaredNorm(), 0.f ) );
                    REQUIRE(
                        Math::areApproxEqual( ( psplit - wd.m_position ).squaredNorm(), 0.f ) );
                }
                else
                { REQUIRE( topo.getWedgeIndex( topo.prev_halfedge_handle( he0 ) ).isInvalid() ); }

                if ( !he1boundary )
                {
                    auto wd0    = topo.getWedgeData( widx10 );
                    auto wd1    = topo.getWedgeData( widx11 );
                    auto f0     = topo.getWedgeData( widx10 ).m_vector4Attrib[0];
                    auto f1     = topo.getWedgeData( widx11 ).m_vector4Attrib[0];
                    auto wd     = topo.getWedgeData( topo.getWedgeIndex( he1 ) );
                    auto fsplit = wd.m_vector4Attrib[0];
                    auto fcheck = ( f * f1 + ( 1.f - f ) * f0 );
                    REQUIRE( Math::areApproxEqual( ( fsplit - fcheck ).squaredNorm(), 0.f ) );
                    REQUIRE(
                        Math::areApproxEqual( ( psplit - wd.m_position ).squaredNorm(), 0.f ) );
                }
                else
                { REQUIRE( topo.getWedgeIndex( he1 ).isInvalid() ); }
            }
            std::swap( from, to );
        }
    };

    SECTION( "With continuous wedges." ) {
        addSplitScene( points1, colors1, indices1, points1[5], points1[2] );
    }

    SECTION( "with top/bottom wedges" ) {
        addSplitScene( points2, colors3, indices2, points1[5], points1[2] );
    }

    SECTION( "with continuous top/bottom wedges" ) {
        addSplitScene( points2, colors4, indices2, points1[5], points1[2] );
    }
    SECTION( "with flat face wedges" ) {
        addSplitScene( points2, colors2, indices2, points1[5], points1[2] );
    }
    SECTION( "boundary  With continuous wedges." ) {
        addSplitScene( points1, colors1, indices3, points1[5], points1[2] );
    }
    SECTION( "boundary with top/bottom wedges" ) {
        addSplitScene( points2, colors3, indices4, points1[5], points1[2] );
    }
    SECTION( "boundary with continuous top/bottom wedges" ) {
        addSplitScene( points2, colors4, indices4, points1[5], points1[2] );
    }
    SECTION( "boundary with flat face wedges" ) {
        addSplitScene( points2, colors2, indices4, points1[5], points1[2] );
    }
}
