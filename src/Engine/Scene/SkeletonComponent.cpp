#include <Engine/Scene/SkeletonComponent.hpp>

#include <fstream>
#include <iostream>
#include <queue>

#include <Core/Animation/KeyFramedValueInterpolators.hpp>
#include <Core/Animation/Pose.hpp>
#include <Core/Asset/HandleToSkeleton.hpp>
#include <Core/Containers/AlignedStdVector.hpp>
#include <Core/Containers/MakeShared.hpp>
#include <Core/Geometry/TriangleMesh.hpp>
#include <Core/Math/Math.hpp> // areApproxEqual

#include <Engine/Data/BlinnPhongMaterial.hpp>
#include <Engine/Data/Mesh.hpp>
#include <Engine/Data/ShaderConfigFactory.hpp>
#include <Engine/Rendering/RenderObject.hpp>
#include <Engine/Rendering/RenderObjectManager.hpp>
#include <Engine/Rendering/RenderTechnique.hpp>
#include <Engine/Scene/ComponentMessenger.hpp>

using KeyFramedValue = Ra::Core::Animation::KeyFramedValue<Ra::Core::Transform>;

using Ra::Core::Animation::HandleArray;
using Ra::Core::Animation::Skeleton;
using Ra::Core::Animation::WeightMatrix;
using Ra::Engine::Scene::ComponentMessenger;

using namespace Ra::Core::Utils; // log
using SpaceType = Ra::Core::Animation::HandleArray::SpaceType;

namespace Ra {
namespace Engine {
namespace Scene {

std::shared_ptr<Data::Mesh> SkeletonComponent::s_boneMesh {nullptr};
std::shared_ptr<Data::BlinnPhongMaterial> SkeletonComponent::s_boneMaterial {nullptr};
std::shared_ptr<Rendering::RenderTechnique> SkeletonComponent::s_boneRenderTechnique {nullptr};

SkeletonComponent::SkeletonComponent( const std::string& name, Entity* entity ) :
    Component( name, entity ) {}

SkeletonComponent::~SkeletonComponent() {}

// Component interface

bool SkeletonComponent::canEdit( const Index& roIdx ) const {
    // returns true if the roIdx is one of our bones.
    return ( m_boneMap.find( roIdx ) != m_boneMap.end() );
}

Core::Transform SkeletonComponent::getTransform( const Index& roIdx ) const {
    CORE_ASSERT( canEdit( roIdx ), "Transform is not editable" );
    const uint boneIdx = m_boneMap.at( roIdx );
    return m_skel.getPose( SpaceType::MODEL )[boneIdx];
}

void SkeletonComponent::setTransform( const Index& roIdx, const Core::Transform& transform ) {
    CORE_ASSERT( canEdit( roIdx ), "Transform is not editable" );
    const uint boneIdx                = m_boneMap.at( roIdx );
    const Core::Transform& TBoneModel = m_skel.getTransform( boneIdx, SpaceType::MODEL );
    const Core::Transform& TBoneLocal = m_skel.getTransform( boneIdx, SpaceType::LOCAL );
    auto diff                         = TBoneModel.inverse() * transform;
    m_skel.setTransform( boneIdx, TBoneLocal * diff, SpaceType::LOCAL );
}

// Build from fileData

void SkeletonComponent::handleSkeletonLoading( const Core::Asset::HandleData* data ) {
    m_skelName = data->getName();

    m_skel.setName( data->getName() );

    Core::Asset::createSkeleton( *data, m_skel );

    m_refPose = m_skel.getPose( SpaceType::LOCAL );

    setupSkeletonDisplay();
    setupIO();
}

void SkeletonComponent::handleAnimationLoading(
    const std::vector<Core::Asset::AnimationData*>& data ) {
    CORE_ASSERT( ( m_skel.size() != 0 ), "A Skeleton should be loaded first." );
    m_animations.clear();
    m_animations.reserve( data.size() );

    auto pose = m_skel.getPose( SpaceType::LOCAL );
    for ( uint n = 0; n < data.size(); ++n )
    {
        m_animations.emplace_back();
        m_animations.back().reserve( m_skel.size() );
        auto handleAnim = data[n]->getHandleData();
        for ( uint i = 0; i < m_skel.size(); ++i )
        {
            auto it =
                std::find_if( handleAnim.cbegin(), handleAnim.cend(), [this, i]( const auto& ha ) {
                    return m_skel.getLabel( i ) == ha.m_name;
                } );
            if ( it == handleAnim.cend() )
            { m_animations.back().push_back( KeyFramedValue( 0_ra, pose[i] ) ); }
            else
            { m_animations.back().push_back( it->m_anim ); }
        }
    }
    if ( m_animations.size() == 0 )
    {
        m_animations.emplace_back();
        for ( uint i = 0; i < m_skel.size(); ++i )
        {
            m_animations[0].push_back( KeyFramedValue( 0_ra, pose[i] ) );
        }
    }
    m_animationID   = 0;
    m_animationTime = 0_ra;
}

// Skeleton-based animation data

void SkeletonComponent::setSkeleton( const Skeleton& skel ) {
    m_skel    = skel;
    m_refPose = skel.getPose( SpaceType::LOCAL );
    setupSkeletonDisplay();
}

SkeletonComponent::Animation& SkeletonComponent::addNewAnimation() {
    m_animations.emplace_back();
    for ( uint i = 0; i < m_skel.size(); ++i )
    {
        m_animations.back().push_back( KeyFramedValue( 0_ra, m_refPose[i] ) );
    }
    return m_animations.back();
}

void SkeletonComponent::removeAnimation( const size_t i ) {
    CORE_ASSERT( i < m_animations.size(), "Out of bound index." );
    m_animations.erase( m_animations.begin() + i );
    m_animationID = i > 1 ? i - 1 : 0;
}

void SkeletonComponent::useAnimation( const size_t i ) {
    if ( i < m_animations.size() ) { m_animationID = i; }
}

size_t SkeletonComponent::getAnimationId() const {
    return m_animationID;
}

// Animation Process

void SkeletonComponent::update( Scalar t ) {
    m_wasReset = Core::Math::areApproxEqual( t, 0_ra );
    if ( m_wasReset )
    {
        m_animationTime = t;
        m_skel.setPose( m_refPose, SpaceType::LOCAL );

        updateDisplay();
        return;
    }

    m_animationTime = m_speed * t;
    Scalar lastTime = 0;
    for ( auto boneAnim : m_animations[m_animationID] )
    {
        lastTime = std::max( lastTime, *boneAnim.getTimes().rbegin() );
    }
    if ( m_autoRepeat )
    {
        if ( !m_pingPong ) { m_animationTime = std::fmod( m_animationTime, lastTime ); }
        else
        {
            m_animationTime = std::fmod( m_animationTime, 2 * lastTime );
            if ( m_animationTime > lastTime ) { m_animationTime = 2 * lastTime - m_animationTime; }
        }
    }
    else if ( m_pingPong )
    {
        if ( m_animationTime > 2 * lastTime ) { m_animationTime = 0_ra; }
        else if ( m_animationTime > lastTime )
        { m_animationTime = 2 * lastTime - m_animationTime; }
    }

    // get the current pose from the animation
    Core::Animation::Pose pose = m_skel.getPose( SpaceType::LOCAL );
    if ( !m_animations.empty() )
    {
#pragma omp parallel for
        for ( int i = 0; i < int( m_animations[m_animationID].size() ); ++i )
        {
            pose[uint( i )] = m_animations[m_animationID][uint( i )].at(
                m_animationTime, Core::Animation::linearInterpolate<Core::Transform> );
        }
    }
    else
    { pose = m_refPose; }
    m_skel.setPose( pose, SpaceType::LOCAL );

    updateDisplay();
}

Scalar SkeletonComponent::getAnimationTime() const {
    return m_animationTime;
}

Scalar SkeletonComponent::getAnimationDuration() const {
    if ( m_animations.empty() ) { return 0_ra; }
    Scalar startTime = std::numeric_limits<Scalar>::max();
    Scalar endTime   = 0;
    for ( auto boneAnim : m_animations[m_animationID] )
    {
        const auto& times = boneAnim.getTimes();
        startTime         = std::min( startTime, *times.begin() );
        endTime           = std::max( endTime, *times.rbegin() );
    }
    return endTime - startTime;
}

void SkeletonComponent::setSpeed( const Scalar value ) {
    m_speed = value;
}

Scalar SkeletonComponent::getSpeed() const {
    return m_speed;
}

void SkeletonComponent::autoRepeat( const bool status ) {
    m_autoRepeat = status;
}

bool SkeletonComponent::isAutoRepeat() const {
    return m_autoRepeat;
}

void SkeletonComponent::pingPong( const bool status ) {
    m_pingPong = status;
}

bool SkeletonComponent::isPingPong() const {
    return m_pingPong;
}

// Skeleton display

void SkeletonComponent::setXray( bool on ) const {
    for ( const auto& b : m_boneDrawables )
    {
        b->setXRay( on );
    }
}

bool SkeletonComponent::isXray() const {
    if ( m_boneDrawables.size() == 0 ) return false;
    return m_boneDrawables.front()->isXRay();
}

void SkeletonComponent::toggleSkeleton( const bool status ) {
    for ( const auto& b : m_boneDrawables )
    {
        b->setVisible( status );
    }
}

bool SkeletonComponent::isShowingSkeleton() const {
    if ( m_boneDrawables.size() == 0 ) return false;
    return m_boneDrawables.front()->isVisible();
}

Core::Geometry::TriangleMesh makeBoneShape() {
    // Bone along Z axis.
    Core::Geometry::TriangleMesh mesh;
    const Scalar l = 0.1_ra;
    const Scalar w = 0.1_ra;
    mesh.setVertices( {Core::Vector3( 0, 0, 0 ),
                       Core::Vector3( 0, 0, 1 ),
                       Core::Vector3( 0, w, l ),
                       Core::Vector3( w, 0, l ),
                       Core::Vector3( 0, -w, l ),
                       Core::Vector3( -w, 0, l )} );

    mesh.setNormals( {Core::Vector3( 0, 0, -1 ),
                      Core::Vector3( 0, 0, 1 ),
                      Core::Vector3( 0, 1, 0 ),
                      Core::Vector3( 1, 0, 0 ),
                      Core::Vector3( 0, -1, 0 ),
                      Core::Vector3( -1, 0, 0 )} );

    mesh.setIndices( {Core::Vector3ui( 0, 2, 3 ),
                      Core::Vector3ui( 0, 5, 2 ),
                      Core::Vector3ui( 0, 3, 4 ),
                      Core::Vector3ui( 0, 4, 5 ),
                      Core::Vector3ui( 1, 3, 2 ),
                      Core::Vector3ui( 1, 2, 5 ),
                      Core::Vector3ui( 1, 4, 3 ),
                      Core::Vector3ui( 1, 5, 4 )} );
    return mesh;
}

void SkeletonComponent::setupSkeletonDisplay() {
    m_renderObjects.clear();
    m_boneDrawables.clear();
    if ( !s_boneMesh )
    {
        s_boneMesh = std::make_shared<Data::Mesh>( "Bone Mesh" );
        s_boneMesh->loadGeometry( makeBoneShape() );
        s_boneMaterial.reset( new Data::BlinnPhongMaterial( "Bone Material" ) );
        s_boneMaterial->m_kd = Color::Grey( 0.4_ra );
        s_boneMaterial->m_ks = Color::Black();
        s_boneRenderTechnique.reset( new Rendering::RenderTechnique );
        s_boneRenderTechnique->setConfiguration(
            *Data::ShaderConfigurationFactory::getConfiguration( "BlinnPhong" ) );
        s_boneRenderTechnique->setParametersProvider( s_boneMaterial );
    }
    for ( uint i = 0; i < m_skel.size(); ++i )
    {
        if ( !m_skel.m_graph.isLeaf( i ) && !m_skel.m_graph.isRoot( i ) &&
             m_skel.getLabel( i ).find( "_$AssimpFbx$_" ) == std::string::npos )
        {
            std::string name = m_skel.getLabel( i ) + "_" + std::to_string( i );
            auto ro          = new Engine::Rendering::RenderObject(
                name, this, Rendering::RenderObjectType::Geometry );
            ro->setRenderTechnique( s_boneRenderTechnique );
            ro->setMesh( s_boneMesh );
            ro->setMaterial( s_boneMaterial );
            ro->setXRay( false );
            addRenderObject( ro );
            m_boneMap[m_renderObjects.back()] = i;
            m_boneDrawables.push_back( ro );
        }
        else
        { LOG( logDEBUG ) << "Bone " << m_skel.getLabel( i ) << " not displayed."; }
    }
    updateDisplay();
}

void SkeletonComponent::printSkeleton( const Skeleton& skeleton ) {
    std::deque<uint> queue;
    std::deque<int> levels;

    queue.push_back( 0 );
    levels.push_back( 0 );
    while ( !queue.empty() )
    {
        uint i = queue.front();
        queue.pop_front();
        int level = levels.front();
        levels.pop_front();
        std::cout << i << " " << skeleton.getLabel( i ) << "\t";
        for ( const auto& c : skeleton.m_graph.children()[i] )
        {
            queue.push_back( c );
            levels.push_back( level + 1 );
        }

        if ( levels.front() != level ) { std::cout << std::endl; }
    }
}

void SkeletonComponent::updateDisplay() {
    for ( auto& bone : m_boneDrawables )
    {
        uint boneIdx = m_boneMap.at( bone->getIndex() );
        Core::Vector3 start;
        Core::Vector3 end;
        m_skel.getBonePoints( boneIdx, start, end );

        Core::Transform scale = Core::Transform::Identity();
        scale.scale( ( end - start ).norm() );

        Core::Quaternion rot =
            Core::Quaternion::FromTwoVectors( Core::Vector3::UnitZ(), end - start );

        Core::Transform boneTransform = m_skel.getTransform( boneIdx, SpaceType::MODEL );
        Core::Matrix3 rotation        = rot.toRotationMatrix();
        Core::Transform drawTransform;
        drawTransform.linear()      = rotation;
        drawTransform.translation() = boneTransform.translation();

        bone->setLocalTransform( drawTransform * scale );
    }
}

// Component Communication (CC)

void SkeletonComponent::setupIO() {
    ComponentMessenger::CallbackTypes<Skeleton>::Getter skelOut =
        std::bind( &SkeletonComponent::getSkeleton, this );
    ComponentMessenger::getInstance()->registerOutput<Skeleton>(
        getEntity(), this, m_skelName, skelOut );

    using BoneMap = std::map<Index, uint>;
    ComponentMessenger::CallbackTypes<BoneMap>::Getter boneMapOut =
        std::bind( &SkeletonComponent::getBoneRO2idx, this );
    ComponentMessenger::getInstance()->registerOutput<BoneMap>(
        getEntity(), this, m_skelName, boneMapOut );

    ComponentMessenger::CallbackTypes<Core::Animation::RefPose>::Getter refpOut =
        std::bind( &SkeletonComponent::getRefPoseOutput, this );
    ComponentMessenger::getInstance()->registerOutput<Core::Animation::Pose>(
        getEntity(), this, m_skelName, refpOut );

    ComponentMessenger::CallbackTypes<Animation>::Getter animOut =
        std::bind( &SkeletonComponent::getAnimationOutput, this );
    ComponentMessenger::getInstance()->registerOutput<Animation>(
        getEntity(), this, m_skelName, animOut );

    ComponentMessenger::CallbackTypes<Scalar>::Getter timeOut =
        std::bind( &SkeletonComponent::getTimeOutput, this );
    ComponentMessenger::getInstance()->registerOutput<Scalar>(
        getEntity(), this, m_skelName, timeOut );

    ComponentMessenger::CallbackTypes<bool>::Getter resetOut =
        std::bind( &SkeletonComponent::getWasReset, this );
    ComponentMessenger::getInstance()->registerOutput<bool>(
        getEntity(), this, m_skelName, resetOut );
}

const std::map<Index, uint>* SkeletonComponent::getBoneRO2idx() const {
    return &m_boneMap;
}

const Core::Animation::RefPose* SkeletonComponent::getRefPoseOutput() const {
    return &m_refPose;
}

const SkeletonComponent::Animation* SkeletonComponent::getAnimationOutput() const {
    if ( m_animations.empty() ) { return nullptr; }
    return &m_animations[m_animationID];
}

const Scalar* SkeletonComponent::getTimeOutput() const {
    return &m_animationTime;
}

const bool* SkeletonComponent::getWasReset() const {
    return &m_wasReset;
}

} // namespace Scene
} // namespace Engine
} // namespace Ra
