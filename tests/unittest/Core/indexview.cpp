#include <Core/Geometry/IndexedGeometry.hpp>
#include <Core/Geometry/MeshPrimitives.hpp>
#include <catch2/catch.hpp>

struct CustomTriangleIndexLayer : public Ra::Core::Geometry::TriangleIndexLayer {
    inline CustomTriangleIndexLayer() :
        Ra::Core::Geometry::TriangleIndexLayer( staticSemanticName ) {}
    static constexpr const char* staticSemanticName = "CustomSemantic";
};

TEST_CASE( "Core/Geometry/IndexedGeometry", "[Core][Core/Geometry][IndexedGeometry]" ) {
    using Ra::Core::Vector3;
    using Ra::Core::Geometry::AttribArrayGeometry;
    using Ra::Core::Geometry::MultiIndexedGeometry;
    using Ra::Core::Geometry::PointCloudIndexLayer;
    using Ra::Core::Geometry::TriangleIndexLayer;
    using Ra::Core::Geometry::TriangleMesh;
    using Ra::Core::Utils::ObjectWithSemantic;

    // Store keys of the layers that should be in the geometry
    std::set<MultiIndexedGeometry::LayerKeyType> keys;

    TriangleMesh mesh = Ra::Core::Geometry::makeBox();

    // copy AttribArrayGeometry;
    MultiIndexedGeometry geo( mesh );

    // Create triangle Layer
    ObjectWithSemantic::SemanticNameCollection tilSemantics;
    {
        // TriangleMesh is a MultiIndexedGeometry, so the layer has already
        // been added
        REQUIRE( geo.containsLayer( TriangleIndexLayer::staticSemanticName ) );
        auto [key, layer] = geo.getFirstLayerOccurrence( TriangleIndexLayer::staticSemanticName );
        keys.insert( key );
        tilSemantics = layer.semantics();
    }

    ObjectWithSemantic::SemanticNameCollection pilSemantics;
    {
        //! [Creating and adding pointcloud layer]
        auto pil = std::make_unique<PointCloudIndexLayer>();
        // fill indices as linspace
        pil->linearIndices( geo );
        // optional: save semantics for later
        pilSemantics = pil->semantics();
        // insert with default name
        bool layerAdded = geo.addLayer( std::move( pil ) );
        //! [Creating and adding pointcloud layer]
        REQUIRE( layerAdded );

        keys.insert( { pilSemantics, "" } );
    }

    REQUIRE( geo.containsLayer( tilSemantics ) );
    REQUIRE( geo.containsLayer( pilSemantics ) );
    REQUIRE( geo.containsLayer( TriangleIndexLayer::staticSemanticName ) );
    REQUIRE( geo.containsLayer( PointCloudIndexLayer::staticSemanticName ) );

    REQUIRE( geo.countLayers( tilSemantics ) == 1 );
    REQUIRE( geo.countLayers( pilSemantics ) == 1 );
    REQUIRE( geo.countLayers( TriangleIndexLayer::staticSemanticName ) == 1 );
    REQUIRE( geo.countLayers( PointCloudIndexLayer::staticSemanticName ) == 1 );

    ObjectWithSemantic::SemanticNameCollection cilSemantics;
    auto cil          = std::make_unique<CustomTriangleIndexLayer>();
    cil->collection() = mesh.getIndices();
    cilSemantics      = cil->semantics();

    REQUIRE( !geo.containsLayer( cilSemantics ) );
    REQUIRE( geo.countLayers( cilSemantics ) == 0 );

    REQUIRE( geo.addLayer( std::move( cil ) ) );
    keys.insert( { cilSemantics, "" } );

    REQUIRE( geo.containsLayer( cilSemantics ) );
    REQUIRE( geo.containsLayer( TriangleIndexLayer::staticSemanticName ) );
    REQUIRE( geo.containsLayer( PointCloudIndexLayer::staticSemanticName ) );
    REQUIRE( geo.containsLayer( CustomTriangleIndexLayer::staticSemanticName ) );

    REQUIRE( geo.countLayers( tilSemantics ) == 1 );
    REQUIRE( geo.countLayers( pilSemantics ) == 1 );
    REQUIRE( geo.countLayers( cilSemantics ) == 1 );
    REQUIRE( geo.countLayers( TriangleIndexLayer::staticSemanticName ) == 2 );
    REQUIRE( geo.countLayers( PointCloudIndexLayer::staticSemanticName ) == 1 );
    REQUIRE( geo.countLayers( CustomTriangleIndexLayer::staticSemanticName ) == 1 );

    // Check layer keys iterator: we should traverse all keys
    REQUIRE( keys.size() != 0 );
    //! [Iterating over layer keys]
    for ( const auto& k : geo.layerKeys() )
    {
        REQUIRE( keys.erase( k ) == 1 );
        REQUIRE( geo.countLayers( k ) == 1 );
    }
    //! [Iterating over layer keys]
    REQUIRE( keys.size() == 0 );
}

TEST_CASE( "Core/Geometry/IndexedGeometry/Attributes", "[Core][Core/Geometry][IndexedGeometry]" ) {
    using Ra::Core::Vector3;
    using Ra::Core::Geometry::AttribArrayGeometry;
    using Ra::Core::Geometry::MultiIndexedGeometry;
    using Ra::Core::Geometry::PointCloudIndexLayer;
    using Ra::Core::Geometry::TriangleIndexLayer;
    using Ra::Core::Geometry::TriangleMesh;
    using Ra::Core::Utils::ObjectWithSemantic;
    using Vec3AttribHandle = AttribArrayGeometry::Vec3AttribHandle;

    MultiIndexedGeometry mesh( Ra::Core::Geometry::makeBox() );

    // base attributes are automatically added
    auto h_pos = mesh.getAttribHandle<Vector3>( "in_position" );
    REQUIRE( mesh.isValid( h_pos ) );
    auto h_nor = mesh.getAttribHandle<Vector3>( "in_normal" );
    REQUIRE( mesh.isValid( h_nor ) );

    // Add/Remove attributes without filling it
    auto handleEmpty = mesh.addAttrib<Vec3AttribHandle::value_type>( "empty" );
    mesh.removeAttrib( handleEmpty );
    REQUIRE( !mesh.isValid( handleEmpty ) );
    handleEmpty = mesh.addAttrib<Vec3AttribHandle::value_type>( "empty" );
    REQUIRE( mesh.isValid( handleEmpty ) );
    mesh.removeAttrib( handleEmpty );
    handleEmpty = mesh.getAttribHandle<Vec3AttribHandle::value_type>( "empty" );
    REQUIRE( !mesh.isValid( handleEmpty ) );

    // Test access to the attribute container
    auto handleFilled     = mesh.addAttrib<Vec3AttribHandle::value_type>( "filled" );
    auto& attribFilled    = mesh.getAttrib( handleFilled );
    auto& containerFilled = attribFilled.getDataWithLock();
    REQUIRE( attribFilled.isLocked() );

    // Test filling and removing vec3 attributes
    for ( int i = 0; i != int( mesh.vertices().size() ); ++i )
        containerFilled.push_back( Vec3AttribHandle::value_type::Random() );
    attribFilled.unlock();

    auto handleFilled2     = mesh.getAttribHandle<Vec3AttribHandle::value_type>( "filled" );
    auto& containerFilled2 = mesh.getAttrib( handleFilled2 ).data();
    REQUIRE( containerFilled == containerFilled2 );

    mesh.removeAttrib( handleFilled );

    // Test attribute creation by type, filling and removal
    auto handle      = mesh.addAttrib<Eigen::Matrix<unsigned int, 1, 1>>( "filled2" );
    auto& container3 = mesh.getAttrib( handle ).getDataWithLock();
    using HandleType = decltype( handle );

    for ( int i = 0; i != int( mesh.vertices().size() ); ++i )
        container3.push_back( typename HandleType::value_type( i ) );
    mesh.getAttrib( handle ).unlock();
    mesh.removeAttrib( handle );

    // Test dummy handle
    auto invalid = mesh.getAttribHandle<float>( "toto" );
    REQUIRE( !mesh.isValid( invalid ) );

    // Test attribute copy
    const auto v0 = mesh.vertices()[0];
    TriangleMesh meshCopy;
    meshCopy.copy( mesh );
    REQUIRE( mesh.vertices()[0].isApprox( v0 ) );
    meshCopy.verticesWithLock()[0] += Ra::Core::Vector3( 0.5, 0.5, 0.5 );
    meshCopy.verticesUnlock();
    REQUIRE( !meshCopy.vertices()[0].isApprox( v0 ) );

    // For the documentation in doc/developer/mesh.md
    //! [create TriangleMesh]
    using Ra::Core::Geometry::TriangleMesh;
    TriangleMesh m;
    TriangleMesh::PointAttribHandle::Container vertices;
    TriangleMesh::NormalAttribHandle::Container normals;
    TriangleMesh::IndexContainerType indices;

    vertices.push_back( { 0, 0, 0 } );
    vertices.push_back( { 1, 0, 0 } );
    vertices.push_back( { 0, 2, 0 } );
    normals.push_back( { 0, 0, 1 } );
    normals.push_back( { 0, 0, 1 } );
    normals.push_back( { 0, 0, 1 } );

    m.setVertices( std::move( vertices ) );
    m.setNormals( std::move( normals ) );

    m.setIndices( { { 0, 1, 2 } } );

    auto handle1  = m.addAttrib<Vector3>( "vector3_attrib" );
    auto& attrib1 = m.getAttrib( handle1 );
    auto& buf     = attrib1.getDataWithLock();

    buf.reserve( 3 );
    buf.push_back( { 1, 1, 1 } );
    buf.push_back( { 2, 2, 2 } );
    buf.push_back( { 3, 3, 3 } );
    attrib1.unlock();

    auto handle2  = m.addAttrib<float>( "float_attrib" );
    auto& attrib2 = m.getAttrib( handle2 );
    attrib2.setData( { 1.f, 2.f, 3.f } );

    TriangleMesh m2;
    m2.copyBaseGeometry( m );
    m2.copyAttributes( m, handle1 );
    //! [create TriangleMesh]

    m2.copyAttributes( m, handle2 );

    auto& attribM2_1 = m2.getAttrib( handle1 );
    auto& attribM2_2 = m2.getAttrib( handle2 );
    REQUIRE( attribM2_1.getSize() == 3 );
    REQUIRE( attribM2_1.getElementSize() == 3 );
    REQUIRE( attribM2_1.getStride() == sizeof( Vector3 ) );
    REQUIRE( attribM2_1.getBufferSize() == 3 * sizeof( Vector3 ) );
    attribM2_1.resize( 10 );
    REQUIRE( attribM2_1.getSize() == 10 );
    REQUIRE( attribM2_1.getElementSize() == 3 );
    REQUIRE( attribM2_1.getStride() == sizeof( Vector3 ) );
    REQUIRE( attribM2_1.getBufferSize() == 10 * sizeof( Vector3 ) );
    REQUIRE( attribM2_2.getSize() == 3 );
    REQUIRE( attribM2_2.getElementSize() == 1 );
    REQUIRE( attribM2_2.getStride() == sizeof( float ) );
    REQUIRE( attribM2_2.getBufferSize() == 3 * sizeof( float ) );

    auto& attrMgr = m2.vertexAttribs();

    REQUIRE( attrMgr.getAttribPtr( handle1 ) == attrMgr.getAttribBase( attribM2_1.getName() ) );
    REQUIRE( nullptr == attrMgr.getAttribBase( "unkown" ) );

    int cpt = 0;
    attrMgr.for_each_attrib(
        [&cpt, &attribM2_1, &attribM2_2, &attrMgr]( Ra::Core::Utils::AttribBase* b ) {
            cpt++;
            // 3 since we want to skip position and normals
            if ( cpt == 3 )
            {
                auto& t        = b->cast<Vector3>();
                const void* p1 = t.dataPtr();
                const void* p2 = attribM2_1.dataPtr();
                REQUIRE( p1 == p2 );
            }
            if ( cpt == 4 )
            {
                // const to check const cast;
                const Ra::Core::Utils::AttribBase* cb   = b;
                const Ra::Core::Utils::Attrib<float>& t = cb->cast<float>();
                REQUIRE( t.dataPtr() == attribM2_2.dataPtr() );
            }
        } );
    REQUIRE( cpt == attrMgr.getNumAttribs() );
    const Ra::Core::Utils::AttribHandle<float>::Container newData { 0.f, 1.f, 2.f };
    attrMgr.setAttrib( handle2, newData );
    REQUIRE( m2.getAttrib( handle2 ).data() == newData );
}

TEST_CASE( "Core/Geometry/IndexedGeometry/CopyAllAttributes",
           "[Core][Core/Geometry][IndexedGeometry]" ) {
    using Ra::Core::Vector2;
    using Ra::Core::Vector3;
    using Ra::Core::Geometry::TriangleMesh;
    using Vec3AttribHandle = Ra::Core::Utils::AttribHandle<Vector3>;
    using Ra::Core::Geometry::TriangleMesh;

    TriangleMesh m;
    TriangleMesh::PointAttribHandle::Container vertices;
    TriangleMesh::NormalAttribHandle::Container normals;
    TriangleMesh::IndexContainerType indices;

    vertices.push_back( { 0, 0, 0 } );
    vertices.push_back( { 1, 0, 0 } );
    vertices.push_back( { 0, 2, 0 } );
    normals.push_back( { 0, 0, 1 } );
    normals.push_back( { 0, 0, 1 } );
    normals.push_back( { 0, 0, 1 } );

    m.setVertices( std::move( vertices ) );
    m.setNormals( std::move( normals ) );

    m.setIndices( { { 0, 1, 2 } } );

    auto handle1  = m.addAttrib<Vector2>( "vector2_attrib" );
    auto& attrib1 = m.getAttrib( handle1 );
    auto& buf1    = attrib1.getDataWithLock();

    buf1.reserve( 3 );
    buf1.push_back( { 1, 1 } );
    buf1.push_back( { 2, 2 } );
    buf1.push_back( { 3, 3 } );
    attrib1.unlock();

    auto handle2  = m.addAttrib<Scalar>( "float_attrib" );
    auto& attrib2 = m.getAttrib( handle2 );
    attrib2.setData( { 1.f, 2.f, 3.f } );

    using Vector5 = Eigen::Matrix<Scalar, 5, 1>;
    auto handle3  = m.addAttrib<Vector5>( "vector5_attrib" );
    auto& attrib3 = m.getAttrib( handle3 );
    auto& buf3    = attrib3.getDataWithLock();

    buf3.reserve( 3 );
    for ( int val = 1; val <= 3; ++val )
    {
        Vector5 v;
        v << val, val, val, val, val;
        buf3.push_back( v );
    }
    attrib3.unlock();

    TriangleMesh m3;

    m3.copyBaseGeometry( m );

    // this one do not copy Vector5 attrib
    m3.copyAllAttributes( m );

    REQUIRE( m3.getAttribHandle<Vector3>( attrib1.getName() ).idx().isValid() );
    REQUIRE( m3.getAttribHandle<Scalar>( attrib2.getName() ).idx().isValid() );
    REQUIRE( m3.getAttribHandle<Vector5>( attrib3.getName() ).idx().isInvalid() );
    REQUIRE( !m3.vertexAttribs().hasSameAttribs( m.vertexAttribs() ) );
    REQUIRE( !m.vertexAttribs().hasSameAttribs( m3.vertexAttribs() ) );

    // but we can copy it explicitly
    auto handleM3 = m3.addAttrib( "vector5_attrib", m.getAttrib( handle3 ).data() );

    REQUIRE( m3.vertexAttribs().hasSameAttribs( m.vertexAttribs() ) );
    REQUIRE( m.vertexAttribs().hasSameAttribs( m3.vertexAttribs() ) );

    m3.getAttribBase( "vector5_attrib" )->setName( "better" );
    REQUIRE( m3.getAttrib( handleM3 ).getName() == "better" );
}
