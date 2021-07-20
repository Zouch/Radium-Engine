#include <IO/AssimpLoader/AssimpHandleDataLoader.hpp>

#include <assimp/mesh.h>
#include <assimp/scene.h>
#include <queue>
#include <set>

#include <Core/Asset/HandleData.hpp>
#include <Core/Utils/Log.hpp>

#include <IO/AssimpLoader/AssimpWrapper.hpp>

namespace Ra {
namespace IO {

using namespace Core::Utils; // log
using namespace Core::Asset;

/// CONSTRUCTOR
AssimpHandleDataLoader::AssimpHandleDataLoader( const bool VERBOSE_MODE ) :
    DataLoader<HandleData>( VERBOSE_MODE ) {}

/// DESTRUCTOR
AssimpHandleDataLoader::~AssimpHandleDataLoader() = default;

/// LOAD

void AssimpHandleDataLoader::loadData( const aiScene* scene,
                                       std::vector<std::unique_ptr<HandleData>>& data ) {
    data.clear();

    if ( scene == nullptr )
    {
        LOG( logDEBUG ) << "AssimpHandleDataLoader : scene is nullptr.";
        return;
    }

    if ( m_verbose )
    {
        LOG( logDEBUG ) << "File contains handle.";
        LOG( logDEBUG ) << "Handle Loading begin...";
    }

    loadHandleData( scene, data );

    if ( m_verbose ) { LOG( logDEBUG ) << "Handle Loading end.\n"; }
}

/// Helper functions

namespace {

aiNode* findMeshNode( aiNode* node, const aiScene* scene, const aiString& meshName ) {
    // look inside the node
    for ( uint i = 0; i < node->mNumMeshes; ++i )
    {
        if ( scene->mMeshes[node->mMeshes[i]]->mName == meshName ) { return node; }
    }
    // repeat on children
    for ( uint i = 0; i < node->mNumChildren; ++i )
    {
        aiNode* n = findMeshNode( node->mChildren[i], scene, meshName );
        if ( n != nullptr ) { return n; }
    }
    return nullptr;
}

void initMarks( const aiNode* node, std::map<std::string, bool>& flags, bool flag = false ) {
    flags[assimpToCore( node->mName )] = flag;
    for ( uint i = 0; i < node->mNumChildren; ++i )
    { initMarks( node->mChildren[i], flags, flag ); }
}

void markParents( aiNode* node,
                  const aiScene* scene,
                  const std::vector<aiNode*>& meshParents,
                  std::map<std::string, bool>& flag ) {
    const std::string nodeName = assimpToCore( node->mName );
    if ( flag[nodeName] )
    {
        // skip already visited up-tree.
        return;
    }
    flag[nodeName] = true;
    // check if node is in the mesh hierarchy
    auto it = std::find_if(
        meshParents.begin(), meshParents.end(), [node]( const auto& n ) { return n == node; } );
    if ( it != meshParents.end() )
    {
        flag[nodeName] = false;
        return;
    }
    // otherwise mark parents
    if ( node->mParent != nullptr ) { markParents( node->mParent, scene, meshParents, flag ); }
}

} // namespace

/// LOAD
void AssimpHandleDataLoader::loadHandleData(
    const aiScene* scene,
    std::vector<std::unique_ptr<HandleData>>& data ) const {
    // list mesh names according to GeometryLoader naming
    std::set<std::string> meshNames;

    // initialize need flag on all scene nodes
    std::map<std::string, bool> needNode;
    initMarks( scene->mRootNode, needNode );

    std::vector<std::vector<aiNode*>> meshParents( scene->mNumMeshes );

    // load the HandleComponentData for all meshes
    std::map<std::string, HandleComponentData> mapBone2Data;
    for ( uint n = 0; n < scene->mNumMeshes; ++n )
    {
        const aiMesh* mesh = scene->mMeshes[n];
        // fetch mesh name as registered by the GeometryLoader
        std::string meshName = assimpToCore( mesh->mName );
        while ( meshNames.find( meshName ) != meshNames.end() )
        { meshName.append( "_" ); }
        meshNames.insert( meshName );

        // if bound to no skeleton, skip
        if ( !mesh->HasBones() ) { continue; }

        // get mesh node parents
        aiNode* meshNode = findMeshNode( scene->mRootNode, scene, mesh->mName );
        while ( meshNode != nullptr )
        {
            meshParents[n].push_back( meshNode );
            meshNode = meshNode->mParent;
        }

        // deal with skeleton
        for ( uint j = 0; j < mesh->mNumBones; ++j )
        {
            const aiBone* bone = mesh->mBones[j];
            // fetch bone data
            const std::string boneName = assimpToCore( bone->mName );
            // if data doesn't exist yet, create it
            mapBone2Data[boneName].m_name = boneName;
            // fill skinning weights and offset matrix for this mesh
            loadHandleComponentDataWeights( bone, meshName, mapBone2Data[boneName] );
            // deal with hierarchy
            aiNode* node = scene->mRootNode->FindNode( bone->mName );
            if ( node == nullptr ) { continue; }
            // mark parents as needed up to mesh node relative
            markParents( node, scene, meshParents[n], needNode );
            // check children since end bones may not have weights
            if ( node->mNumChildren == 1 )
            {
                const aiNode* child         = node->mChildren[0];
                const std::string childName = assimpToCore( child->mName );
                // mark as needed
                needNode[childName] = true;
                // if data doesn't exist yet, create it
                mapBone2Data[childName].m_name = childName;
            }
        }
    }

    // also add bone nodes for skeletons not attached to a mesh
    auto rootNode = scene->mRootNode;
    for ( uint i = 0; i < rootNode->mNumChildren; ++i )
    {
        auto node = rootNode->mChildren[i];
        if ( needNode[assimpToCore( node->mName )] ) { continue; }
        // check not up from a mesh
        auto res =
            std::find_if( meshParents.begin(), meshParents.end(), [node]( const auto& nodes ) {
                return std::find_if( nodes.begin(), nodes.end(), [node]( const auto& n ) {
                           return n == node;
                       } ) != nodes.end();
            } );
        if ( res != meshParents.end() ) { continue; }
        // mark subtree
        initMarks( node, needNode, true );
    }

    // load hierarchy for needed bones
    std::vector<std::pair<std::string, std::string>> edgeList;
    for ( const auto& n : needNode )
    {
        if ( !n.second ) { continue; }
        // if data doesn't exist, create it (bone with no weight)
        mapBone2Data[n.first].m_name = n.first;
        // link to children
        const aiNode* node = scene->mRootNode->FindNode( aiString( n.first ) );
        for ( uint j = 0; j < node->mNumChildren; ++j )
        {
            const std::string childName = assimpToCore( node->mChildren[j]->mName );
            if ( needNode.at( childName ) )
            {
                // if data doesn't exist, create it (bone with no weight)
                mapBone2Data[childName].m_name = childName;
                // register parenthood
                edgeList.push_back( { n.first, childName } );
            }
        }
    }

    // load bone frame once all are registered
    for ( auto& bone : mapBone2Data )
    { loadHandleComponentDataFrame( scene, aiString( bone.first ), bone.second ); }

    // find roots and leaves
    std::set<std::string> roots;
    for ( const auto& node : needNode )
    {
        if ( node.second ) { roots.insert( node.first ); }
    }
    std::set<std::string> leaves = roots;
    for ( auto edge : edgeList )
    {
        roots.erase( edge.second );
        leaves.erase( edge.first );
    }

    // build one HandleData per root
    for ( auto root : roots )
    {
        HandleData* handle = new HandleData();
        handle->setType( HandleData::SKELETON );
        handle->setName( root );

        Ra::Core::Transform frame = Ra::Core::Transform::Identity();
        aiNode* node              = scene->mRootNode->FindNode( aiString( root ) );
        while ( node->mParent != nullptr )
        {
            node  = node->mParent;
            frame = assimpToCore( node->mTransformation ) * frame;
        }
        handle->setFrame( frame );

        // get list of bones and edges for this skeleton
        std::map<std::string, uint> nameTable;
        fillHandleData( root, edgeList, mapBone2Data, nameTable, handle );
        handle->setNameTable( nameTable );

        // check if need additional end bones
        bool needEndBone = false;
        for ( const std::string& leaf : leaves )
        {
            if ( nameTable.find( leaf ) != nameTable.end() )
            {
                const auto& handleComponentData = mapBone2Data[leaf];
                for ( const auto& mesh : handleComponentData.m_weights )
                {
                    if ( mesh.second.size() != 0 )
                    {
                        needEndBone = true;
                        break;
                    }
                }
                if ( needEndBone ) { break; }
            }
        }
        handle->needEndNodes( needEndBone );

        if ( handle->isSkeleton() && !handle->hasEdges() )
        {
            // Do not load empty skeleton
            delete handle;
        }
        else
        {
            // register the HandleData
            data.emplace_back( handle );
            if ( m_verbose ) { handle->displayInfo(); }
        }
    }
}

void AssimpHandleDataLoader::loadHandleComponentDataFrame( const aiScene* scene,
                                                           const aiString& boneName,
                                                           HandleComponentData& data ) const {
    // fetch global transform
    data.m_frame.setIdentity();
    aiNode* node = scene->mRootNode->FindNode( boneName );
    while ( node != nullptr )
    {
        data.m_frame = assimpToCore( node->mTransformation ) * data.m_frame;
        node         = node->mParent;
    }
}

void AssimpHandleDataLoader::loadHandleComponentDataWeights( const aiBone* bone,
                                                             const std::string& meshName,
                                                             HandleComponentData& data ) const {
    // fetch skinning weigthts
    for ( uint j = 0; j < bone->mNumWeights; ++j )
    {
        std::pair<uint, Scalar> weight( bone->mWeights[j].mVertexId, bone->mWeights[j].mWeight );
        data.m_weights[meshName].push_back( weight );
    }
    data.m_bindMatrices[meshName] = assimpToCore( bone->mOffsetMatrix );
}

void AssimpHandleDataLoader::fillHandleData(
    const std::string& node,
    const std::vector<std::pair<std::string, std::string>>& edgeList,
    const std::map<std::string, HandleComponentData>& mapBone2Data,
    std::map<std::string, uint>& nameTable,
    HandleData* data ) const {
    // register the HandleComponentData for the bone
    nameTable[node] = uint( data->getComponentData().size() );
    data->getComponentData().push_back( mapBone2Data.at( node ) );
    // bind meshes bound to the bone
    for ( const auto& mesh : mapBone2Data.at( node ).m_weights )
    { data->addBindMesh( mesh.first ); }
    // go through children
    for ( const auto& edge : edgeList )
    {
        if ( edge.first == node )
        {
            fillHandleData( edge.second, edgeList, mapBone2Data, nameTable, data );
            data->getEdgeData().push_back(
                { nameTable.at( edge.first ), nameTable.at( edge.second ) } );
        }
    }
}

} // namespace IO
} // namespace Ra
