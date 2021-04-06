#include <IO/AssimpLoader/AssimpAnimationDataLoader.hpp>

#include <assimp/scene.h>
#include <set>

#include <Core/Animation/KeyFramedValue.hpp>
#include <Core/Animation/KeyFramedValueInterpolators.hpp>
#include <Core/Asset/AnimationData.hpp>
#include <Core/Math/Math.hpp>
#include <Core/Utils/Log.hpp>

#include <IO/AssimpLoader/AssimpWrapper.hpp>

namespace Ra {
namespace IO {

using namespace Core::Utils; // log
using namespace Core::Animation;
using namespace Core::Asset;

/// CONSTRUCTOR
AssimpAnimationDataLoader::AssimpAnimationDataLoader( const bool VERBOSE_MODE ) :
    DataLoader<AnimationData>( VERBOSE_MODE ) {}

/// DESTRUCTOR
AssimpAnimationDataLoader::~AssimpAnimationDataLoader() = default;

/// LOADING
void AssimpAnimationDataLoader::loadData( const aiScene* scene,
                                          std::vector<std::unique_ptr<AnimationData>>& data ) {
    data.clear();

    if ( scene == nullptr )
    {
        LOG( logDEBUG ) << "AssimpAnimationDataLoader : scene is nullptr.";
        return;
    }

    if ( !sceneHasAnimation( scene ) )
    {
        LOG( logDEBUG ) << "AssimpAnimationDataLoader : scene has no animation.";
        return;
    }

    if ( m_verbose )
    {
        LOG( logDEBUG ) << "File contains animation.";
        LOG( logDEBUG ) << "Animation Loading begin...";
    }

    loadAnimationData( scene, data );

    if ( m_verbose ) { LOG( logDEBUG ) << "Animation Loading end.\n"; }
}

/// QUERY
bool AssimpAnimationDataLoader::sceneHasAnimation( const aiScene* scene ) const {
    return ( sceneAnimationSize( scene ) != 0 );
}

uint AssimpAnimationDataLoader::sceneAnimationSize( const aiScene* scene ) const {
    if ( scene->HasAnimations() ) { return scene->mNumAnimations; }
    return 0;
}

/// NAME
void AssimpAnimationDataLoader::fetchName( const aiAnimation* anim, AnimationData* data ) const {
    data->setName( assimpToCore( anim->mName ) );
}

/// TIME
void AssimpAnimationDataLoader::fetchTime( const aiAnimation* anim, AnimationData* data ) const {
    const auto tick     = Scalar( anim->mTicksPerSecond );
    const auto duration = Scalar( anim->mDuration );

    AnimationTime time;
    AnimationTime::Time dt;
    time.setStart( 0.0 );
    if ( Ra::Core::Math::areApproxEqual( tick, 0_ra ) )
    {
        dt = 0.0;
        time.setEnd( duration );
    }
    else
    {
        dt = Scalar( 1.0 ) / tick;
        time.setEnd( dt * duration );
    }
    data->setTime( time );
    data->setTimeStep( dt );
}

/// KEY FRAME
void AssimpAnimationDataLoader::loadAnimationData(
    const aiScene* scene,
    std::vector<std::unique_ptr<AnimationData>>& data ) const {
    const uint size = sceneAnimationSize( scene );
    data.resize( size );
    for ( uint i = 0; i < size; ++i )
    {
        aiAnimation* anim       = scene->mAnimations[i];
        AnimationData* animData = new AnimationData();
        fetchName( anim, animData );
        fetchTime( anim, animData );
        fetchAnimation( anim, animData );
        if ( m_verbose ) { animData->displayInfo(); }
        data[i].reset( animData );
    }
}

void AssimpAnimationDataLoader::fetchAnimation( const aiAnimation* anim,
                                                AnimationData* data ) const {
    const uint size    = anim->mNumChannels;
    AnimationTime time = data->getTime();
    std::vector<HandleAnimation> keyFrame( size );
    for ( uint i = 0; i < size; ++i )
    {
        fetchHandleAnimation( anim->mChannels[i], keyFrame[i], data->getTimeStep() );
        time.extends( keyFrame[i].m_animationTime );
    }
    data->setHandleData( keyFrame );
    data->setTime( time );
}

void AssimpAnimationDataLoader::fetchHandleAnimation( aiNodeAnim* node,
                                                      HandleAnimation& data,
                                                      const AnimationTime::Time dt ) const {
    const uint T_size = node->mNumPositionKeys;
    const uint R_size = node->mNumRotationKeys;
    const uint S_size = node->mNumScalingKeys;
    data.m_name       = assimpToCore( node->mNodeName );

    // check if there are keyframes
    if ( T_size == 0 && R_size == 0 && S_size == 0 ) { return; }

    // fetch the first keyframes
    std::set<AnimationTime::Time> keyFrame;
    AnimationTime::Time time( node->mPositionKeys[0].mTime );
    // According to Assimp's doc, time can be negative so deal with it
    AnimationTime::Time timeOffset = time < 0_ra ? -time : 0_ra;
    keyFrame.insert( time + timeOffset );
    KeyFramedValue<Core::Vector3> tr( time + timeOffset,
                                      assimpToCore( node->mPositionKeys[0].mValue ) );
    time = Scalar( node->mRotationKeys[0].mTime );
    keyFrame.insert( time );
    KeyFramedValue<Core::Quaternion> rot( time + timeOffset,
                                          assimpToCore( node->mRotationKeys[0].mValue ) );
    time = Scalar( node->mScalingKeys[0].mTime );
    keyFrame.insert( time );
    KeyFramedValue<Core::Vector3> s( time + timeOffset,
                                     assimpToCore( node->mScalingKeys[0].mValue ) );

    // fetch the other keyframes
    for ( uint i = 1; i < T_size; ++i )
    {
        time = Scalar( node->mPositionKeys[i].mTime ) + timeOffset;
        tr.insertKeyFrame( time, assimpToCore( node->mPositionKeys[i].mValue ) );
        keyFrame.insert( time );
    }

    for ( uint i = 1; i < R_size; ++i )
    {
        time = Scalar( node->mRotationKeys[i].mTime ) + timeOffset;
        rot.insertKeyFrame( time, assimpToCore( node->mRotationKeys[i].mValue ) );
        keyFrame.insert( time );
    }

    for ( uint i = 1; i < S_size; ++i )
    {
        time = Scalar( node->mScalingKeys[i].mTime ) + timeOffset;
        s.insertKeyFrame( time, assimpToCore( node->mScalingKeys[i].mValue ) );
        keyFrame.insert( time );
    }

    // fill data
    data.m_animationTime = AnimationTime( *keyFrame.begin(), *keyFrame.rbegin() );
    for ( const auto& t : keyFrame )
    {
        Core::Transform T;
        T.fromPositionOrientationScale( tr.at( t, linearInterpolate<Core::Vector3> ),
                                        rot.at( t, linearInterpolate<Core::Quaternion> ),
                                        s.at( t, linearInterpolate<Core::Vector3> ) );
        data.m_anim.insertKeyFrame( ( Ra::Core::Math::areApproxEqual( dt, 0_ra ) ? t : ( dt * t ) ),
                                    T );
    }
    data.m_anim.removeKeyFrame( 0 );
}

} // namespace IO
} // namespace Ra
