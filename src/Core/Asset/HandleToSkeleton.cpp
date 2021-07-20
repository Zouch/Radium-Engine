#include <Core/Asset/HandleToSkeleton.hpp>

#include <set>

#include <Core/Animation/Skeleton.hpp>
#include <Core/Asset/HandleData.hpp>
#include <Core/Utils/Log.hpp>

using namespace Ra::Core::Utils;

namespace Ra {
namespace Core {
namespace Asset {

namespace {
// Recursive function to add bones
void addBone( const uint parent,                       // index of parent bone
              const uint dataID,                       // index in map
              const Ra::Core::Asset::HandleData& data, // handle data
              const Ra::Core::AlignedStdVector<Ra::Core::Vector2ui>& edgeList, // list of edges
              std::vector<bool>& processed,        // which ids have been processed
              Core::Animation::Skeleton& skelOut ) // correspondance between bone name and bone idx
{
    if ( !processed[dataID] )
    {
        processed[dataID] = true;
        const auto& dd    = data.getComponentData()[dataID];
        uint index        = skelOut.addBone(
            parent, dd.m_frame, Ra::Core::Animation::HandleArray::SpaceType::MODEL, dd.m_name );
        for ( const auto& edge : edgeList )
        {
            if ( edge[0] == dataID )
            { addBone( index, edge[1], data, edgeList, processed, skelOut ); }
        }
    }
}
// function to add bones from a given root
void addRoot( const uint dataID,                                               // index in map
              const Ra::Core::Asset::HandleData& data,                         // handle data
              const Ra::Core::AlignedStdVector<Ra::Core::Vector2ui>& edgeList, // list of edges
              std::vector<bool>& processed,        // which ids have been processed
              Core::Animation::Skeleton& skelOut ) // correspondance between bone name and bone idx
{
    if ( !processed[dataID] )
    {
        processed[dataID] = true;
        const auto& dd    = data.getComponentData()[dataID];
        uint index        = skelOut.addRoot( dd.m_frame, dd.m_name );
        for ( const auto& edge : edgeList )
        {
            if ( edge[0] == dataID )
            { addBone( index, edge[1], data, edgeList, processed, skelOut ); }
        }
    }
}
} // namespace

void createSkeleton( const Ra::Core::Asset::HandleData& data, Core::Animation::Skeleton& skelOut ) {
    const uint size = data.getComponentDataSize();
    auto component  = data.getComponentData();

    std::set<uint> root;
    for ( uint i = 0; i < size; ++i )
    { root.insert( i ); }
    std::set<uint> leaves = root;

    auto edgeList = data.getEdgeData();
    for ( const auto& edge : edgeList )
    {
        root.erase( edge[1] );
        leaves.erase( edge[0] );
    }

    std::vector<bool> processed( size, false );
    for ( const auto& r : root )
    { addRoot( r, data, edgeList, processed, skelOut ); }

    if ( data.needsEndNodes() )
    {
        std::map<std::string, uint> boneNameMap;
        for ( uint i = 0; i < skelOut.size(); ++i )
        { boneNameMap[skelOut.getLabel( i )] = i; }
        for ( const auto& l : leaves )
        {
            const auto& dd = component[l];
            if ( dd.m_weights.size() )
            {
                LOG( logDEBUG ) << "Adding end-bone at " << dd.m_name << ".";
                skelOut.addBone( boneNameMap[dd.m_name],
                                 data.getFrame().inverse() * dd.m_frame,
                                 Ra::Core::Animation::HandleArray::SpaceType::MODEL,
                                 dd.m_name + "_Ra_endBone" );
            }
        }
    }
}
} // namespace Asset
} // namespace Core
} // namespace Ra
