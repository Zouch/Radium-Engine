#include <IO/AssimpLoader/AssimpLightDataLoader.hpp>

#include <assimp/scene.h>

#include <Core/Utils/Log.hpp>

#include <IO/AssimpLoader/AssimpWrapper.hpp>

namespace Ra {
namespace IO {

using namespace Core::Utils; // log
using namespace Core::Asset;

AssimpLightDataLoader::AssimpLightDataLoader( const std::string& filepath,
                                              const bool VERBOSE_MODE ) :
    DataLoader<LightData>( VERBOSE_MODE ), m_filepath( filepath ) {}

AssimpLightDataLoader::~AssimpLightDataLoader() = default;

/// LOADING
void AssimpLightDataLoader::loadData( const aiScene* scene,
                                      std::vector<std::unique_ptr<LightData>>& data ) {
    data.clear();

    if ( scene == nullptr ) {
        LOG( logDEBUG ) << "AssimpLightDataLoader : scene is nullptr.";
        return;
    }

    if ( !sceneHasLight( scene ) ) {
        LOG( logDEBUG ) << "AssimpLightDataLoader : scene has no lights.";
        return;
    }

    if ( m_verbose ) {
        LOG( logINFO ) << "File contains light.";
        LOG( logINFO ) << "Light Loading begin...";
    }

    uint lightSize = sceneLightSize( scene );
    data.reserve( lightSize );
    for ( uint lightId = 0; lightId < lightSize; ++lightId ) {
        data.emplace_back( loadLightData( scene, *( scene->mLights[lightId] ) ) );
    }

    if ( m_verbose ) { LOG( logINFO ) << "Light Loading end.\n"; }
}

bool AssimpLightDataLoader::sceneHasLight( const aiScene* scene ) const {
    return ( scene->HasLights() );
}

uint AssimpLightDataLoader::sceneLightSize( const aiScene* scene ) const {
    return scene->mNumLights;
}

// LightData * AssimpLightDataLoader::loadLightData(const aiScene *scene, const aiLight
// &light)
std::unique_ptr<LightData> AssimpLightDataLoader::loadLightData( const aiScene* scene,
                                                                 const aiLight& light ) {
    //    auto builtLight = new LightData(fetchName( light ), fetchType( light) );
    auto builtLight = std::make_unique<LightData>( fetchName( light ), fetchType( light ) );
    Core::Matrix4 rootMatrix;
    rootMatrix          = Core::Matrix4::Identity();
    Core::Matrix4 frame = loadLightFrame( scene, rootMatrix, builtLight->getName() );
    setFrame( frame );
    auto color = assimpToCore( light.mColorDiffuse );

    switch ( builtLight->getType() ) {
    case LightData::DIRECTIONAL_LIGHT: {
        Core::Vector4 dir( light.mDirection[0], light.mDirection[1], light.mDirection[2], 0.0 );
        builtLight->setLight( color, -( frame.transpose().inverse() * dir ).head<3>() );
    } break;

    case LightData::POINT_LIGHT: {
        builtLight->setLight(
            color,
            ( frame *
              Core::Vector4 { light.mPosition[0], light.mPosition[1], light.mPosition[2], 1.0 } )
                .hnormalized(),
            LightData::LightAttenuation( light.mAttenuationConstant,
                                         light.mAttenuationLinear,
                                         light.mAttenuationQuadratic ) );
    } break;

    case LightData::SPOT_LIGHT: {
        Core::Vector4 dir( light.mDirection[0], light.mDirection[1], light.mDirection[2], 0.0 );

        builtLight->setLight(
            color,
            ( frame *
              Core::Vector4 { light.mPosition[0], light.mPosition[1], light.mPosition[2], 1.0 } )
                .hnormalized(),
            -( frame.transpose().inverse() * dir ).head<3>(),
            light.mAngleInnerCone,
            light.mAngleOuterCone,
            LightData::LightAttenuation( light.mAttenuationConstant,
                                         light.mAttenuationLinear,
                                         light.mAttenuationQuadratic ) );
    } break;

    case LightData::AREA_LIGHT: {
        LOG( logWARNING ) << "Light " << builtLight->getName()
                          << " : AREA light are not yet supported.";
    } break;
    default: {
        LOG( logWARNING ) << "Light " << builtLight->getName() << " : unknown type.";
    } break;
    }
    return builtLight;
}

Core::Matrix4 AssimpLightDataLoader::loadLightFrame( const aiScene* scene,
                                                     const Core::Matrix4& parentFrame,
                                                     const std::string& lightName ) const {
    const aiNode* lightNode = scene->mRootNode->FindNode( lightName.c_str() );

    if ( lightNode != nullptr ) {

        auto t0 = Core::Matrix4::NullaryExpr(
            [&scene]( int i, int j ) { return scene->mRootNode->mTransformation[i][j]; } );
        auto t1 = Core::Matrix4::NullaryExpr(
            [&lightNode]( int i, int j ) { return lightNode->mTransformation[i][j]; } );

        return parentFrame * t0 * t1;
    }
    return parentFrame;
}

std::string AssimpLightDataLoader::fetchName( const aiLight& light ) const {
    return assimpToCore( light.mName );
}

LightData::LightType AssimpLightDataLoader::fetchType( const aiLight& light ) const {
    switch ( light.mType ) {
    case aiLightSource_DIRECTIONAL: {
        return LightData::DIRECTIONAL_LIGHT;
    }

    case aiLightSource_POINT: {
        return LightData::POINT_LIGHT;
    }

    case aiLightSource_SPOT: {
        return LightData::SPOT_LIGHT;
    }

    case aiLightSource_AREA: {
        return LightData::AREA_LIGHT;
    }

    case aiLightSource_UNDEFINED:
    default: {
        //                LOG(ERROR) << "Light " << name.C_Str() << " has undefined type.";
        return LightData::UNKNOWN;
    }
    }
}

} // namespace IO
} // namespace Ra
