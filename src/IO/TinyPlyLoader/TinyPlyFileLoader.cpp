#include <IO/TinyPlyLoader/TinyPlyFileLoader.hpp>

#include <Core/Asset/FileData.hpp>
#include <Core/Containers/VectorArray.hpp>
#include <Core/Utils/Attribs.hpp>

#include <tinyply.h>

#include <fstream>
#include <iostream>
#include <string>

const std::string plyExt( "ply" );

namespace Ra {
namespace IO {

using namespace Core::Utils; // log
using namespace Core::Asset; // Filedata

TinyPlyFileLoader::TinyPlyFileLoader() {}

TinyPlyFileLoader::~TinyPlyFileLoader() {}

std::vector<std::string> TinyPlyFileLoader::getFileExtensions() const {
    return std::vector<std::string>( {"*." + plyExt} );
}

bool TinyPlyFileLoader::handleFileExtension( const std::string& extension ) const {
    return extension.compare( plyExt ) == 0;
}
// from https://github.com/ddiakopoulos/tinyply/blob/master/source/example-utils.hpp

struct memory_buffer : public std::streambuf {
    char* p_start {nullptr};
    char* p_end {nullptr};
    size_t size;

    memory_buffer( char const* first_elem, size_t size_ ) :
        p_start( const_cast<char*>( first_elem ) ), p_end( p_start + size_ ), size( size_ ) {
        setg( p_start, p_start, p_end );
    }

    pos_type seekoff( off_type off, std::ios_base::seekdir dir, std::ios_base::openmode ) override {
        if ( dir == std::ios_base::cur )
            gbump( static_cast<int>( off ) );
        else
            setg( p_start, ( dir == std::ios_base::beg ? p_start : p_end ) + off, p_end );
        return gptr() - p_start;
    }

    pos_type seekpos( pos_type pos, std::ios_base::openmode which ) override {
        return seekoff( pos, std::ios_base::beg, which );
    }
};

inline std::vector<uint8_t> read_file_binary( const std::string& pathToFile ) {
    std::ifstream file( pathToFile, std::ios::binary );
    std::vector<uint8_t> fileBufferBytes;

    if ( file.is_open() )
    {
        file.seekg( 0, std::ios::end );
        size_t sizeBytes = file.tellg();
        file.seekg( 0, std::ios::beg );
        fileBufferBytes.resize( sizeBytes );
        if ( file.read( (char*)fileBufferBytes.data(), sizeBytes ) ) return fileBufferBytes;
    }
    else
        throw std::runtime_error( "could not open binary ifstream to path " + pathToFile );
    return fileBufferBytes;
}

struct memory_stream : virtual memory_buffer, public std::istream {
    memory_stream( char const* first_elem, size_t size_ ) :
        memory_buffer( first_elem, size_ ), std::istream( static_cast<std::streambuf*>( this ) ) {}
};

template <typename DataType>
void copyArrayToContainer( const uint8_t* buffer,
                           Ra::Core::Vector1Array& container,
                           size_t count ) {
    auto data = reinterpret_cast<const DataType*>( buffer );
    container.reserve( count );
    for ( size_t i = 0; i < count; ++i )
    {
        container.emplace_back( data[i] );
    }
}

template <typename DataType>
void copyArrayToContainer( const uint8_t* buffer,
                           Ra::Core::Vector3Array& container,
                           size_t count ) {
    auto data = reinterpret_cast<const DataType*>( buffer );
    container.reserve( count );
    for ( size_t i = 0; i < count; ++i )
    {
        container.emplace_back( data[i * 3 + 0], data[i * 3 + 1], data[i * 3 + 2] );
    }
}

template <typename ContainerType>
void copyBufferToContainer( const std::shared_ptr<tinyply::PlyData>& buffer,
                            ContainerType& container ) {
    if ( buffer && buffer->count != 0 )
    {
        switch ( buffer->t )
        {
        case tinyply::Type::FLOAT32: {
            copyArrayToContainer<float>( buffer->buffer.get(), container, buffer->count );
        }
        break;
        case tinyply::Type::FLOAT64: {
            copyArrayToContainer<double>( buffer->buffer.get(), container, buffer->count );
        }
        break;
        default:
            LOG( logWARNING ) << "[TinyPLY] copyBufferToContainer - unsupported buffer type ..."
                              << tinyply::PropertyTable[buffer->t].str;
        }
    }
}

FileData* TinyPlyFileLoader::loadFile( const std::string& filename ) {

    std::unique_ptr<std::istream> file_stream;
    std::vector<uint8_t> byte_buffer;

    // from https://github.com/ddiakopoulos/tinyply/blob/master/source/example.cpp
    // For most files < 1gb, pre-loading the entire file upfront and wrapping it into a
    // stream is a net win for parsing speed, about 40% faster.
    const bool preload_into_memory = true;
    if ( preload_into_memory )
    {
        byte_buffer = read_file_binary( filename );
        file_stream.reset( new memory_stream( (char*)byte_buffer.data(), byte_buffer.size() ) );
    }
    else
    { file_stream.reset( new std::ifstream( filename, std::ios::binary ) ); }

    if ( !file_stream || file_stream->fail() )
    {
        LOG( logINFO ) << "[TinyPLY] Could not open file [" << filename << "] Aborting"
                       << std::endl;
        return nullptr;
    }

    // Parse the ASCII header fields
    tinyply::PlyFile file;
    file.parse_header( *file_stream );

    auto elements = file.get_elements();
    if ( std::any_of( elements.begin(), elements.end(), []( const auto& e ) -> bool {
             return e.name == "face" && e.size != 0;
         } ) )
    {
        // Mesh found. Let the other loaders handle it
        LOG( logINFO ) << "[TinyPLY] Faces found. Aborting" << std::endl;
        return nullptr;
    }

    // we are now sure to have a point-cloud
    FileData* fileData = new FileData( filename );
    fileData->setVerbose( true );

    if ( !fileData->isInitialized() )
    {
        delete fileData;
        LOG( logINFO ) << "[TinyPLY] Filedata cannot be initialized...";
        return nullptr;
    }

    if ( fileData->isVerbose() )
    {
        LOG( logINFO ) << "[TinyPLY] File Loading begin...";
        LOG( logINFO ) << "....................................................................";
        for ( auto c : file.get_comments() )
            LOG( logINFO ) << "Comment: " << c;
        for ( auto e : file.get_elements() )
        {
            LOG( logINFO ) << "element - " << e.name << " (" << e.size << ")";
            for ( auto p : e.properties )
                LOG( logINFO ) << "\tproperty - " << p.name << " ("
                               << tinyply::PropertyTable[p.propertyType].str << ")";
        }
        LOG( logINFO ) << "....................................................................";
    }

    auto initBuffer = [&file]( const std::string& elementKey,
                               const std::vector<std::string> propertyKeys ) {
        std::shared_ptr<tinyply::PlyData> ret;
        try
        { ret = file.request_properties_from_element( elementKey, propertyKeys ); }
        catch ( const std::exception& e )
        {
            ret = nullptr;
            LOG( logINFO ) << "[TinyPLY] " << e.what();
        }
        return ret;
    };

    auto startTime {std::clock()};

    // a unique name is required by the component messaging system
    static int nameId {0};
    auto geometry = std::make_unique<GeometryData>( "PC_" + std::to_string( ++nameId ),
                                                    GeometryData::POINT_CLOUD );
    geometry->setFrame( Core::Transform::Identity() );

    /// request for vertex position
    auto vertBuffer {initBuffer( "vertex", {"x", "y", "z"} )};
    // if there is no vertex prop, or their count is 0, then quit.
    if ( !vertBuffer || vertBuffer->count == 0 )
    {
        delete fileData;
        LOG( logINFO ) << "[TinyPLY] No vertex found";
        return nullptr;
    }
    /// request for standard vertex attributes
    /// \todo merge with non standard attributes when all will be stored as Attribs in GeometryData
    auto normalBuffer {initBuffer( "vertex", {"nx", "ny", "nz"} )};
    auto alphaBuffer {initBuffer( "vertex", {"alpha"} )};
    auto colorBuffer {initBuffer( "vertex", {"red", "green", "blue"} )};

    //// request for non standard vertex attributes
    std::vector<std::pair<std::string, std::shared_ptr<tinyply::PlyData>>> allAttributes;
    const std::set<std::string> usedAttributes {
        "x", "y", "z", "nx", "ny", "nz", "alpha", "red", "green", "blue"};
    for ( const auto& e : file.get_elements() )
    {
        if ( e.name == "vertex" )
        {
            for ( const auto& p : e.properties )
            {
                bool exists = usedAttributes.find( p.name ) != usedAttributes.end();
                if ( !exists )
                { allAttributes.emplace_back( p.name, initBuffer( "vertex", {p.name} ) ); }
            }
            break;
        }
    }

    // read requested buffers (and only those) from file content
    file.read( *file_stream );

    copyBufferToContainer( vertBuffer, geometry->getVertices() );
    copyBufferToContainer( normalBuffer, geometry->getNormals() );

    auto& attribManager = geometry->getAttribManager();

    size_t colorCount = colorBuffer ? colorBuffer->count : 0;
    if ( colorCount != 0 )
    {
        auto handle     = attribManager.addAttrib<Core::Vector4>( "in_color" );
        auto& attrib    = attribManager.getAttrib( handle );
        auto& container = attrib.getDataWithLock();

        if ( alphaBuffer && alphaBuffer->count == colorCount )
        {
            uint8_t* al     = alphaBuffer->buffer.get();
            uint8_t* colors = colorBuffer->buffer.get();
            for ( size_t i = 0; i < colorCount; ++i, al++, colors += 3 )
            {
                container.emplace_back( Scalar( colors[0] ) / 255_ra,
                                        Scalar( colors[1] ) / 255_ra,
                                        Scalar( colors[2] ) / 255_ra,
                                        Scalar( *al ) / 255_ra );
            }
        }
        else
        {
            uint8_t* colors = colorBuffer->buffer.get();
            for ( size_t i = 0; i < colorCount; ++i, colors += 3 )
            {
                container.emplace_back( Scalar( colors[0] ) / 255_ra,
                                        Scalar( colors[1] ) / 255_ra,
                                        Scalar( colors[2] ) / 255_ra,
                                        1_ra );
            }
        }
        attrib.unlock();
    }

    //// Save other attributes
    for ( const auto& a : allAttributes )
    {
        // For now manage scalar properties only
        /// @todo, when all vertex attribs are managed through the attrib manager, add here
        if ( a.second->isList )
        {
            LOG( logWARNING ) << "[TinyPLY] unmanaged vector attribute " << a.first;
            continue;
        }
        /// Transform attrib name to valid GLSL identifier
        auto attribName {a.first};
        std::replace( attribName.begin(), attribName.end(), '-', '_' );
        LOG( logINFO ) << "[TinyPLY] Adding custom attrib with name " << attribName << " (was "
                       << a.first << ")";
        auto handle     = attribManager.addAttrib<Scalar>( attribName );
        auto& attrib    = attribManager.getAttrib( handle );
        auto& container = attrib.getDataWithLock();
        copyBufferToContainer( a.second, container );
        attrib.unlock();
    }

    fileData->m_geometryData.clear();
    fileData->m_geometryData.reserve( 1 );
    fileData->m_geometryData.push_back( std::move( geometry ) );

    fileData->m_loadingTime = ( std::clock() - startTime ) / Scalar( CLOCKS_PER_SEC );

    if ( fileData->isVerbose() )
    {
        LOG( logINFO ) << "[TinyPLY] File Loading end.";
        fileData->displayInfo();
    }

    fileData->m_processed = true;

    return fileData;
}

std::string TinyPlyFileLoader::name() const {
    return "TinyPly";
}
} // namespace IO
} // namespace Ra
