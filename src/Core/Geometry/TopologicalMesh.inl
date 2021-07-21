#pragma once

#include "TopologicalMesh.hpp"

#include <typeinfo>
#include <unordered_map>

namespace Ra {
namespace Core {
namespace Geometry {

////////////////////////////////////////////////////////////////////////////////
///////////////////      WedgeData                //////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline bool TopologicalMesh::WedgeData::operator==( const TopologicalMesh::WedgeData& lhs ) const {
    return
        // do not have this yet, not sure we need to test them
        // m_inputTriangleMeshIndex == lhs.m_inputTriangleMeshIndex &&
        // m_outputTriangleMeshIndex == lhs.m_outputTriangleMeshIndex &&
        m_position == lhs.m_position && m_floatAttrib == lhs.m_floatAttrib &&
        m_vector2Attrib == lhs.m_vector2Attrib && m_vector3Attrib == lhs.m_vector3Attrib &&
        m_vector4Attrib == lhs.m_vector4Attrib;
}

bool TopologicalMesh::WedgeData::operator!=( const TopologicalMesh::WedgeData& lhs ) const {
    return !( *this == lhs );
}

inline bool TopologicalMesh::WedgeData::operator<( const TopologicalMesh::WedgeData& lhs ) const {

    CORE_ASSERT( ( m_floatAttrib.size() == lhs.m_floatAttrib.size() ) &&
                     ( m_vector2Attrib.size() == lhs.m_vector2Attrib.size() ) &&
                     ( m_vector3Attrib.size() == lhs.m_vector3Attrib.size() ) &&
                     ( m_vector4Attrib.size() == lhs.m_vector4Attrib.size() ),
                 "Could only compare wedge with same number of attributes" );

    {
        int comp = compareVector( m_position, lhs.m_position );
        if ( comp == 2 ) return true;
        if ( comp == 3 ) return false;
    }
    for ( size_t i = 0; i < m_floatAttrib.size(); i++ ) {
        if ( m_floatAttrib[i] < lhs.m_floatAttrib[i] )
            return true;
        else if ( m_floatAttrib[i] > lhs.m_floatAttrib[i] )
            return false;
    }

    for ( size_t i = 0; i < m_vector2Attrib.size(); i++ ) {
        int comp = compareVector( m_vector2Attrib[i], lhs.m_vector2Attrib[i] );
        if ( comp == 2 ) return true;
        if ( comp == 3 ) return false;
    }
    for ( size_t i = 0; i < m_vector3Attrib.size(); i++ ) {
        int comp = compareVector( m_vector3Attrib[i], lhs.m_vector3Attrib[i] );
        if ( comp == 2 ) return true;
        if ( comp == 3 ) return false;
    }
    for ( size_t i = 0; i < m_vector4Attrib.size(); i++ ) {
        int comp = compareVector( m_vector4Attrib[i], lhs.m_vector4Attrib[i] );
        if ( comp == 2 ) return true;
        if ( comp == 3 ) return false;
    }
    return false;
}

#define GET_ATTRIB_ARRAY_HELPER( TYPE, NAME )                                                  \
    template <>                                                                                \
    inline VectorArray<TYPE>& TopologicalMesh::WedgeData::getAttribArray<TYPE>() {             \
        return m_##NAME##Attrib;                                                               \
    }                                                                                          \
    template <>                                                                                \
    inline const VectorArray<TYPE>& TopologicalMesh::WedgeData::getAttribArray<TYPE>() const { \
        return m_##NAME##Attrib;                                                               \
    }

GET_ATTRIB_ARRAY_HELPER( Scalar, float )
GET_ATTRIB_ARRAY_HELPER( Vector2, vector2 )
GET_ATTRIB_ARRAY_HELPER( Vector3, vector3 )
GET_ATTRIB_ARRAY_HELPER( Vector4, vector4 )
#undef GET_ATTRIB_ARRAY_HELPER

template <typename T>
inline VectorArray<T>& TopologicalMesh::WedgeData::getAttribArray() {
    static_assert( sizeof( T ) == -1, "this type is not supported" );
}

// return 1 : equals, 2: strict less, 3: strict greater
template <typename T>
int TopologicalMesh::WedgeData::compareVector( const T& a, const T& b ) {
    for ( int i = 0; i < T::RowsAtCompileTime; i++ ) {
        if ( a[i] < b[i] ) return 2;
        if ( a[i] > b[i] ) return 3;
    }
    // (a == b)
    return 1;
}

////////////////////////////////////////////////////////////////////////////////
///////////////////      Wedge                    //////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// all in class for the moment

////////////////////////////////////////////////////////////////////////////////
///////////////////      WedgeCollection          //////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline void TopologicalMesh::WedgeCollection::del( const TopologicalMesh::WedgeIndex& idx ) {
    if ( idx.isValid() ) m_data[idx].decrementRefCount();
}

inline TopologicalMesh::WedgeIndex
TopologicalMesh::WedgeCollection::newReference( const TopologicalMesh::WedgeIndex& idx ) {
    if ( idx.isValid() ) m_data[idx].incrementRefCount();
    return idx;
}

inline const TopologicalMesh::Wedge&
TopologicalMesh::WedgeCollection::getWedge( const TopologicalMesh::WedgeIndex& idx ) const {
    return m_data[idx];
}

inline const TopologicalMesh::WedgeData&
TopologicalMesh::WedgeCollection::getWedgeData( const WedgeIndex& idx ) const {
    CORE_ASSERT( idx.isValid() && !m_data[idx].isDeleted(),
                 "access to invalid or deleted wedge is prohibited" );

    return m_data[idx].getWedgeData();
}

template <typename T>
inline const T&
TopologicalMesh::WedgeCollection::getWedgeData( const TopologicalMesh::WedgeIndex& idx,
                                                const std::string& name ) const {
    return getWedgeAttrib<T>( idx, name );
}

template <typename T>
inline const T&
TopologicalMesh::WedgeCollection::getWedgeAttrib( const TopologicalMesh::WedgeIndex& idx,
                                                  const std::string& name ) const {
    if ( idx.isValid() ) {
        auto nameArray = getNameArray<T>();
        auto itr       = std::find( nameArray.begin(), nameArray.end(), name );
        if ( itr != nameArray.end() ) {
            auto attrIndex = std::distance( nameArray.begin(), itr );
            return m_data[idx].getWedgeData().getAttribArray<T>()[attrIndex];
        }
        else {
            LOG( logERROR ) << "Warning, set wedge: no wedge attrib named " << name << " of type "
                            << typeid( T ).name();
        }
    }
    static T dummy;
    return dummy;
}

template <typename T>
inline T& TopologicalMesh::WedgeCollection::getWedgeData( const TopologicalMesh::WedgeIndex& idx,
                                                          int attribIndex ) {
    return getWedgeAttrib<T>( idx, attribIndex );
}

template <typename T>
inline T& TopologicalMesh::WedgeCollection::getWedgeAttrib( const TopologicalMesh::WedgeIndex& idx,
                                                            int attribIndex ) {
    return m_data[idx].getWedgeData().getAttribArray<T>()[attribIndex];
}

inline unsigned int
TopologicalMesh::WedgeCollection::getWedgeRefCount( const WedgeIndex& idx ) const {
    CORE_ASSERT( idx.isValid(), "access to invalid or deleted wedge is prohibited" );
    return m_data[idx].getRefCount();
}

inline void TopologicalMesh::WedgeCollection::setWedgeData( const TopologicalMesh::WedgeIndex& idx,
                                                            const TopologicalMesh::WedgeData& wd ) {
    if ( !( wd.m_floatAttrib.size() == m_floatAttribNames.size() &&
            wd.m_vector2Attrib.size() == m_vector2AttribNames.size() &&
            wd.m_vector3Attrib.size() == m_vector3AttribNames.size() &&
            wd.m_vector4Attrib.size() == m_vector4AttribNames.size() ) ) {
        LOG( logWARNING ) << "Warning, topological mesh set wedge: number of attribs inconsistency";
    }
    if ( idx.isValid() ) m_data[idx].setWedgeData( wd );
}

template <typename T>
inline bool TopologicalMesh::WedgeCollection::setWedgeAttrib( TopologicalMesh::WedgeData& wd,
                                                              const std::string& name,
                                                              const T& value ) {
    auto nameArray = getNameArray<T>();
    auto itr       = std::find( nameArray.begin(), nameArray.end(), name );
    if ( itr != nameArray.end() ) {
        auto attrIndex                    = std::distance( nameArray.begin(), itr );
        wd.getAttribArray<T>()[attrIndex] = value;
        return true;
    }
    else {
        LOG( logERROR ) << "Warning, set wedge: no wedge attrib named " << name << " of type "
                        << typeid( T ).name();
    }
    return false;
}

template <typename T>
inline bool
TopologicalMesh::WedgeCollection::setWedgeAttrib( const TopologicalMesh::WedgeIndex& idx,
                                                  const std::string& name,
                                                  const T& value ) {
    if ( idx.isValid() ) {
        auto nameArray = getNameArray<T>();
        auto itr       = std::find( nameArray.begin(), nameArray.end(), name );
        if ( itr != nameArray.end() ) {
            auto attrIndex = std::distance( nameArray.begin(), itr );
            m_data[idx].getWedgeData().getAttribArray<T>()[attrIndex] = value;
            return true;
        }
        else {
            LOG( logERROR ) << "Warning, set wedge: no wedge attrib named " << name << " of type "
                            << typeid( T ).name();
        }
    }
    return false;
}

template <typename T>
inline void
TopologicalMesh::WedgeCollection::setWedgeAttrib( const TopologicalMesh::WedgeIndex& idx,
                                                  const int& attrIndex,
                                                  const T& value ) {
    m_data[idx].getWedgeData().getAttribArray<T>()[attrIndex] = value;
}

template <typename T>
inline int TopologicalMesh::WedgeCollection::getWedgeAttribIndex( const std::string& name ) {
    auto nameArray = getNameArray<T>();
    auto itr       = std::find( nameArray.begin(), nameArray.end(), name );
    if ( itr != nameArray.end() ) { return std::distance( nameArray.begin(), itr ); }
    return 0;
}

inline bool
TopologicalMesh::WedgeCollection::setWedgePosition( const TopologicalMesh::WedgeIndex& idx,
                                                    const Vector3& value ) {
    if ( idx.isValid() ) {
        m_data[idx].getWedgeData().m_position = value;
        return true;
    }
    return false;
}

#define GET_NAME_ARRAY_HELPER( TYPE, NAME )                                                       \
    template <>                                                                                   \
    inline const std::vector<std::string>& TopologicalMesh::WedgeCollection::getNameArray<TYPE>() \
        const {                                                                                   \
        return m_##NAME##AttribNames;                                                             \
    }                                                                                             \
    template <>                                                                                   \
    inline std::vector<std::string>& TopologicalMesh::WedgeCollection::getNameArray<TYPE>() {     \
        return m_##NAME##AttribNames;                                                             \
    }

GET_NAME_ARRAY_HELPER( Scalar, float )
GET_NAME_ARRAY_HELPER( Vector2, vector2 )
GET_NAME_ARRAY_HELPER( Vector3, vector3 )
GET_NAME_ARRAY_HELPER( Vector4, vector4 )

#undef GET_NAME_ARRAY_HELPER
// These template functions are defined above for supported types.
// For unsupported types they simply generate a compile error.
template <typename T>
inline const std::vector<std::string>& TopologicalMesh::WedgeCollection::getNameArray() const {

    LOG( logWARNING ) << "Warning, mesh attribute " << typeid( T ).name()
                      << " is not supported (only float, vec2, vec3 nor vec4 are supported)";
    static_assert( sizeof( T ) == -1, "this type is not supported" );
    return m_floatAttribNames;
}

template <typename T>
inline std::vector<std::string>& TopologicalMesh::WedgeCollection::getNameArray() {

    LOG( logWARNING ) << "Warning, mesh attribute " << typeid( T ).name()
                      << " is not supported (only float, vec2, vec3 nor vec4 are supported)";
    static_assert( sizeof( T ) == -1, "this type is not supported" );
    return m_floatAttribNames;
}
template <typename T>
int TopologicalMesh::WedgeCollection::addAttribName( const std::string& name ) {
    if ( name != std::string( "in_position" ) ) { getNameArray<T>().push_back( name ); }
    return getNameArray<T>().size() - 1;
}

template <typename T>
int TopologicalMesh::WedgeCollection::addAttrib( const std::string& name, const T& value ) {

    auto index = addAttribName<T>( name );
    for ( auto& w : m_data ) {
        CORE_ASSERT( index = w.getWedgeData().getAttribArray<T>().size(),
                     "inconsistent wedge attrib" );
        w.getWedgeData().getAttribArray<T>().push_back( value );
    }
    return index;
}

inline void TopologicalMesh::WedgeCollection::garbageCollection() {
    m_data.erase( std::remove_if( m_data.begin(),
                                  m_data.end(),
                                  []( const Wedge& w ) { return w.isDeleted(); } ),
                  m_data.end() );
}

inline void TopologicalMesh::WedgeCollection::clean() {
    m_data.clear();
    m_floatAttribNames.clear();
    m_vector2AttribNames.clear();
    m_vector3AttribNames.clear();
    m_vector4AttribNames.clear();
    m_wedgeFloatAttribHandles.clear();
    m_wedgeVector2AttribHandles.clear();
    m_wedgeVector3AttribHandles.clear();
    m_wedgeVector4AttribHandles.clear();
}

template <typename T>
void init( VectorArray<T>& vec, const std::vector<std::string> names ) {
    for ( size_t i = 0; i < names.size(); ++i ) {
        vec.emplace_back();
    }
}
// return a new wedgeData with uninit values.
inline TopologicalMesh::WedgeData TopologicalMesh::WedgeCollection::newWedgeData() const {
    WedgeData ret;
    init<Scalar>( ret.getAttribArray<Scalar>(), m_floatAttribNames );
    init<Vector2>( ret.getAttribArray<Vector2>(), m_vector2AttribNames );
    init<Vector3>( ret.getAttribArray<Vector3>(), m_vector3AttribNames );
    init<Vector4>( ret.getAttribArray<Vector4>(), m_vector4AttribNames );
    return ret;
}

inline TopologicalMesh::WedgeData
TopologicalMesh::WedgeCollection::newWedgeData( TopologicalMesh::VertexHandle vh,
                                                TopologicalMesh::Point p ) const {
    WedgeData ret      = newWedgeData();
    ret.m_vertexHandle = vh;
    ret.m_position     = p;
    return ret;
}

////////////////////////////////////////////////////////////////////////////////
///////////////////      InitWedgeProps           //////////////////////////////
////////////////////////////////////////////////////////////////////////////////
inline void
TopologicalMesh::InitWedgeAttribsFromMultiIndexedGeometry::operator()( AttribBase* attr ) const {
    if ( attr->getSize() != m_triMesh.vertices().size() ) {
        LOG( logWARNING ) << "[TopologicalMesh] Skip badly sized attribute " << attr->getName();
    }
    else if ( attr->getName() != std::string( "in_position" ) ) {
        if ( attr->isFloat() ) {
            m_topo->m_wedges.m_wedgeFloatAttribHandles.push_back(
                m_triMesh.template getAttribHandle<Scalar>( attr->getName() ) );
            m_topo->m_wedges.addAttribName<Scalar>( attr->getName() );
        }
        else if ( attr->isVector2() ) {
            m_topo->m_wedges.m_wedgeVector2AttribHandles.push_back(
                m_triMesh.template getAttribHandle<Vector2>( attr->getName() ) );
            m_topo->m_wedges.addAttribName<Vector2>( attr->getName() );
        }
        else if ( attr->isVector3() ) {
            m_topo->m_wedges.m_wedgeVector3AttribHandles.push_back(
                m_triMesh.template getAttribHandle<Vector3>( attr->getName() ) );
            m_topo->m_wedges.addAttribName<Vector3>( attr->getName() );
        }
        else if ( attr->isVector4() ) {
            m_topo->m_wedges.m_wedgeVector4AttribHandles.push_back(
                m_triMesh.template getAttribHandle<Vector4>( attr->getName() ) );
            m_topo->m_wedges.addAttribName<Vector4>( attr->getName() );
        }
        else
            LOG( logWARNING )
                << "Warning, mesh attribute " << attr->getName()
                << " type is not supported (only float, vec2, vec3 nor vec4 are supported)";
    }
}

////////////////////////////////////////////////////////////////////////////////
///////////////////      TopologicalMesh          //////////////////////////////
////////////////////////////////////////////////////////////////////////////////

struct hash_vec {
    std::size_t operator()( const Vector3& lvalue ) const {
        size_t hx = std::hash<Scalar>()( lvalue[0] );
        size_t hy = std::hash<Scalar>()( lvalue[1] );
        size_t hz = std::hash<Scalar>()( lvalue[2] );
        return ( hx ^ ( hy << 1 ) ) ^ hz;
    }
};

template <typename MeshIndex>
TopologicalMesh::TopologicalMesh( const Ra::Core::Geometry::IndexedGeometry<MeshIndex>& mesh ) :
    TopologicalMesh( mesh, DefaultNonManifoldFaceCommand( "[default ctor]" ) ) {}

template <typename MeshIndex, typename NonManifoldFaceCommand>
TopologicalMesh::TopologicalMesh( const IndexedGeometry<MeshIndex>& mesh,
                                  NonManifoldFaceCommand command ) :
    TopologicalMesh() {
    initWithWedge( mesh, mesh.getLayerKey(), command );
}

inline TopologicalMesh::TopologicalMesh(
    const Ra::Core::Geometry::MultiIndexedGeometry& mesh,
    const Ra::Core::Geometry::MultiIndexedGeometry::LayerKeyType& layerKey ) :
    TopologicalMesh( mesh, layerKey, DefaultNonManifoldFaceCommand( "[default ctor]" ) ) {}

template <typename NonManifoldFaceCommand>
TopologicalMesh::TopologicalMesh(
    const Ra::Core::Geometry::MultiIndexedGeometry& mesh,
    const Ra::Core::Geometry::MultiIndexedGeometry::LayerKeyType& layerKey,
    NonManifoldFaceCommand command ) :
    TopologicalMesh() {
    initWithWedge( mesh, layerKey, command );
}

inline void TopologicalMesh::initWithWedge(
    const Ra::Core::Geometry::MultiIndexedGeometry& mesh,
    const Ra::Core::Geometry::MultiIndexedGeometry::LayerKeyType& layerKey ) {
    initWithWedge( mesh,
                   layerKey,
                   DefaultNonManifoldFaceCommand( "[initWithWedges (MultiIndexedGeometry)]" ) );
}

template <typename NonManifoldFaceCommand>
void TopologicalMesh::initWithWedge(
    const Ra::Core::Geometry::MultiIndexedGeometry& mesh,
    const Ra::Core::Geometry::MultiIndexedGeometry::LayerKeyType& layerKey,
    NonManifoldFaceCommand command ) {

    const auto& abstractLayer = mesh.getLayer( layerKey );

    if ( !abstractLayer.hasSemantic( TriangleIndexLayer::staticSemanticName ) &&
         !abstractLayer.hasSemantic( PolyIndexLayer::staticSemanticName ) ) {
        LOG( logWARNING ) << "TopologicalMesh: mesh does not contains faces. Aborting conversion";
        return;
    }

    clean();

    LOG( logINFO ) << "TopologicalMesh: load mesh with "
                   //<< abstractLayer.size()
                   << " faces and " << mesh.vertices().size() << " vertices.";
    // use a hashmap for fast search of existing vertex position
    using VertexMap = std::unordered_map<Vector3, TopologicalMesh::VertexHandle, hash_vec>;
    VertexMap vertexHandles;

    // loop over all attribs and build correspondance pair
    mesh.vertexAttribs().for_each_attrib( InitWedgeAttribsFromMultiIndexedGeometry { this, mesh } );

    for ( size_t i = 0; i < mesh.vertices().size(); ++i ) {
        // create an empty wedge, with 0 ref
        Wedge w;

        WedgeData wd;
        wd.m_position = mesh.vertices()[i];
        copyMeshToWedgeData( mesh,
                             i,
                             m_wedges.m_wedgeFloatAttribHandles,
                             m_wedges.m_wedgeVector2AttribHandles,
                             m_wedges.m_wedgeVector3AttribHandles,
                             m_wedges.m_wedgeVector4AttribHandles,
                             &wd );
        // here ref is not incremented
        w.setWedgeData( std::move( wd ) );
        // the newly added wedge is not referenced yet, will be done with `newReference` when
        // creating faces just below
        m_wedges.m_data.push_back( w );
    }

    LOG( logINFO ) << "TopologicalMesh: have  " << m_wedges.size() << " wedges ";

    const bool hasNormals = !mesh.normals().empty();
    if ( !hasNormals ) {
        release_face_normals();
        release_vertex_normals();
        release_halfedge_normals();
    }

    command.initialize( mesh );

    auto processFaces = [&mesh, &vertexHandles, this, hasNormals, &command]( const auto& faces ) {
        size_t num_triangles = faces.size();
        for ( unsigned int i = 0; i < num_triangles; i++ ) {
            const auto& face      = faces[i];
            const size_t num_vert = face.size();
            std::vector<TopologicalMesh::VertexHandle> face_vhandles( num_vert );
            std::vector<TopologicalMesh::Normal> face_normals( num_vert );
            std::vector<WedgeIndex> face_wedges( num_vert );

            for ( size_t j = 0; j < num_vert; ++j ) {
                unsigned int inMeshVertexIndex = face[j];
                const Vector3& p               = mesh.vertices()[inMeshVertexIndex];

                typename VertexMap::iterator vtr = vertexHandles.find( p );
                TopologicalMesh::VertexHandle vh;
                if ( vtr == vertexHandles.end() ) {
                    vh = add_vertex( p );
                    vertexHandles.insert( vtr, typename VertexMap::value_type( p, vh ) );
                }
                else {
                    vh = vtr->second;
                }

                face_vhandles[j] = vh;
                if ( hasNormals ) face_normals[j] = mesh.normals()[inMeshVertexIndex];
                face_wedges[j] = WedgeIndex { inMeshVertexIndex };
                m_wedges.m_data[inMeshVertexIndex].getWedgeData().m_vertexHandle = vh;
            }

            // remove consecutive equal vertex
            // first take care of "loop" if begin == *end-1
            // apply the same modifications on wedges and normals
            // e.g. 1 2 1 becomes 1 2
            {
                auto begin = face_vhandles.begin();
                if ( face_vhandles.size() > 2 ) {
                    auto end       = face_vhandles.end() - 1;
                    auto wedgeEnd  = face_wedges.end() - 1;
                    auto normalEnd = face_normals.end() - 1;

                    while ( begin != end && *begin == *end ) {
                        end--;
                        wedgeEnd--;
                        normalEnd--;
                    }
                    face_vhandles.erase( end + 1, face_vhandles.end() );
                    face_wedges.erase( wedgeEnd + 1, face_wedges.end() );
                    face_normals.erase( normalEnd + 1, face_normals.end() );
                }
            }
            // then remove duplicates
            // e.g. 1 2 2 becomes 1 2
            // equiv of
            // face_vhandles.erase( std::unique( face_vhandles.begin(), face_vhandles.end() ),
            //                     face_vhandles.end() );
            // but handles wedges and normals
            // see (https://en.cppreference.com/w/cpp/algorithm/unique)
            {
                auto first       = face_vhandles.begin();
                auto wedgeFirst  = face_wedges.begin();
                auto normalFirst = face_normals.begin();
                auto last        = face_vhandles.end();

                if ( first != last ) {
                    auto result       = first;
                    auto wedgeResult  = wedgeFirst;
                    auto normalResult = normalFirst;
                    while ( ++first != last ) {
                        if ( !( *result == *first ) ) {
                            ++result;
                            ++wedgeResult;
                            ++normalResult;
                            if ( result != first ) {
                                *result       = std::move( *first );
                                *wedgeResult  = std::move( *wedgeFirst );
                                *normalResult = std::move( *normalFirst );
                            }
                        }
                    }
                    face_vhandles.erase( result + 1, face_vhandles.end() );
                    face_wedges.erase( wedgeResult + 1, face_wedges.end() );
                    face_normals.erase( normalResult + 1, face_normals.end() );
                }
            }

            ///\todo and "cross face ?"
            // unique sort size == vhandles size, if not split ...

            TopologicalMesh::FaceHandle fh;
            // skip 2 vertex face
            if ( face_vhandles.size() > 2 ) fh = add_face( face_vhandles );

            // In case of topological inconsistancy, face will be invalid (or uninitialized <>
            // invalid)
            if ( fh.is_valid() ) {
                for ( size_t vindex = 0; vindex < face_vhandles.size(); vindex++ ) {
                    TopologicalMesh::HalfedgeHandle heh =
                        halfedge_handle( face_vhandles[vindex], fh );
                    if ( hasNormals ) set_normal( heh, face_normals[vindex] );
                    property( m_wedgeIndexPph, heh ) = m_wedges.newReference( face_wedges[vindex] );
                }
            }
            else {
                command.process( face_vhandles );
            }
            face_vhandles.clear();
            face_normals.clear();
        }
    };

    if ( abstractLayer.hasSemantic( TriangleIndexLayer::staticSemanticName ) ) {
        const auto& faces = static_cast<const TriangleIndexLayer&>( abstractLayer ).collection();
        LOG( logINFO ) << "TopologicalMesh: process " << faces.size() << " triangular faces ";
        processFaces( faces );
    }
    else if ( abstractLayer.hasSemantic( PolyIndexLayer::staticSemanticName ) ) {
        const auto& faces = static_cast<const PolyIndexLayer&>( abstractLayer ).collection();
        LOG( logINFO ) << "TopologicalMesh: process " << faces.size() << " polygonal faces ";
        processFaces( faces );
    }

    command.postProcess( *this );
    if ( hasNormals ) {
        m_normalsIndex = m_wedges.getWedgeAttribIndex<Normal>( "in_normal" );

        m_vertexFaceWedgesWithSameNormals.clear();
        m_vertexFaceWedgesWithSameNormals.resize( n_vertices() );

        for ( auto itr = vertices_begin(), stop = vertices_end(); itr != stop; ++itr ) {
            std::unordered_map<TopologicalMesh::Normal,
                               std::pair<std::set<FaceHandle>, std::set<WedgeIndex>>,
                               hash_vec>
                normalSharedByWedges;

            auto vh = *itr;

            for ( ConstVertexIHalfedgeIter vh_it = cvih_iter( vh ); vh_it.is_valid(); ++vh_it ) {
                const auto& widx = property( m_wedgeIndexPph, *vh_it );
                if ( widx.isValid() && !m_wedges.getWedge( widx ).isDeleted() ) {
                    auto oldNormal = m_wedges.getWedgeData<Normal>( widx, m_normalsIndex );
                    normalSharedByWedges[oldNormal].first.insert( face_handle( *vh_it ) );
                    normalSharedByWedges[oldNormal].second.insert( widx );
                }
            }

            for ( const auto& pair : normalSharedByWedges ) {
                for ( const auto& fh : pair.second.first ) {
                    auto& v = m_vertexFaceWedgesWithSameNormals[vh.idx()][fh.idx()];
                    v.insert( v.end(), pair.second.second.begin(), pair.second.second.end() );
                }
            }
        }
    }
    LOG( logINFO ) << "TopologicalMesh: load end with  " << m_wedges.size() << " wedges ";
}

template <typename T>
void TopologicalMesh::copyAttribToWedgeData( const MultiIndexedGeometry& mesh,
                                             unsigned int vindex,
                                             const std::vector<AttribHandle<T>>& attrHandleVec,
                                             VectorArray<T>* to ) {
    for ( auto handle : attrHandleVec ) {
        auto& attr = mesh.template getAttrib<T>( handle );
        to->push_back( attr.data()[vindex] );
    }
}

void TopologicalMesh::copyMeshToWedgeData( const MultiIndexedGeometry& mesh,
                                           unsigned int vindex,
                                           const std::vector<AttribHandle<Scalar>>& wprop_float,
                                           const std::vector<AttribHandle<Vector2>>& wprop_vec2,
                                           const std::vector<AttribHandle<Vector3>>& wprop_vec3,
                                           const std::vector<AttribHandle<Vector4>>& wprop_vec4,
                                           TopologicalMesh::WedgeData* wd ) {
    copyAttribToWedgeData( mesh, vindex, wprop_float, &wd->m_floatAttrib );
    copyAttribToWedgeData( mesh, vindex, wprop_vec2, &wd->m_vector2Attrib );
    copyAttribToWedgeData( mesh, vindex, wprop_vec3, &wd->m_vector3Attrib );
    copyAttribToWedgeData( mesh, vindex, wprop_vec4, &wd->m_vector4Attrib );
}

inline void TopologicalMesh::propagate_normal_to_wedges( VertexHandle vh ) {
    if ( !has_halfedge_normals() ) {
        LOG( logERROR ) << "TopologicalMesh has no normals, nothing set";
        return;
    }
    for ( VertexIHalfedgeIter vih_it = vih_iter( vh ); vih_it.is_valid(); ++vih_it ) {
        auto wd = getWedgeData( property( getWedgeIndexPph(), *vih_it ) );

        m_wedges.setWedgeAttrib( wd, "in_normal", normal( vh ) );

        replaceWedge( *vih_it, wd );
    }
}

inline TopologicalMesh::HalfedgeHandle TopologicalMesh::halfedge_handle( VertexHandle vh,
                                                                         FaceHandle fh ) const {
    for ( ConstVertexIHalfedgeIter vih_it = cvih_iter( vh ); vih_it.is_valid(); ++vih_it ) {
        if ( face_handle( *vih_it ) == fh ) { return *vih_it; }
    }
    CORE_ASSERT( false, "vh is not a vertex of face fh" );
    return HalfedgeHandle();
}

inline const OpenMesh::HPropHandleT<TopologicalMesh::Index>&
TopologicalMesh::getInputTriangleMeshIndexPropHandle() const {
    return m_inputTriangleMeshIndexPph;
}

inline const OpenMesh::HPropHandleT<TopologicalMesh::Index>&
TopologicalMesh::getOutputTriangleMeshIndexPropHandle() const {
    return m_outputTriangleMeshIndexPph;
}

inline std::set<TopologicalMesh::WedgeIndex>
TopologicalMesh::getVertexWedges( OpenMesh::VertexHandle vh ) const {
    std::set<TopologicalMesh::WedgeIndex> ret;

    for ( ConstVertexIHalfedgeIter vh_it = cvih_iter( vh ); vh_it.is_valid(); ++vh_it ) {
        auto widx = property( m_wedgeIndexPph, *vh_it );
        if ( widx.isValid() && !m_wedges.getWedge( widx ).isDeleted() ) ret.insert( widx );
    }
    return ret;
}

inline TopologicalMesh::WedgeIndex
TopologicalMesh::getWedgeIndex( OpenMesh::HalfedgeHandle heh ) const {
    return property( getWedgeIndexPph(), heh );
}

inline unsigned int TopologicalMesh::getWedgeRefCount( const WedgeIndex& idx ) const {
    return m_wedges.getWedgeRefCount( idx );
}

template <typename T>
inline bool TopologicalMesh::setWedgeData( const TopologicalMesh::WedgeIndex& idx,
                                           const std::string& name,
                                           const T& value ) {
    return setWedgeAttrib( idx, name, value );
}

template <typename T>
inline bool TopologicalMesh::setWedgeAttrib( const TopologicalMesh::WedgeIndex& idx,
                                             const std::string& name,
                                             const T& value ) {
    return m_wedges.setWedgeAttrib( idx, name, value );
}

inline void TopologicalMesh::setWedgeData( TopologicalMesh::WedgeIndex widx,
                                           const TopologicalMesh::WedgeData& wedge ) {
    m_wedges.setWedgeData( widx, wedge );
}

inline const TopologicalMesh::WedgeData&
TopologicalMesh::getWedgeData( const WedgeIndex& idx ) const {
    return m_wedges.getWedgeData( idx );
}

template <typename T>
inline const T& TopologicalMesh::getWedgeData( const TopologicalMesh::WedgeIndex& idx,
                                               const std::string& name ) const {
    return getWedgeAttrib<T>( idx, name );
}

template <typename T>
inline const T& TopologicalMesh::getWedgeAttrib( const TopologicalMesh::WedgeIndex& idx,
                                                 const std::string& name ) const {
    return m_wedges.getWedgeData<T>( idx, name );
}

inline void TopologicalMesh::replaceWedge( OpenMesh::HalfedgeHandle he, const WedgeData& wd ) {
    m_wedges.del( property( getWedgeIndexPph(), he ) );
    property( getWedgeIndexPph(), he ) = m_wedges.add( wd );
}

inline void TopologicalMesh::replaceWedgeIndex( OpenMesh::HalfedgeHandle he,
                                                const WedgeIndex& widx ) {
    m_wedges.del( property( getWedgeIndexPph(), he ) );
    property( getWedgeIndexPph(), he ) = m_wedges.newReference( widx );
}

inline void TopologicalMesh::mergeEqualWedges() {
    for ( auto itr = vertices_begin(), stop = vertices_end(); itr != stop; ++itr ) {
        mergeEqualWedges( *itr );
    }
}

inline void TopologicalMesh::mergeEqualWedges( OpenMesh::VertexHandle vh ) {
    for ( auto itr = vih_iter( vh ); itr.is_valid(); ++itr ) {
        // replace will search if wedge already present and use it, so merge occurs.
        if ( !is_boundary( *itr ) )
            replaceWedge( *itr, getWedgeData( property( getWedgeIndexPph(), *itr ) ) );
    }
}

inline const std::vector<std::string>& TopologicalMesh::getVec4AttribNames() const {
    return m_wedges.m_vector4AttribNames;
}
inline const std::vector<std::string>& TopologicalMesh::getVec3AttribNames() const {
    return m_wedges.m_vector3AttribNames;
}
inline const std::vector<std::string>& TopologicalMesh::getVec2AttribNames() const {
    return m_wedges.m_vector2AttribNames;
}
inline const std::vector<std::string>& TopologicalMesh::getFloatAttribNames() const {
    return m_wedges.m_floatAttribNames;
}

inline bool TopologicalMesh::isFeatureVertex( const VertexHandle& vh ) const {
    return getVertexWedges( vh ).size() != 1;
}

inline bool TopologicalMesh::isFeatureEdge( const EdgeHandle& eh ) const {
    auto heh0 = halfedge_handle( eh, 0 );
    auto heh1 = halfedge_handle( eh, 1 );

    return property( m_wedgeIndexPph, heh0 ) !=
               property( m_wedgeIndexPph,
                         prev_halfedge_handle( opposite_halfedge_handle( heh0 ) ) ) ||
           property( m_wedgeIndexPph, heh1 ) !=
               property( m_wedgeIndexPph,
                         prev_halfedge_handle( opposite_halfedge_handle( heh1 ) ) );
}

inline const OpenMesh::HPropHandleT<TopologicalMesh::WedgeIndex>&
TopologicalMesh::getWedgeIndexPph() const {
    return m_wedgeIndexPph;
}

} // namespace Geometry
} // namespace Core
} // namespace Ra
