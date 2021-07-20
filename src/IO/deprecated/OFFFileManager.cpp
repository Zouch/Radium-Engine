#include <IO/deprecated/OFFFileManager.hpp>

namespace Ra {

using namespace Core;

namespace IO {

/// ===============================================================================
/// CONSTRUCTOR
/// ===============================================================================
OFFFileManager::OFFFileManager() : FileManager<Geometry::TriangleMesh>() {}

/// ===============================================================================
/// DESTRUCTOR
/// ===============================================================================
OFFFileManager::~OFFFileManager() {}

/// ===============================================================================
/// HEADER
/// ===============================================================================
std::string OFFFileManager::header() const {
    return "OFF";
}

/// ===============================================================================
/// INTERFACE
/// ===============================================================================
std::string OFFFileManager::fileExtension() const {
    return "off";
}

bool OFFFileManager::importData( std::istream& file, Geometry::TriangleMesh& data ) {
    std::string h;
    file >> h;
    if ( h != header() )
    {
        addLogErrorEntry( "HEADER IS NOT CORRECT." );
        return false;
    }
    uint v_size;
    uint f_size;
    uint e_size;
    file >> v_size >> f_size >> e_size;
    data.clear();
    Geometry::TriangleMesh::PointAttribHandle::Container vertices;
    Geometry::TriangleMesh::IndexContainerType indices;
    vertices.resize( v_size );
    indices.resize( f_size );

    // Vertices
    for ( uint i = 0; i < v_size; ++i )
    {
        Vector3 v;
        file >> v[0] >> v[1] >> v[2];
        vertices[i] = v;
    }

    // Edge
    for ( uint i = 0; i < e_size; ++i )
    { break; }

    // Triangle
    for ( uint i = 0; i < f_size; ++i )
    {
        uint side;
        Vector3ui f;
        file >> side;
        if ( side == 3 )
        {
            file >> f[0] >> f[1] >> f[2];
            indices.push_back( f );
        }
    }

    data.setVertices( std::move( vertices ) );
    data.setIndices( std::move( indices ) );
    return true;
}

bool OFFFileManager::exportData( std::ostream& file, const Geometry::TriangleMesh& data ) {
    std::string content = "";
    const uint v_size   = data.vertices().size();
    const uint f_size   = data.getIndices().size();
    const uint e_size   = 0;

    if ( v_size == 0 )
    {
        addLogErrorEntry( "NO VERTICES PRESENT." );
        return false;
    }

    // Header
    content += header() + "\n" + std::to_string( v_size ) + " " + std::to_string( f_size ) + " " +
               std::to_string( e_size ) + "\n";

    // Vertices
    for ( const auto& v : data.vertices() )
    {
        content += std::to_string( v[0] ) + " " + std::to_string( v[1] ) + " " +
                   std::to_string( v[2] ) + "\n";
    }

    // Triangle
    for ( const auto& f : data.getIndices() )
    {
        content += "3 " + std::to_string( f[0] ) + " " + std::to_string( f[1] ) + " " +
                   std::to_string( f[2] ) + "\n";
    }

    file << content;

    return true;
}

} // namespace IO
} // namespace Ra
