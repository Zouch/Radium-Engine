#include <Engine/Scene/SkinningComponent.hpp>

#include <Core/Animation/PoseOperation.hpp>
#include <Core/Geometry/Normal.hpp>

#include <Core/Animation/DualQuaternionSkinning.hpp>
#include <Core/Animation/HandleWeightOperation.hpp>
#include <Core/Animation/LinearBlendSkinning.hpp>
#include <Core/Animation/RotationCenterSkinning.hpp>
#include <Core/Animation/StretchableTwistableBoneSkinning.hpp>
#include <Core/Geometry/DistanceQueries.hpp>
#include <Core/Geometry/TriangleOperation.hpp>
#include <Core/Utils/Color.hpp>
#include <Core/Utils/Log.hpp>

#include <Engine/RadiumEngine.hpp>
#include <Engine/Data/BlinnPhongMaterial.hpp>
#include <Engine/Data/Mesh.hpp>
#include <Engine/Data/ShaderConfigFactory.hpp>
#include <Engine/Data/Texture.hpp>
#include <Engine/Data/TextureManager.hpp>
#include <Engine/Rendering/RenderObject.hpp>
#include <Engine/Rendering/RenderObjectManager.hpp>
#include <Engine/Rendering/RenderTechnique.hpp>

using namespace Ra::Core;

using Geometry::AttribArrayGeometry;
using Geometry::PolyMesh;
using Geometry::TriangleMesh;

using namespace Animation;
using SpaceType = HandleArray::SpaceType;

using namespace Utils;

namespace Ra {
namespace Engine {
namespace Scene {

static std::string tangentName = Data::Mesh::getAttribName( Data::Mesh::VERTEX_TANGENT );
static std::string bitangentName = Data::Mesh::getAttribName( Data::Mesh::VERTEX_BITANGENT );

bool findDuplicates( const TriangleMesh& mesh,
                     std::vector<Index>& duplicatesMap ) {
    bool hasDuplicates = false;
    duplicatesMap.clear();
    const uint numVerts = mesh.vertices().size();
    duplicatesMap.resize( numVerts, Index::Invalid() );

    Vector3Array::const_iterator vertPos;
    Vector3Array::const_iterator duplicatePos;
    std::vector<std::pair<Vector3, Index>> vertices;

    for ( uint i = 0; i < numVerts; ++i )
    {
        vertices.push_back( std::make_pair( mesh.vertices()[i], Index( i ) ) );
    }

    std::sort( vertices.begin(),
               vertices.end(),
               []( std::pair<Vector3, int> a, std::pair<Vector3, int> b ) {
                   if ( a.first.x() == b.first.x() )
                   {
                       if ( a.first.y() == b.first.y() )
                           if ( a.first.z() == b.first.z() )
                               return a.second < b.second;
                           else
                               return a.first.z() < b.first.z();
                       else
                           return a.first.y() < b.first.y();
                   }
                   return a.first.x() < b.first.x();
               } );
    // Here vertices contains vertex pos and idx, with equal
    // vertices contiguous, sorted by idx, so checking if current
    // vertex equals the previous one state if its a duplicated
    // vertex position.
    duplicatesMap[vertices[0].second] = vertices[0].second;
    for ( uint i = 1; i < numVerts; ++i )
    {
        if ( vertices[i].first == vertices[i - 1].first )
        {
            duplicatesMap[vertices[i].second] = duplicatesMap[vertices[i - 1].second];
            hasDuplicates                     = true;
        }
        else
        { duplicatesMap[vertices[i].second] = vertices[i].second; }
    }

    return hasDuplicates;
}

TriangleMesh triangulate( const PolyMesh& polyMesh )
{
    TriangleMesh res;
    res.setVertices( polyMesh.vertices() );
    res.setNormals( polyMesh.normals() );
    res.copyAllAttributes( polyMesh );
    VectorArray<Vector3ui> indices;
    // using the same triangulation as in Ra::Engine::PolyMesh::triangulate
    for ( const auto& face : polyMesh.getIndices() )
    {
        if ( face.size() == 3 ) { indices.push_back( face ); }
        else
        {
            int minus {int( face.size() ) - 1};
            int plus {0};
            while ( plus + 1 < minus )
            {
                if ( ( plus - minus ) % 2 )
                {
                    indices.emplace_back( face[plus], face[plus + 1], face[minus] );
                    ++plus;
                }
                else
                {
                    indices.emplace_back( face[minus], face[plus], face[minus - 1] );
                    --minus;
                }
            }
        }
    }
    res.setIndices( std::move( indices ) );
    return res;
}

void SkinningComponent::initialize() {
    auto compMsg = ComponentMessenger::getInstance();
    // get the current animation data.
    bool hasSkel     = compMsg->canGet<Skeleton>( getEntity(), m_skelName );
    bool hasRefPose  = compMsg->canGet<RefPose>( getEntity(), m_skelName );
    bool hasTriMesh  = compMsg->canGet<TriangleMesh>( getEntity(), m_meshName );
    m_meshIsPoly     = compMsg->canGet<PolyMesh>( getEntity(), m_meshName );

    if ( hasSkel && hasRefPose && (hasTriMesh || m_meshIsPoly) )
    {
        m_renderObjectReader = compMsg->getterCallback<Index>( getEntity(), m_meshName );
        m_skeletonGetter     = compMsg->getterCallback<Skeleton>( getEntity(), m_skelName );
        if ( !m_meshIsPoly )
        {
            m_triMeshWriter = compMsg->rwCallback<TriangleMesh>( getEntity(), m_meshName );
        }
        else
        {
            m_polyMeshWriter = compMsg->rwCallback<PolyMesh>( getEntity(), m_meshName );
        }

        // copy mesh triangles and find duplicates for normal computation.
        if ( !m_meshIsPoly )
        {
            m_refData.m_referenceMesh = *m_triMeshWriter();
        }
        else {
            m_refData.m_referenceMesh = triangulate( *m_polyMeshWriter() );
        }
        if ( !m_refData.m_referenceMesh.hasAttrib( tangentName ) &&
             !m_refData.m_referenceMesh.hasAttrib( bitangentName ) )
        {
            const auto& normals = m_refData.m_referenceMesh.normals();
            Vector3Array tangents( normals.size() );
            Vector3Array bitangents( normals.size() );
#pragma omp parallel for
            for ( int i = 0; i < normals.size(); ++i )
            {
                Core::Math::getOrthogonalVectors( normals[i], tangents[i], bitangents[i] );
            }
            m_refData.m_referenceMesh.addAttrib( tangentName, std::move( tangents ) );
            m_refData.m_referenceMesh.addAttrib( bitangentName, std::move( bitangents ) );
        }
        else if ( !m_refData.m_referenceMesh.hasAttrib( tangentName ) )
        {
            const auto& normals = m_refData.m_referenceMesh.normals();
            const auto& bitangentHandle = m_refData.m_referenceMesh.getAttribHandle<Vector3>( bitangentName );
            const auto& bitangents = m_refData.m_referenceMesh.getAttrib( bitangentHandle ).data();
            Vector3Array tangents( normals.size() );
#pragma omp parallel for
            for ( int i = 0; i < normals.size(); ++i )
            {
                tangents[i] = bitangents[i].cross( normals[i] );
            }
            m_refData.m_referenceMesh.addAttrib( tangentName, std::move( tangents ) );
        }
        else if ( !m_refData.m_referenceMesh.hasAttrib( bitangentName ) )
        {
            const auto& normals = m_refData.m_referenceMesh.normals();
            const auto& tangentHandle = m_refData.m_referenceMesh.getAttribHandle<Vector3>( tangentName );
            const auto& tangents = m_refData.m_referenceMesh.getAttrib( tangentHandle ).data();
            Vector3Array bitangents( normals.size() );
#pragma omp parallel for
            for ( int i = 0; i < normals.size(); ++i )
            {
                bitangents[i] = normals[i].cross( tangents[i] );
            }
            m_refData.m_referenceMesh.addAttrib( bitangentName, std::move( bitangents ) );
        }
        findDuplicates( m_refData.m_referenceMesh, m_duplicatesMap );

        // get other data
        m_refData.m_meshTransformInverse = m_meshFrameInv;
        m_refData.m_skeleton = *m_skeletonGetter();
        createWeightMatrix();

        // initialize frame data
        m_frameData.m_skeleton = m_refData.m_skeleton;

        m_frameData.m_currentPosition  = m_refData.m_referenceMesh.vertices();
        m_frameData.m_currentNormal = m_refData.m_referenceMesh.normals();
        const auto& tangentHandle = m_refData.m_referenceMesh.getAttribHandle<Vector3>( tangentName );
        m_frameData.m_currentTangent = m_refData.m_referenceMesh.getAttrib( tangentHandle ).data();
        const auto& bitangentHandle = m_refData.m_referenceMesh.getAttribHandle<Vector3>( bitangentName );
        m_frameData.m_currentBitangent = m_refData.m_referenceMesh.getAttrib( bitangentHandle ).data();

        m_frameData.m_frameCounter = 0;
        m_frameData.m_doSkinning   = true;
        m_frameData.m_doReset      = false;

        // setup comp data
        m_isReady = true;
        m_forceUpdate = true;
        setupSkinningType( m_skinningType );
        setupSkinningType( STBS_LBS ); // ensure weights are present for display

        // prepare RO for skinning weights display
        auto ro         = getRoMgr()->getRenderObject( *m_renderObjectReader() );
        m_baseMaterial  = ro->getMaterial();

        auto attrUV = Data::Mesh::getAttribName( Data::Mesh::VERTEX_TEXCOORD );
        AttribArrayGeometry* geom;
        if ( !m_meshIsPoly )
        {
            geom = const_cast<TriangleMesh*>( m_triMeshWriter() );
        }
        else {
            geom = const_cast<PolyMesh*>( m_polyMeshWriter() );
        }
        if ( geom->hasAttrib( attrUV ) )
        {
            auto handle = geom->getAttribHandle<Vector3>( attrUV );
            m_baseUV    = geom->getAttrib( handle ).data();
        }

        auto mat  = new Data::BlinnPhongMaterial( "SkinningWeights_Mat" );
        mat->m_kd = Color::Skin();
        mat->m_ks = Color::White();
        // assign texture
        Data::TextureParameters texParam;
        texParam.name = ":/Textures/Influence0.png";
        auto tex = RadiumEngine::getInstance()->getTextureManager()->getOrLoadTexture(
            texParam );
        mat->addTexture( Data::BlinnPhongMaterial::TextureSemantic::TEX_DIFFUSE, tex );
        m_weightMaterial.reset( mat );
        // compute default weights uv
        showWeightsType( STANDARD );
    }
}

void uniformNormal( const Vector3Array& p,
                    const AlignedStdVector<Vector3ui>& T,
                    const std::vector<Index>& duplicateTable,
                    Vector3Array& normal ) {
    const uint N = p.size();
    normal.clear();
    normal.resize( N, Vector3::Zero() );

    for ( const auto& t : T )
    {
        const Index& i = duplicateTable.at( t( 0 ) );
        const Index& j = duplicateTable.at( t( 1 ) );
        const Index& k = duplicateTable.at( t( 2 ) );
        const Vector3 triN   = Geometry::triangleNormal( p[i], p[j], p[k] );
        if ( !triN.allFinite() ) { continue; }
        normal[i] += triN;
        normal[j] += triN;
        normal[k] += triN;
    }

#pragma omp parallel for
    for ( int i = 0; i < N; ++i )
    {
        if ( !normal[i].isApprox( Vector3::Zero() ) ) { normal[i].normalize(); }
    }

#pragma omp parallel for
    for ( int i = 0; i < N; ++i )
    {
        normal[i] = normal[duplicateTable[i]];
    }
}

void SkinningComponent::skin() {
    CORE_ASSERT( m_isReady, "Skinning is not setup" );

    const Skeleton* skel = m_skeletonGetter();

    bool reset = ComponentMessenger::getInstance()->get<bool>( getEntity(), m_skelName );

    // Reset the skin if it wasn't done before
    if ( reset && !m_frameData.m_doReset )
    {
        m_frameData.m_doReset      = true;
        m_frameData.m_frameCounter = 0;
        m_forceUpdate              = true;
    }
    const auto prevPose = m_frameData.m_skeleton.getPose( SpaceType::MODEL );
    m_frameData.m_skeleton = *skel;
    auto currentPose = m_frameData.m_skeleton.getPose( SpaceType::MODEL );
    if ( m_smartStretch ){ applySmartStretch( currentPose ); }
    m_frameData.m_skeleton.setPose( currentPose, SpaceType::MODEL );
    if ( !areEqual( currentPose, prevPose ) ||
         m_forceUpdate )
    {
        m_forceUpdate            = false;
        m_frameData.m_doSkinning = true;
        m_frameData.m_frameCounter++;

        const auto tangentHandle = m_refData.m_referenceMesh.getAttribHandle<Vector3>( tangentName );
        const Vector3Array& tangents = m_refData.m_referenceMesh.getAttrib( tangentHandle ).data();
        const auto bitangentHandle = m_refData.m_referenceMesh.getAttribHandle<Vector3>( bitangentName );
        const Vector3Array& bitangents = m_refData.m_referenceMesh.getAttrib( bitangentHandle ).data();

        switch ( m_skinningType )
        {
        case LBS:
        {
            linearBlendSkinning( m_refData, tangents, bitangents, m_frameData );
            break;
        }
        case DQS:
        {
            dualQuaternionSkinning( m_refData, tangents, bitangents, m_frameData );
            break;
        }
        case COR:
        {
            centerOfRotationSkinning( m_refData, tangents, bitangents, m_frameData );
            break;
        }
        case STBS_LBS:
        {
            linearBlendSkinningSTBS( m_refData, tangents, bitangents, m_frameData );
            break;
        }
        case STBS_DQS:
        {
            dualQuaternionSkinningSTBS( m_refData, tangents, bitangents, m_frameData );
            break;
        }
        }

        if ( m_normalSkinning == GEOMETRIC )
        {
            uniformNormal( m_frameData.m_currentPosition,
                           m_refData.m_referenceMesh.getIndices(),
                           m_duplicatesMap,
                           m_frameData.m_currentNormal );
#pragma omp parallel for
            for ( int i = 0; i < m_frameData.m_currentNormal.size(); ++i )
            {
                Core::Math::getOrthogonalVectors( m_frameData.m_currentNormal[i],
                                                  m_frameData.m_currentTangent[i],
                                                  m_frameData.m_currentBitangent[i] );
            }
        }
    }
}

void SkinningComponent::endSkinning() {
    if ( m_frameData.m_doSkinning )
    {
        AttribArrayGeometry* geom;
        if ( !m_meshIsPoly )
        {
            geom = const_cast<TriangleMesh*>( m_triMeshWriter() );
        }
        else {
            geom = const_cast<PolyMesh*>( m_polyMeshWriter() );
        }

        geom->setVertices( m_frameData.m_currentPosition );
        geom->setNormals( m_frameData.m_currentNormal );
        auto handle = geom->getAttribHandle<Vector3>( tangentName );
        if ( handle.idx().isValid() )
        {
            geom->getAttrib( handle ).setData( m_frameData.m_currentTangent );
        }
        handle = geom->getAttribHandle<Vector3>( bitangentName );
        if ( handle.idx().isValid() )
        {
            geom->getAttrib( handle ).setData( m_frameData.m_currentBitangent );
        }

        m_frameData.m_doReset    = false;
        m_frameData.m_doSkinning = false;
    }
}

void SkinningComponent::handleSkinDataLoading( const Asset::HandleData* data,
                                               const std::string& meshName,
                                               const Transform& meshFrame ) {
    m_skelName = data->getName();
    m_meshName = meshName;
    setupIO( meshName );
    m_meshFrameInv = meshFrame.inverse();
    for ( const auto& bone : data->getComponentData() )
    {
        auto it_w = bone.m_weights.find( meshName );
        if ( it_w != bone.m_weights.end() )
        {
            m_loadedWeights[bone.m_name] = it_w->second;
            auto it_b                    = bone.m_bindMatrices.find( meshName );
            if ( it_b != bone.m_bindMatrices.end() )
            { m_loadedBindMatrices[bone.m_name] = it_b->second; }
            else
            {
                LOG( logWARNING ) << "Bone " << bone.m_name
                                  << " has skinning weights but no bind matrix. Using Identity.";
                m_loadedBindMatrices[bone.m_name] = Transform::Identity();
            }
        }
    }
}

void SkinningComponent::createWeightMatrix() {
    m_refData.m_bindMatrices.resize( m_refData.m_skeleton.size(), Transform::Identity() );
    m_refData.m_weights.resize( int( m_refData.m_referenceMesh.vertices().size() ),
                                m_refData.m_skeleton.size() );
    std::vector<Eigen::Triplet<Scalar>> triplets;
    for ( uint col = 0; col < m_refData.m_skeleton.size(); ++col )
    {
        std::string boneName = m_refData.m_skeleton.getLabel( col );
        auto it              = m_loadedWeights.find( boneName );
        if ( it != m_loadedWeights.end() )
        {
            const auto& W   = it->second;
            for ( uint i = 0; i < W.size(); ++i )
            {
                const auto& w = W[i];
                int row{int( w.first )};
                CORE_ASSERT( row < m_refData.m_weights.rows(),
                             "Weights are incompatible with mesh." );
                triplets.push_back( {row, int( col ), w.second} );
            }
            m_refData.m_bindMatrices[col] = m_loadedBindMatrices[boneName];
        }
    }
    m_refData.m_weights.setFromTriplets( triplets.begin(), triplets.end() );

    checkWeightMatrix( m_refData.m_weights, false, true );

    if ( normalizeWeights( m_refData.m_weights, true ) )
    { LOG( logINFO ) << "Skinning weights have been normalized"; }
}

void SkinningComponent::applySmartStretch( Pose& pose ) {
    auto refSkel = m_refData.m_skeleton;
    const auto& currentSkel = m_frameData.m_skeleton;
    for( int i = 0 ; i < int( refSkel.size() ) ; ++i )
    {
        if( refSkel.m_graph.isRoot( i ) )
        {
            continue;
        }

        // get transforms
        const uint parent = refSkel.m_graph.parents()[i];
        const auto& boneModel = currentSkel.getTransform( i, SpaceType::MODEL );
        const auto& parentModel = currentSkel.getTransform( parent, SpaceType::MODEL );
        const auto& parentRef = refSkel.getTransform( parent, SpaceType::MODEL );
        const auto parentT = parentModel * parentRef.inverse( Eigen::Affine );

        // do nothing for multi-siblings
        if( refSkel.m_graph.children()[parent].size() > 1 )
        {
            refSkel.setTransform( parent, parentModel, SpaceType::MODEL );
            continue;
        }

        // rotate parent to align with bone
        Vector3 A;
        Vector3 B;
        refSkel.getBonePoints( parent, A, B );
        B = parentT * B;
        Vector3 B_ = boneModel.translation();
        auto q = Quaternion::FromTwoVectors( ( B - A ), ( B_ - A ) );
        Transform R( q );
        R.pretranslate( A );
        R.translate( -A );
        refSkel.setTransform( parent, R * parentModel, SpaceType::MODEL );
    }
    pose = refSkel.getPose( SpaceType::MODEL );
}

void SkinningComponent::setupIO( const std::string& id ) {
    auto compMsg = ComponentMessenger::getInstance();
    auto wOut = std::bind( &SkinningComponent::getWeightsOutput, this );
    compMsg->registerOutput<WeightMatrix>( getEntity(), this, id, wOut );

    auto refData = std::bind( &SkinningComponent::getSkinningRefData, this );
    compMsg->registerOutput<SkinningRefData>( getEntity(), this, id, refData );

    auto frameData = std::bind( &SkinningComponent::getSkinningFrameData, this );
    compMsg->registerOutput<SkinningFrameData>( getEntity(), this, id, frameData );
}

void SkinningComponent::setSkinningType( SkinningType type ) {
    m_skinningType = type;
    if ( m_isReady )
    {
        setupSkinningType( type );
        m_forceUpdate = true;
    }
}

void SkinningComponent::setNormalSkinning( NormalSkinning normalSkinning ) {
    m_normalSkinning = normalSkinning;
    if ( m_isReady )
    {
        m_forceUpdate = true;
    }
}

const WeightMatrix* SkinningComponent::getWeightsOutput() const {
    return &m_refData.m_weights;
}

const std::string SkinningComponent::getMeshName() const {
    return m_meshName;
}

const std::string SkinningComponent::getSkeletonName() const {
    return m_skeletonGetter()->getName();
}

void SkinningComponent::setupSkinningType( SkinningType type ) {
    CORE_ASSERT( m_isReady, "component is not ready" );
    switch ( type )
    {
    case LBS:
        break;
    case DQS:
        break;
    case COR:
    {
        // FIXME: takes time, would be nice to store them in a file and reload.
        if ( m_refData.m_CoR.empty() ) { computeCoR( m_refData ); }
        break;
    }
    case STBS_DQS:
        [[fallthrough]];
    case STBS_LBS:
    {
        if ( m_refData.m_weightSTBS.size() == 0 )
        {
            auto vertices = m_refData.m_referenceMesh.vertices();
            Transform M = m_refData.m_meshTransformInverse.inverse();
#pragma omp parallel for
            for ( int i = 0; i< vertices.size(); ++i )
            {
                vertices[i] = M * vertices[i];
            }
            m_refData.m_weightSTBS = computeSTBS_weights( vertices, m_refData.m_skeleton );
        }
    }
    }
}

void SkinningComponent::setSmartStretch( bool on ){
    m_smartStretch = on;
}

void SkinningComponent::showWeights( bool on ) {
    m_showingWeights   = on;
    auto ro            = getRoMgr()->getRenderObject( *m_renderObjectReader() );
    auto attrUV        = Data::Mesh::getAttribName( Data::Mesh::VERTEX_TEXCOORD );
    AttribHandle<Vector3> handle;

    AttribArrayGeometry* geom;
    if ( !m_meshIsPoly )
    {
        geom = const_cast<TriangleMesh*>( m_triMeshWriter() );
    }
    else {
        geom = const_cast<PolyMesh*>( m_polyMeshWriter() );
    }

    if ( m_showingWeights )
    {
        // update the displayed weights
        const auto size = m_frameData.m_currentPosition.size();
        m_weightsUV.resize( size, Vector3::Zero() );
        switch ( m_weightType ) {
        case STANDARD:
        {
    #pragma omp parallel for
            for ( int i = 0; i < int( size ); ++i )
            {
                m_weightsUV[i][0] = m_refData.m_weights.coeff( i, m_weightBone );
            }
        } break;
        case STBS:
        {
    #pragma omp parallel for
            for ( int i = 0; i < int( size ); ++i )
            {
                m_weightsUV[i][0] = m_refData.m_weightSTBS.coeff( i, m_weightBone );
            }
        } break;
        }
        // change the material
        ro->setMaterial( m_weightMaterial );
        ro->getRenderTechnique()->setParametersProvider( m_weightMaterial );
        // get the UV attrib handle, will create it if not there.
        handle = geom->addAttrib<Vector3>( attrUV );
        geom->getAttrib( handle ).setData( m_weightsUV );
    }
    else
    {
        // change the material
        ro->setMaterial( m_baseMaterial );
        ro->getRenderTechnique()->setParametersProvider( m_baseMaterial );
        // if the UV attrib existed before, reset it, otherwise remove it.
        handle = geom->getAttribHandle<Vector3>( attrUV );
        if ( m_baseUV.size() > 0 ) { geom->getAttrib( handle ).setData( m_baseUV ); }
        else
        { geom->removeAttrib( handle ); }
    }
    m_forceUpdate = true;
}

bool SkinningComponent::isShowingWeights() {
    return m_showingWeights;
}

void SkinningComponent::showWeightsType( WeightType type ) {
    m_weightType = type;
    if ( m_showingWeights )
    {
        showWeights( true );
    }
}

SkinningComponent::WeightType SkinningComponent::getWeightsType() {
    return m_weightType;
}

void SkinningComponent::setWeightBone( uint bone ) {
    m_weightBone = bone;
    if ( m_showingWeights )
    {
        showWeights( true );
    }
}

} // namespace Scene
} // namespace Engine
} // namespace Ra
