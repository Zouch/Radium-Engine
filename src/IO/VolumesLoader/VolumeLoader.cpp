#include <VolumesLoader/VolumeLoader.hpp>

#include <VolumesLoader/pvmutils.hpp>

#include <Core/Asset/FileData.hpp>
#include <Core/Geometry/Volume.hpp>
#include <Core/Utils/Log.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>

namespace Ra {
namespace IO {

using namespace Ra::Core;
using namespace Ra::Core::Utils;

const std::string volFileExtension {"vol"};
const std::string pvmFileExtension {"pvm"};
VolumeLoader::VolumeLoader() {}

VolumeLoader::~VolumeLoader() {}

std::vector<std::string> VolumeLoader::getFileExtensions() const {
    return {{"*." + volFileExtension}, {"*." + pvmFileExtension}};
}

bool VolumeLoader::handleFileExtension( const std::string& extension ) const {
    return ( extension.compare( volFileExtension ) == 0 ) ||
           ( extension.compare( pvmFileExtension ) == 0 );
}

Ra::Core::Utils::Color readColor( std::ifstream& input, std::string& name ) {
    char beg, end;
    Scalar r, g, b;
    input >> name >> beg >> r >> g >> b >> end;
    if ( beg == '[' && end == ']' ) { return Ra::Core::Utils::Color( r, g, b ); }
    return Ra::Core::Utils::Color( 0, 0, 0 );
}

bool checkExpected( const std::string& expected, const std::string& found ) {
    if ( expected != found )
    {
        LOG( logWARNING ) << "\tVolumeLoader : unexpected volume attribute " << found << " : "
                          << expected << " was expected";
        return false;
    }
    return true;
}

Ra::Core::Asset::FileData* VolumeLoader::loadVolFile( const std::string& filename ) {
    LOG( logINFO ) << "VolumeLoader : loading vol (pbrt based) file " << filename;
    std::ifstream input( filename );
    if ( input.is_open() )
    {
        auto fileData = new Ra::Core::Asset::FileData( filename );
        std::string attribname;
        auto sigma_a = readColor( input, attribname );
        if ( !checkExpected( "sigma_a", attribname ) ) { return nullptr; }
        auto sigma_s = readColor( input, attribname );
        if ( !checkExpected( "sigma_s", attribname ) ) { return nullptr; }

        char beg, end;
        int sx, sy, sz;

        input >> attribname >> beg >> sx >> sy >> sz >> end;
        if ( !checkExpected( "size", attribname ) ) { return nullptr; }

        input >> attribname >> beg;
        if ( !checkExpected( "density", attribname ) ) { return nullptr; }
        if ( beg != '[' )
        {
            LOG( logWARNING ) << "\tVolumeLoader : unexpected start of density gri delimiter "
                              << beg;
            return nullptr;
        }
        LOG( logINFO ) << "\tVolumeLoader : reading a volume of size " << sx << "x" << sy << "x"
                       << sz;

        Ra::Core::Vector3 voxelSize {1_ra, 1_ra, 1_ra};
        auto density = new Geometry::VolumeGrid();
        density->setSize( Vector3i( sx, sy, sz ) );
        density->setBinSize( voxelSize );
        std::generate( density->data().begin(), density->data().end(), [&input]() {
            Scalar v;
            input >> v;
            return v;
        } );

        input >> end;
        if ( end != ']' )
        {
            LOG( logWARNING ) << "\tVolumeLoader : unexpected end of density grid delimiter "
                              << end;
            return nullptr;
        }
        LOG( logINFO ) << "\tVolumeLoader : done reading";

        auto volume = new Asset::VolumeData( filename.substr( filename.find_last_of( '/' ) + 1 ) );
        volume->volume  = density;
        volume->sigma_a = sigma_a;
        volume->sigma_s = sigma_s;
        Scalar maxDim   = std::max( std::max( sx, sy ), sz );
        Ra::Core::Vector3 p0( 0, 0, 0 );
        Ra::Core::Vector3 p1( sx, sy, sz );
        volume->boundingBox    = Aabb( p0, p1 );
        volume->densityToModel = Transform::Identity(); // Eigen::Scaling( 1_ra / maxDim );
        volume->modelToWorld   = Eigen::Scaling( 1_ra / maxDim ); // Transform::Identity();
        fileData->m_volumeData.push_back( std::unique_ptr<Ra::Core::Asset::VolumeData>( volume ) );
        return fileData;
    }
    LOG( logWARNING ) << "VolumeLoader : unable to open file " << filename;
    return nullptr;
}

Ra::Core::Asset::FileData* VolumeLoader::loadPvmFile( const std::string& filename ) {
    using namespace PVMVolume;

    unsigned int width, height, depth, bytePerVoxel;
    float scalex, scaley, scalez;
    unsigned char *description, *courtesy, *parameter, *comment;

    auto volumeData = readPVMvolume( filename.c_str(),
                                     &width,
                                     &height,
                                     &depth,
                                     &bytePerVoxel,
                                     &scalex,
                                     &scaley,
                                     &scalez,
                                     &description,
                                     &courtesy,
                                     &parameter,
                                     &comment );
    if ( volumeData )
    {
        LOG( logINFO ) << "VolumeLoader : pvm (The Volume Library) file " << filename
                       << " \n\twidth = " << width << " \n\theight = " << height
                       << " \n\tdepth = " << depth << " \n\tbyte per voxel = " << bytePerVoxel
                       << " \n\tscalex = " << scalex << " \n\tscaley = " << scaley
                       << " \n\tscalez = " << scalez << " \n\tdescription = " << description
                       << " \n\tcourtesy = " << courtesy << " \n\tparameter = " << parameter
                       << " \n\tcomment = " << comment;

        auto fillRadiumVolume = []( auto container, auto densityData ) {
            std::generate( container->data().begin(), container->data().end(), [&densityData]() {
                auto d = *densityData++;
                return Scalar( d ) / std::numeric_limits<decltype( d )>::max();
            } );
        };
        auto density = new Geometry::VolumeGrid();
        density->setSize( Vector3i( width, height, depth ) );
        density->setBinSize(
            Ra::Core::Vector3 {Scalar( scalex ), Scalar( scaley ), Scalar( scalez )} );

        switch ( bytePerVoxel )
        {
        case 1: {
            fillRadiumVolume( density, volumeData );
            break;
        }
        case 2: {
            fillRadiumVolume( density, (uint16_t*)( volumeData ) );
            break;
        }
        case 4: {
            fillRadiumVolume( density, (uint*)( volumeData ) );
            break;
        }
        default:
            LOG( logERROR ) << "VolumeLoader : unsupported number of componenets : "
                            << bytePerVoxel;
        }
        free( volumeData );

        LOG( logINFO ) << "\tVolumeLoader : done reading";

        auto volume = new Asset::VolumeData( filename.substr( filename.find_last_of( '/' ) + 1 ) );
        volume->volume = density;
        Scalar maxDim  = std::max( std::max( width, height ), depth );
        Ra::Core::Vector3 p0( 0, 0, 0 );
        Ra::Core::Vector3 p1( width * scalex, height * scaley, depth * scalez );
        volume->boundingBox    = Aabb( p0, p1 );
        volume->densityToModel = Eigen::Scaling( scalex, scaley, scalez );
        volume->modelToWorld   = Eigen::Scaling( 1_ra / maxDim ) * Translation( p1 * -0.5 );

        auto fileData = new Ra::Core::Asset::FileData( filename );
        fileData->m_volumeData.push_back( std::unique_ptr<Ra::Core::Asset::VolumeData>( volume ) );
        return fileData;
    }
    LOG( logWARNING ) << "VolumeLoader : unable to open file " << filename;
    return nullptr;
}

Ra::Core::Asset::FileData* VolumeLoader::loadFile( const std::string& filename ) {
    std::string extension = filename.substr( filename.find_last_of( '.' ) + 1 );
    if ( extension.compare( volFileExtension ) == 0 ) { return loadVolFile( filename ); }
    else if ( extension.compare( pvmFileExtension ) == 0 )
    { return loadPvmFile( filename ); }
    LOG( logWARNING ) << "VolumeLoader : unsupported file format : " << filename;
    return nullptr;
}

std::string VolumeLoader::name() const {
    return "VolumeLoader (pbrt experimental, pvm)";
}

} // namespace IO
} // namespace Ra
