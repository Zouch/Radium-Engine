#include <IO/AssimpLoader/AssimpCameraDataLoader.hpp>

#include <assimp/scene.h>

#include <Core/Asset/Camera.hpp>
#include <Core/Utils/Log.hpp>

#include <IO/AssimpLoader/AssimpWrapper.hpp>

namespace Ra {
namespace IO {

using namespace Core::Utils; // log
using namespace Core::Asset; // log

AssimpCameraDataLoader::AssimpCameraDataLoader( const bool VERBOSE_MODE ) :
    DataLoader<Camera>( VERBOSE_MODE ) {}

AssimpCameraDataLoader::~AssimpCameraDataLoader() = default;

void AssimpCameraDataLoader::loadData( const aiScene* scene,
                                       std::vector<std::unique_ptr<Camera>>& data ) {
    data.clear();

    if ( scene == nullptr )
    {
        LOG( logDEBUG ) << "AssimpCameraDataLoader : scene is nullptr.";
        return;
    }

    if ( !sceneHasCamera( scene ) )
    {
        LOG( logDEBUG ) << "AssimpCameraDataLoader : scene has no Cameras.";
        return;
    }

    if ( m_verbose )
    {
        LOG( logINFO ) << "File contains Camera.";
        LOG( logINFO ) << "Camera Loading begin...";
    }

    uint CameraSize = sceneCameraSize( scene );
    data.reserve( CameraSize );
    for ( uint CameraId = 0; CameraId < CameraSize; ++CameraId )
    {
        Camera* camera = new Camera();
        loadCameraData( scene, *( scene->mCameras[CameraId] ), *camera );
        ///\todo process camera name to feed component name
        data.push_back( std::unique_ptr<Camera>( camera ) );
    }

    if ( m_verbose ) { LOG( logINFO ) << "Camera Loading end.\n"; }
}

bool AssimpCameraDataLoader::sceneHasCamera( const aiScene* scene ) const {
    return ( scene->HasCameras() );
}

uint AssimpCameraDataLoader::sceneCameraSize( const aiScene* scene ) const {
    return scene->mNumCameras;
}

void AssimpCameraDataLoader::loadCameraData( const aiScene* scene,
                                             const aiCamera& camera,
                                             Camera& data ) {
    Core::Matrix4 rootMatrix;
    rootMatrix           = Core::Matrix4::Identity();
    Core::Matrix4 frame  = loadCameraFrame( scene, rootMatrix, camera );
    Core::Vector3 pos    = assimpToCore( camera.mPosition );
    Core::Vector3 lookAt = -assimpToCore( camera.mLookAt ).normalized();
    Core::Vector3 up     = assimpToCore( camera.mUp ).normalized();
    Core::Vector3 right  = lookAt.cross( up );
    Core::Matrix4 view;

    // make frame a normal frame change (consider it's already ortho ...)
    frame.block( 0, 0, 3, 1 ).normalize();
    frame.block( 0, 1, 3, 1 ).normalize();
    frame.block( 0, 2, 3, 1 ).normalize();

    view.block<3, 1>( 0, 0 ) = right;
    view.block<3, 1>( 0, 1 ) = up;
    view.block<3, 1>( 0, 2 ) = lookAt;
    view.block<3, 1>( 0, 3 ) = pos;
    data.setFrame( Core::Transform { view * frame } );

    data.setType( Camera::ProjType::PERSPECTIVE ); // default value since not in aiCamera
    // assimp doc (the version we used, updated in assimp master) state mHorizontalFOV is half
    // angle, but it is not, at least for collada, see  https://github.com/assimp/assimp/issues/2256
    // https://github.com/assimp/assimp/pull/3912
    data.setFOV( camera.mHorizontalFOV );
    data.setZNear( camera.mClipPlaneNear );
    data.setZFar( camera.mClipPlaneFar );
    data.setZoomFactor( 1.0 ); // default value since not in aiCamera
    data.setViewport( camera.mAspect, 1_ra );
}

Core::Matrix4 AssimpCameraDataLoader::loadCameraFrame( const aiScene* scene,
                                                       const Core::Matrix4& /*parentFrame*/,
                                                       const aiCamera& cameraNode ) const {
    // old version
    //    Core::Matrix4 oldVersionFrame;
    //    {
    //        aiNode* node = scene->mRootNode->FindNode( cameraNode.mName );
    //        auto t0      = Core::Matrix4::NullaryExpr(
    //            [&scene]( int i, int j ) { return scene->mRootNode->mTransformation[i][j]; } );
    //        auto t1 = Core::Matrix4::NullaryExpr(
    //            [&node]( int i, int j ) { return node->mTransformation[i][j]; } );
    //
    //        oldVersionFrame = parentFrame * t0 * t1;
    //    }

    aiNode* node = scene->mRootNode->FindNode( cameraNode.mName );
    Core::Matrix4 frame;
    frame.setIdentity();

    while ( node != nullptr )
    {
        frame = Core::Matrix4::NullaryExpr(
                    [&node]( int i, int j ) { return node->mTransformation[i][j]; } ) *
                frame;
        node = node->mParent;
    }

    // to test old vs new
    // if ( frame != oldVersionFrame ) { LOG( logWARNING ) << "frame computation update failed!"; }

    return frame;
}

} // namespace IO
} // namespace Ra
