#include <IO/CameraLoader/CameraLoader.hpp>

#include <Core/Asset/FileData.hpp>
#include <Core/Utils/StringUtils.hpp>

#include <fstream>
#include <iostream>
#include <string>

const std::string camExt { "cam" };

namespace Ra {
namespace IO {

using namespace Core::Utils; // log
using namespace Core::Asset;

std::vector<std::string> CameraFileLoader::getFileExtensions() const {
    return std::vector<std::string>( { "*." + camExt } );
}

bool CameraFileLoader::handleFileExtension( const std::string& extension ) const {
    return extension == camExt;
}

FileData* CameraFileLoader::loadFile( const std::string& filename ) {
    // Create the FileData
    auto fileData = new FileData( filename );
    if ( !fileData->isInitialized() )
    {
        delete fileData;
        LOG( logERROR ) << "[CameraLoader] Filedata cannot be initialized.";
        return nullptr;
    }
    if ( fileData->isVerbose() ) { LOG( logERROR ) << "[CameraLoader] File Loading begin..."; }

    /// Open the file
    std::ifstream ss( filename );
    if ( !ss.is_open() )
    {
        delete fileData;
        LOG( logERROR ) << "[CameraLoader] Camera file " << filename << " cannot be read.";
        return nullptr;
    }

    /// Read data
    std::clock_t startTime = std::clock();
    std::string str;
    int type;
    Scalar M[16]; // 4x4 view matrix;
    Scalar fov, znear, zfar, zoom, aspect;
    ss >> str;
    bool result = !ss.fail();
    ss >> type;
    result &= !ss.fail();
    for ( uint i = 0; i < 16; ++i )
    {
        ss >> M[i];
        result &= !ss.fail();
    }
    ss >> fov >> znear >> zfar >> zoom >> aspect;
    result &= !ss.fail();
    if ( !result )
    {
        delete fileData;
        LOG( logERROR ) << "[CameraLoader] Could not load camera file data: " << filename;
        return nullptr;
    }
    std::clock_t endTime = std::clock();

    /// create the CameraData
    Camera* camera = new Camera();
    //    camera->setName( Core::Utils::getBaseName( filename, false ) );
    camera->setType( Camera::ProjType( type ) );
    Core::Matrix4 frame;
    frame << M[0], M[1], M[2], M[3], M[4], M[5], M[6], M[7], M[8], M[9], M[10], M[11], M[12], M[13],
        M[14], M[15];
    camera->setFrame( Core::Transform { frame } );
    camera->setFOV( fov );
    camera->setZNear( znear );
    camera->setZFar( zfar );
    camera->setZoomFactor( zoom );
    camera->setViewport( aspect, 1 );
    fileData->m_cameraData.emplace_back( camera );
    if ( fileData->isVerbose() ) { fileData->displayInfo(); }

    fileData->m_loadingTime = ( endTime - startTime ) / Scalar( CLOCKS_PER_SEC );
    fileData->m_processed   = true;
    return fileData;
}

std::string CameraFileLoader::name() const {
    return "CameraLoader";
}

} // namespace IO
} // namespace Ra
