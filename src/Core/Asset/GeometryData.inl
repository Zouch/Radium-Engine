#pragma once
#include <Core/Asset/GeometryData.hpp>

#include <algorithm> //std::transform

namespace Ra {
namespace Core {
namespace Asset {

inline void GeometryData::setName( const std::string& name ) {
    m_name = name;
}

inline GeometryData::GeometryType GeometryData::getType() const {
    return m_type;
}

inline void GeometryData::setType( const GeometryType& type ) {
    m_type = type;
}

inline Transform GeometryData::getFrame() const {
    return m_frame;
}

inline void GeometryData::setFrame( const Transform& frame ) {
    m_frame = frame;
}

inline std::size_t GeometryData::getVerticesSize() const {
    return m_vertex.size();
}

inline const Vector3Array& GeometryData::getVertices() const {
    return m_vertex;
}

inline Vector3Array& GeometryData::getVertices() {
    return m_vertex;
}

namespace internal {

template <typename InContainer, typename OutContainer>
inline void copyData( const InContainer& input, OutContainer& output ) {
    using OutValueType  = typename OutContainer::value_type;
    using OutScalarType = typename OutValueType::Scalar;
    std::transform( std::begin( input ),
                    std::end( input ),
                    std::back_inserter( output ),
                    []( const typename InContainer::value_type& v ) -> OutValueType {
                        return v.template cast<OutScalarType>();
                    } );
}

} // namespace internal

template <typename Container>
inline void GeometryData::setVertices( const Container& vertexList ) {
    internal::copyData( vertexList, m_vertex );
}

inline Vector2uArray& GeometryData::getEdges() {
    return m_edge;
}

inline const Vector2uArray& GeometryData::getEdges() const {
    return m_edge;
}

template <typename Container>
inline void GeometryData::setEdges( const Container& edgeList ) {
    internal::copyData( edgeList, m_edge );
}

inline const VectorNuArray& GeometryData::getFaces() const {
    return m_faces;
}

inline VectorNuArray& GeometryData::getFaces() {
    return m_faces;
}

template <typename Container>
inline void GeometryData::setFaces( const Container& faceList ) {
    internal::copyData( faceList, m_faces );
}

inline VectorNuArray& GeometryData::getPolyhedra() {
    return m_polyhedron;
}

inline const VectorNuArray& GeometryData::getPolyhedra() const {
    return m_polyhedron;
}

template <typename Container>
inline void GeometryData::setPolyhedra( const Container& polyList ) {
    internal::copyData( polyList, m_polyhedron );
}

inline Vector3Array& GeometryData::getNormals() {
    return m_normal;
}

inline const Vector3Array& GeometryData::getNormals() const {
    return m_normal;
}

template <typename Container>
inline void GeometryData::setNormals( const Container& normalList ) {
    internal::copyData( normalList, m_normal );
}

inline Vector3Array& GeometryData::getTangents() {
    return m_tangent;
}

inline const Vector3Array& GeometryData::getTangents() const {
    return m_tangent;
}

template <typename Container>
inline void GeometryData::setTangents( const Container& tangentList ) {
    internal::copyData( tangentList, m_tangent );
}

inline Vector3Array& GeometryData::getBiTangents() {
    return m_bitangent;
}

inline const Vector3Array& GeometryData::getBiTangents() const {
    return m_bitangent;
}

template <typename Container>
inline void GeometryData::setBitangents( const Container& bitangentList ) {
    internal::copyData( bitangentList, m_bitangent );
}

inline Vector3Array& GeometryData::getTexCoords() {
    return m_texCoord;
}

inline const Vector3Array& GeometryData::getTexCoords() const {
    return m_texCoord;
}

template <typename Container>
inline void GeometryData::setTextureCoordinates( const Container& texCoordList ) {
    internal::copyData( texCoordList, m_texCoord );
}

inline const MaterialData& GeometryData::getMaterial() const {
    return *( m_material.get() );
}

inline void GeometryData::setMaterial( MaterialData* material ) {
    m_material.reset( material );
}

inline bool GeometryData::isPointCloud() const {
    return ( m_type == POINT_CLOUD );
}

inline bool GeometryData::isLineMesh() const {
    return ( m_type == LINE_MESH );
}

inline bool GeometryData::isTriMesh() const {
    return ( m_type == TRI_MESH );
}

inline bool GeometryData::isQuadMesh() const {
    return ( m_type == QUAD_MESH );
}

inline bool GeometryData::isPolyMesh() const {
    return ( m_type == POLY_MESH );
}

inline bool GeometryData::isTetraMesh() const {
    return ( m_type == TETRA_MESH );
}

inline bool GeometryData::isHexMesh() const {
    return ( m_type == HEX_MESH );
}

inline bool GeometryData::hasVertices() const {
    return !m_vertex.empty();
}

inline bool GeometryData::hasEdges() const {
    return !m_edge.empty();
}

inline bool GeometryData::hasFaces() const {
    return !m_faces.empty();
}

inline bool GeometryData::hasPolyhedra() const {
    return !m_polyhedron.empty();
}

inline bool GeometryData::hasNormals() const {
    return !m_normal.empty();
}

inline bool GeometryData::hasTangents() const {
    return !m_tangent.empty();
}

inline bool GeometryData::hasBiTangents() const {
    return !m_bitangent.empty();
}

inline bool GeometryData::hasTextureCoordinates() const {
    return !m_texCoord.empty();
}

inline bool GeometryData::hasMaterial() const {
    return m_material != nullptr;
}

const Utils::AttribManager& GeometryData::getAttribManager() const {
    return m_vertexAttribs;
}

Utils::AttribManager& GeometryData::getAttribManager() {
    return m_vertexAttribs;
}

} // namespace Asset
} // namespace Core
} // namespace Ra
