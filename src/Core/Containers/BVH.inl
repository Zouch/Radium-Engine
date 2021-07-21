#pragma once
#include <Core/Containers/BVH.hpp>

#include <iostream>

#include <Core/Utils/Color.hpp>

namespace Ra {
namespace Core {
namespace Containers {

template <typename T>
inline BVH<T>::Node::Node( const std::shared_ptr<T>& t ) : m_aabb( t->getAabb() ), m_data( t ) {}

template <typename T>
inline BVH<T>::Node::Node( const std::shared_ptr<Node>& l, const std::shared_ptr<Node>& r ) :
    m_aabb( l->m_aabb.merged( r->m_aabb ) ) {
    m_children.push_back( l );
    m_children.push_back( r );
}

template <typename T>
inline typename BVH<T>::NodePtr BVH<T>::Node::getLeftChild() const {
    if ( !isFinal() )
        return m_children[0];
    else
        return nullptr;
}

template <typename T>
inline typename BVH<T>::NodePtr BVH<T>::Node::getRightChild() const {
    if ( !isFinal() )
        return m_children[1];
    else
        return nullptr;
}

template <typename T>
inline BVH<T>::BVH() : m_root( nullptr ), m_root_aabb(), m_upToDate( true ) {}

template <typename T>
inline void BVH<T>::insertLeaf( const std::shared_ptr<T>& t ) {
    m_leaves.push_back( std::shared_ptr<Node>( new Node( t ) ) );
    m_root_aabb.extend( t->getAabb() );

    m_upToDate = false;
}

template <typename T>
inline void BVH<T>::clear() {
    // TODO Clear tree

    m_leaves.clear();
    m_root      = nullptr;
    m_root_aabb = Aabb();

    // m_upToDate = true ;
}

template <typename T>
inline void BVH<T>::update() {
    if ( !m_upToDate ) buildBottomUpSlow();
}

template <typename T>
inline void BVH<T>::buildBottomUpSlow() {
    /*if (m_root != nullptr)
        clear();*/

    // We start from known leaves, bottom up
    std::vector<NodePtr> toMerge( m_leaves );

    // As long as there are several leaves to merge
    while ( toMerge.size() > 2 ) {
        Scalar low = m_root_aabb.isEmpty() ? 0 : m_root_aabb.volume();
        typename std::vector<NodePtr>::iterator l_min, r_min;
        Aabb merge;

        for ( typename std::vector<NodePtr>::iterator it = toMerge.begin(); it != toMerge.end();
              ++it ) {
            for ( typename std::vector<NodePtr>::iterator it2 = ( it + 1 ); it2 != toMerge.end();
                  ++it2 ) {
                // Find the merger with smallest volume
                merge = it->get()->getAabb().merged( it2->get()->getAabb() );
                if ( merge.volume() < low ) {
                    low   = merge.volume();
                    l_min = it;
                    r_min = it2;
                }
            }
        }

        NodePtr l_node( *l_min ), r_node( *r_min );

        // Remove nodes going to be merged
        toMerge.erase( r_min );
        toMerge.erase( l_min );
        // Add the merger
        toMerge.push_back( std::shared_ptr<Node>( new Node( l_node, r_node ) ) );
    }

    // Final node (the root) is the merger of last two nodes
    m_root = std::shared_ptr<Node>( new Node( toMerge[0], toMerge[1] ) );

    m_upToDate = true;
}

template <typename T>
inline void BVH<T>::getInFrustumSlow( std::vector<std::shared_ptr<T>>& objects,
                                      const Geometry::Frustum& frustum ) const {

    if ( m_root ) {
        std::vector<NodePtr> toCheck;
        toCheck.push_back( m_root );

        while ( !toCheck.empty() ) {
            NodePtr current = toCheck.back();
            toCheck.pop_back();

            if ( current->isFinal() )
                objects.push_back( current->getData() );
            else {
                // If BBOX cuts the frustum, add child in toCheck list
                bool isIn = true;
                for ( uint i = 0; i < 6 && isIn; ++i ) {
                    const auto& plane = frustum.getPlane( i );
                    Aabb aabb         = current->getAabb();

                    // If the BBOX is fully outside (at least) one plane, it is not in
                    if ( ( plane.dot( aabb.corner( Aabb::BottomLeftFloor ).homogeneous() ) <
                           0.f ) &&
                         ( plane.dot( aabb.corner( Aabb::BottomRightFloor ).homogeneous() ) <
                           0.f ) &&
                         ( plane.dot( aabb.corner( Aabb::TopLeftFloor ).homogeneous() ) < 0.f ) &&
                         ( plane.dot( aabb.corner( Aabb::TopRightFloor ).homogeneous() ) < 0.f ) &&
                         ( plane.dot( aabb.corner( Aabb::BottomLeftCeil ).homogeneous() ) < 0.f ) &&
                         ( plane.dot( aabb.corner( Aabb::BottomRightCeil ).homogeneous() ) <
                           0.f ) &&
                         ( plane.dot( aabb.corner( Aabb::TopLeftCeil ).homogeneous() ) < 0.f ) &&
                         ( plane.dot( aabb.corner( Aabb::TopRightCeil ).homogeneous() ) < 0.f ) )
                        isIn = false;
                }

                if ( isIn ) {
                    toCheck.push_back( current->getLeftChild() );
                    toCheck.push_back( current->getRightChild() );
                }
            }
        }
    }
}
} // namespace Containers
} // namespace Core
} // namespace Ra
