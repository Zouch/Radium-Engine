#pragma once
#include <Core/Asset/HandleData.hpp>

#include <Core/Utils/Log.hpp>

namespace Ra {
namespace Core {
namespace Asset {

inline void HandleData::setName( const std::string& name ) {
    m_name = name;
}

inline HandleData::HandleType HandleData::getType() const {
    return m_type;
}

inline void HandleData::setType( const HandleType& type ) {
    m_type = type;
}

inline Core::Transform HandleData::getFrame() const {
    return m_frame;
}

inline void HandleData::setFrame( const Core::Transform& frame ) {
    m_frame = frame;
}

inline uint HandleData::getVertexSize() const {
    return m_vertexSize;
}
inline void HandleData::setVertexSize( uint size ) {
    m_vertexSize = size;
}

inline void HandleData::setNameTable( const std::map<std::string, uint>& nameTable ) {
    m_nameTable = nameTable;
}

inline uint HandleData::getComponentDataSize() const {
    return m_component.size();
}

inline const Core::AlignedStdVector<HandleComponentData>& HandleData::getComponentData() const {
    return m_component;
}

inline Core::AlignedStdVector<HandleComponentData>& HandleData::getComponentData() {
    return m_component;
}

inline void
HandleData::setComponents( const Core::AlignedStdVector<HandleComponentData>& components ) {
    const uint size = components.size();
    m_component.resize( size );
#pragma omp parallel for
    for ( int i = 0; i < int( size ); ++i )
    { m_component[i] = components[i]; }
}

inline const HandleComponentData& HandleData::getComponent( const uint i ) const {
    CORE_ASSERT( ( i < m_component.size() ), "Index i out of bound" );
    return m_component[i];
}

inline HandleComponentData& HandleData::getComponent( const uint i ) {
    CORE_ASSERT( ( i < m_component.size() ), "Index i out of bound" );
    return m_component[i];
}

inline const Core::AlignedStdVector<Core::Vector2ui>& HandleData::getEdgeData() const {
    return m_edge;
}

inline Core::AlignedStdVector<Core::Vector2ui>& HandleData::getEdgeData() {
    return m_edge;
}

inline void HandleData::setEdges( const Core::AlignedStdVector<Core::Vector2ui>& edgeList ) {
    const uint size = edgeList.size();
    m_edge.resize( size );
#pragma omp parallel for
    for ( int i = 0; i < int( size ); ++i )
    { m_edge[i] = edgeList[i]; }
}

inline const Core::AlignedStdVector<Core::VectorNui>& HandleData::getFaceData() const {
    return m_face;
}

inline Core::AlignedStdVector<Core::VectorNui>& HandleData::getFaceData() {
    return m_face;
}

inline void HandleData::setFaces( const Core::AlignedStdVector<Core::VectorNui>& faceList ) {
    const uint size = faceList.size();
    m_face.resize( size );
#pragma omp parallel for
    for ( int i = 0; i < int( size ); ++i )
    { m_face[i] = faceList[i]; }
}

inline void HandleData::recomputeAllIndices() {
    m_nameTable.clear();
    for ( uint i = 0; i < getComponentDataSize(); ++i )
    { m_nameTable[m_component[i].m_name] = i; }
}

inline bool HandleData::isPointCloud() const {
    return ( m_type == POINT_CLOUD );
}

inline bool HandleData::isSkeleton() const {
    return ( m_type == SKELETON );
}

inline bool HandleData::isCage() const {
    return ( m_type == CAGE );
}

inline bool HandleData::hasComponents() const {
    return !m_component.empty();
}

inline bool HandleData::hasEdges() const {
    return !m_edge.empty();
}

inline bool HandleData::hasFaces() const {
    return !m_face.empty();
}

inline bool HandleData::needsEndNodes() const {
    return m_endNode;
}

inline int HandleData::getIndexOf( const std::string& name ) const {
    auto it = m_nameTable.find( name );
    if ( it == m_nameTable.end() ) { return -1; }
    return it->second;
}

inline void HandleData::needEndNodes( bool need ) {
    m_endNode = need;
}

inline void HandleData::addBindMesh( const std::string& name ) {
    m_bindMeshes.insert( name );
}

inline const std::set<std::string>& HandleData::getBindMeshes() const {
    return m_bindMeshes;
}

inline void HandleData::displayInfo() const {
    using namespace Core::Utils; // log
    std::string type;
    switch ( m_type )
    {
    case UNKNOWN:
        type = "UNKNOWN";
        break;
    case POINT_CLOUD:
        type = "POINT CLOUD";
        break;
    case SKELETON:
        type = "SKELETON";
        break;
    case CAGE:
        type = "CAGE";
        break;
    }
    LOG( logINFO ) << "======== HANDLE INFO ========";
    LOG( logINFO ) << " Name            : " << m_name;
    LOG( logINFO ) << " Type            : " << type;
    LOG( logINFO ) << " Element #       : " << m_component.size();
    LOG( logINFO ) << " Edge #          : " << m_edge.size();
    LOG( logINFO ) << " Face #          : " << m_face.size();
    LOG( logINFO ) << " Need EndNodes ? : " << ( ( m_endNode ) ? "YES" : "NO" );
}

} // namespace Asset
} // namespace Core
} // namespace Ra
