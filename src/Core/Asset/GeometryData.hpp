#pragma once

#include <memory>
#include <string>
#include <vector>

#include <Core/Containers/VectorArray.hpp>
#include <Core/RaCore.hpp>
#include <Core/Types.hpp>
#include <Core/Utils/Attribs.hpp>
#include <Core/Utils/Index.hpp>

#include <Core/Asset/AssetData.hpp>
#include <Core/Asset/MaterialData.hpp>

namespace Ra {
namespace Core {
namespace Asset {

class MaterialData;

/**
 * The GeometryData class stores all the geometry related data of a loaded object.
 */
class RA_CORE_API GeometryData : public AssetData
{

  public:
    using ColorArray = Vector4Array;

  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    /**
     * The type of geometry.
     */
    enum GeometryType {
        UNKNOWN     = 1 << 0,
        POINT_CLOUD = 1 << 1,
        LINE_MESH   = 1 << 2,
        TRI_MESH    = 1 << 3,
        QUAD_MESH   = 1 << 4,
        POLY_MESH   = 1 << 5,
        TETRA_MESH  = 1 << 6,
        HEX_MESH    = 1 << 7
    };

    GeometryData( const std::string& name = "", const GeometryType& type = UNKNOWN );

    GeometryData( const GeometryData& data ) = delete;

    ~GeometryData();

    /// \name Data access
    /// \{

    /// Return the name of the object.
    inline void setName( const std::string& name );

    /// Return the type of geometry.
    inline GeometryType getType() const;

    /// Set the type of geometry.
    inline void setType( const GeometryType& type );

    /// Return the Transform of the object.
    inline Transform getFrame() const;

    /// Set the Transform of the object.
    inline void setFrame( const Transform& frame );

    /// Return the number of vertices.
    inline std::size_t getVerticesSize() const;

    /// Return the list of vertices.
    inline Vector3Array& getVertices();

    /// Return the list of vertices.
    inline const Vector3Array& getVertices() const;

    /// Set the mesh vertices.
    /// \note In-place setting with getVertices() is preferred.
    template <typename Container>
    inline void setVertices( const Container& vertexList );

    /// Return the list of lines.
    /// \note For line meshes only.
    inline Vector2uArray& getEdges();

    /// Return the list of lines.
    /// \note For line meshes only.
    inline const Vector2uArray& getEdges() const;

    /// Set the list of lines.
    /// \note For line meshes only.
    /// \note In-place setting with getEdges() is preferred.
    template <typename Container>
    inline void setEdges( const Container& edgeList );

    /// Return the list of faces.
    /// \note For triangle/quadrangle/polygonal meshes only.
    inline VectorNuArray& getFaces();

    /// Return the list of faces.
    /// \note For triangle/quadrangle/polygonal meshes only.
    inline const VectorNuArray& getFaces() const;

    /// Set the list of faces.
    /// \note For triangle/quadrangle/polygonal meshes only.
    /// \note In-place setting with getFaces() is preferred.
    template <typename Container>
    inline void setFaces( const Container& faceList );

    /// Return the list of polyhedra.
    /// \note For tetrahedron/hexahedron meshes only.
    inline VectorNuArray& getPolyhedra();

    /// Return the list of polyhedra.
    /// \note For tetrahedron/hexahedron meshes only.
    inline const VectorNuArray& getPolyhedra() const;

    /// Set the list of polyhedra.
    /// \note For tetrahedron/hexahedron meshes only.
    /// \note In-place setting with getPolyhedra() is preferred.
    template <typename Container>
    inline void setPolyhedra( const Container& polyList );

    /// Return the list of vertex normals
    inline Vector3Array& getNormals();

    /// Return the list of vertex normals.
    inline const Vector3Array& getNormals() const;

    /// Set the vertex normals.
    /// \note In-place setting with getNormals() is preferred.
    template <typename Container>
    inline void setNormals( const Container& normalList );

    /// Return the list of vertex tangent vectors.
    inline Vector3Array& getTangents();

    /// Return the list of vertex tangent vectors.
    inline const Vector3Array& getTangents() const;

    /// Set the vertex tangent vectors.
    /// \note In-place setting with getTangents() is preferred.
    template <typename Container>
    inline void setTangents( const Container& tangentList );

    /// Return the list of vertex bitangent vectors.
    inline Vector3Array& getBiTangents();

    /// Return the list of vertex bitangent vectors.
    inline const Vector3Array& getBiTangents() const;

    /// Set the vertex bitangent vectors.
    /// \note In-place setting with getBiTangents() is preferred.
    template <typename Container>
    inline void setBitangents( const Container& bitangentList );

    /// Return the list of vertex texture coordinates.
    inline Vector3Array& getTexCoords();

    /// Return the list of vertex texture coordinates.
    inline const Vector3Array& getTexCoords() const;

    /// Set the vertex texture coordinates.
    /// \note In-place setting with getTexCoords() is preferred.
    template <typename Container>
    inline void setTextureCoordinates( const Container& texCoordList );

    /// Return the MaterialData associated to the objet.
    inline const MaterialData& getMaterial() const;

    /// Set the MaterialData for the object.
    inline void setMaterial( MaterialData* material );
    /// \}

    /// \name Status queries
    /// \{

    /// Return true if the object is a Point Cloud.
    inline bool isPointCloud() const;

    /// Return true if the object is a Line Mesh.
    inline bool isLineMesh() const;

    /// Return true if the object is a Triangle Mesh.
    inline bool isTriMesh() const;

    /// Return true if the object is a Quadrangle Mesh.
    inline bool isQuadMesh() const;

    /// Return true if the object is a Polygon Mesh.
    /// \note Return false for Triangle and Quadrangle meshes.
    inline bool isPolyMesh() const;

    /// Return true if the object is a Tetrahedron Mesh.
    inline bool isTetraMesh() const;

    /// Return true if the object is a Hexahedron Mesh.
    inline bool isHexMesh() const;

    /// Return true if the object has vertices.
    inline bool hasVertices() const;

    /// Return true if the object has lines.
    inline bool hasEdges() const;

    /// Return true if the object has faces.
    inline bool hasFaces() const;

    /// Return true if the object has polyhedra.
    inline bool hasPolyhedra() const;

    /// Return true if the object has vertex normals.
    inline bool hasNormals() const;

    /// Return true if the object has vertex tangent vectors.
    inline bool hasTangents() const;

    /// Return true if the object has vertex bitangent vectors.
    inline bool hasBiTangents() const;

    /// Return true if the object has vertex texture coordinates.
    inline bool hasTextureCoordinates() const;

    /// Return true if the object has MaterialData.
    inline bool hasMaterial() const;
    /// \}

    /// Print stast info to the Debug output.
    void displayInfo() const;

    /// Access to the attrib manager
    inline Utils::AttribManager& getAttribManager();

    /// Access to the (const) attrib manager
    inline const Utils::AttribManager& getAttribManager() const;

  protected:
    /// The transformation of the object.
    Transform m_frame;

    /// The type of geometry for the object.
    GeometryType m_type;

    /// The list of vertices.
    Vector3Array m_vertex;

    /// The list of lines.
    Vector2uArray m_edge;

    /// The list of faces
    VectorNuArray m_faces;

    /// The list of polyhedra
    VectorNuArray m_polyhedron;

    /// The list of vertex normals.
    [[deprecated]] Vector3Array m_normal;

    /// The list of vertex tangent vectors.
    [[deprecated]] Vector3Array m_tangent;

    /// The list of vertex bitangent vectors.
    [[deprecated]] Vector3Array m_bitangent;

    /// The list of vertex texture coordinates.
    [[deprecated]] Vector3Array m_texCoord;

    /// The MaterialData for the object.
    std::shared_ptr<MaterialData> m_material;

    /// Named attributes
    /// \todo Move all built-in attributes to m_vertexAttribs
    Utils::AttribManager m_vertexAttribs;
};

} // namespace Asset
} // namespace Core
} // namespace Ra

#include <Core/Asset/GeometryData.inl>
