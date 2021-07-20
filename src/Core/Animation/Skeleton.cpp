#include <Core/Animation/Skeleton.hpp>
#include <Core/Math/LinearAlgebra.hpp> // Math::clamp

#include <algorithm>
#include <stack>

namespace Ra {
namespace Core {
namespace Animation {

/// CONSTRUCTOR
Skeleton::Skeleton() : HandleArray(), m_graph(), m_modelSpace() {}

Skeleton::Skeleton( const uint n ) : HandleArray( n ), m_graph( n ), m_modelSpace( n ) {}

void Skeleton::clear() {
    m_pose.clear();
    m_graph.clear();
    m_modelSpace.clear();
}

const Pose& Skeleton::getPose( const SpaceType MODE ) const {
    static_assert( std::is_same<bool, typename std::underlying_type<SpaceType>::type>::value,
                   "SpaceType is not a boolean" );
    if ( MODE == SpaceType::LOCAL ) return m_pose;
    return m_modelSpace;
}

void Skeleton::setPose( const Pose& pose, const SpaceType MODE ) {
    CORE_ASSERT( ( size() == pose.size() ), "Size mismatching" );
    static_assert( std::is_same<bool, typename std::underlying_type<SpaceType>::type>::value,
                   "SpaceType is not a boolean" );
    if ( MODE == SpaceType::LOCAL ) {
        m_pose = pose;
        m_modelSpace.resize( m_pose.size() );
        for ( uint i = 0; i < m_graph.size(); ++i ) {
            if ( m_graph.isRoot( i ) ) { m_modelSpace[i] = m_pose[i]; }
            for ( const auto& child : m_graph.children()[i] ) {
                m_modelSpace[child] = m_modelSpace[i] * m_pose[child];
            }
        }
    }
    else {
        m_modelSpace = pose;
        m_pose.resize( m_modelSpace.size() );
        for ( uint i = 0; i < m_graph.size(); ++i ) {
            if ( m_graph.isRoot( i ) ) { m_pose[i] = m_modelSpace[i]; }
            for ( const auto& child : m_graph.children()[i] ) {
                m_pose[child] = m_modelSpace[i].inverse() * m_modelSpace[child];
            }
        }
    }
}

const Transform& Skeleton::getTransform( const uint i, const SpaceType MODE ) const {
    CORE_ASSERT( ( i < size() ), "Index i out of bounds" );
    static_assert( std::is_same<bool, typename std::underlying_type<SpaceType>::type>::value,
                   "SpaceType is not a boolean" );
    if ( MODE == SpaceType::LOCAL ) return m_pose[i];
    return m_modelSpace[i];
}

void Skeleton::setTransform( const uint i, const Transform& T, const SpaceType MODE ) {
    CORE_ASSERT( ( i < size() ), "Index i out of bounds" );
    static_assert( std::is_same<bool, typename std::underlying_type<SpaceType>::type>::value,
                   "SpaceType is not a boolean" );

    switch ( m_manipulation ) {
    case FORWARD: {
        // just set the transfrom
        if ( MODE == SpaceType::LOCAL )
            setLocalTransform( i, T );
        else
            setModelTransform( i, T );
    } break;
    case PSEUDO_IK: {
        Transform modelT = T;
        if ( MODE == SpaceType::LOCAL ) modelT = ( m_modelSpace[i] * m_pose[i].inverse() * T );
        // turn bone translation into rotation for parent
        const int pBoneIdx = m_graph.parents()[i];
        if ( pBoneIdx != -1 && m_graph.children()[pBoneIdx].size() == 1 ) {
            const auto& pTBoneModel = m_modelSpace[pBoneIdx];

            Ra::Core::Vector3 A;
            Ra::Core::Vector3 B;
            getBonePoints( pBoneIdx, A, B );
            Ra::Core::Vector3 B_;
            B_     = modelT.translation();
            auto q = Ra::Core::Quaternion::FromTwoVectors( ( B - A ), ( B_ - A ) );
            Ra::Core::Transform R( q );
            R.pretranslate( A );
            R.translate( -A );
            setModelTransform( pBoneIdx, R * pTBoneModel );
        }
        // update bone transform and also children's transform
        setLocalTransform( i, m_pose[i] * m_modelSpace[i].inverse() * modelT );
    } break;
    }
}

void Skeleton::setLocalTransform( const uint i, const Transform& T ) {
    m_pose[i] = T;
    // Compute the model space pose
    if ( m_graph.isRoot( i ) ) { m_modelSpace[i] = m_pose[i]; }
    else {
        m_modelSpace[i] = m_modelSpace[m_graph.parents()[i]] * T;
    }
    if ( !m_graph.isLeaf( i ) ) {
        std::stack<uint> stack;
        stack.push( i );
        while ( !stack.empty() ) {
            uint parent = stack.top();
            stack.pop();
            for ( const auto& child : m_graph.children()[parent] ) {
                m_modelSpace[child] = m_modelSpace[parent] * m_pose[child];
                stack.push( child );
            }
        }
    }
}

void Skeleton::setModelTransform( const uint i, const Transform& T ) {
    m_modelSpace[i] = T;
    // Compute the local space pose
    if ( m_graph.isRoot( i ) ) { m_pose[i] = m_modelSpace[i]; }
    else {
        m_pose[i] = m_modelSpace[m_graph.parents()[i]].inverse() * T;
    }
    if ( !m_graph.isLeaf( i ) ) {
        std::stack<uint> stack;
        stack.push( i );
        while ( !stack.empty() ) {
            uint parent = stack.top();
            stack.pop();
            for ( const auto& child : m_graph.children()[parent] ) {
                m_pose[child] = m_modelSpace[parent].inverse() * m_modelSpace[child];
                stack.push( child );
            }
        }
    }
}

uint Skeleton::addRoot( const Transform& T, const Label label ) {
    m_pose.push_back( T );
    m_modelSpace.push_back( T );
    m_label.push_back( label );
    return m_graph.addRoot();
}

uint Skeleton::addBone( const uint parent,
                        const Transform& T,
                        const SpaceType MODE,
                        const Label label ) {
    static_assert( std::is_same<bool, typename std::underlying_type<SpaceType>::type>::value,
                   "SpaceType is not a boolean" );
    if ( MODE == SpaceType::LOCAL ) {
        m_pose.push_back( T );
        m_modelSpace.push_back( T * m_modelSpace[parent] );
    }
    else {
        m_modelSpace.push_back( T );
        m_pose.push_back( m_modelSpace[parent].inverse() * T );
    }
    m_label.push_back( label );
    return m_graph.addNode( parent );
}

void Skeleton::getBonePoints( const uint i, Vector3& startOut, Vector3& endOut ) const {
    // Check bone index is valid
    CORE_ASSERT( i < m_modelSpace.size(), "invalid bone index" );

    startOut = m_modelSpace[i].translation();
    // A leaf bone has length 0
    if ( m_graph.isLeaf( i ) ) { endOut = startOut; }
    else {
        const auto& children = m_graph.children()[i];
        CORE_ASSERT( children.size() > 0, "non-leaf bone has no children." );
        // End point is the average of chidren start points.
        endOut = Vector3::Zero();
        for ( auto child : children ) {
            endOut += m_modelSpace[child].translation();
        }
        endOut *= ( 1_ra / children.size() );
    }
}

Vector3 Skeleton::projectOnBone( uint boneIdx, const Ra::Core::Vector3& pos ) const {
    Vector3 start, end;
    getBonePoints( boneIdx, start, end );

    auto op  = pos - start;
    auto dir = ( end - start );
    // Square length of bone
    const Scalar length_sq = dir.squaredNorm();
    CORE_ASSERT( length_sq != 0.f, "bone has lenght 0, cannot project." );

    // Project on the line segment
    const Scalar t = std::clamp( op.dot( dir ) / length_sq, Scalar( 0 ), Scalar( 1 ) );
    return start + ( t * dir );
}

std::ostream& operator<<( std::ostream& os, const Skeleton& skeleton ) {
    for ( uint i = 0; i < skeleton.size(); ++i ) {
        const uint id          = i;
        const std::string name = skeleton.getLabel( i );
        const std::string type = skeleton.m_graph.isRoot( i )     ? "ROOT"
                                 : skeleton.m_graph.isJoint( i )  ? "JOINT"
                                 : skeleton.m_graph.isBranch( i ) ? "BRANCH"
                                 : skeleton.m_graph.isLeaf( i )   ? "LEAF"
                                                                  : "UNKNOWN";
        const int pid          = skeleton.m_graph.parents()[i];
        const std::string pname =
            ( pid == -1 ) ? "" : ( "(" + std::to_string( pid ) + ") " + skeleton.getLabel( pid ) );

        const auto& children = skeleton.m_graph.children()[i];

        os << "Bone " << id << "\t: " << name << std::endl;
        os << "Type\t: " << type << std::endl;
        os << "Parent\t: " << pname << std::endl;
        os << "Children#\t: " << children.size() << std::endl;
        os << "Children\t: ";
        for ( const auto& cid : children ) {
            const std::string cname = skeleton.getLabel( cid );
            os << "(" << cid << ") " << cname << " | ";
        }

        os << " " << std::endl;
        os << " " << std::endl;
    }
    return os;
}

} // namespace Animation
} // namespace Core
} // namespace Ra
