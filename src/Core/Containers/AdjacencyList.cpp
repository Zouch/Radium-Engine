#include <Core/Containers/AdjacencyList.hpp>

#include <fstream>

namespace Ra {
namespace Core {

/// CONSTRUCTOR
AdjacencyList::AdjacencyList() : m_child(), m_parent() {}
AdjacencyList::AdjacencyList( const uint n ) : m_child( n ), m_parent( n, -1 ) {}
AdjacencyList::AdjacencyList( const AdjacencyList& adj ) :
    m_child( adj.m_child ), m_parent( adj.m_parent ) {}

/// DESTRUCTOR
AdjacencyList::~AdjacencyList() {}

/// NODE
uint AdjacencyList::addRoot() {
    uint idx = size();
    m_child.push_back( ChildrenList() );
    m_parent.push_back( -1 );
    m_level.push_back( 0 );
    return idx;
}

uint AdjacencyList::addNode( const uint parent ) {
    CORE_ASSERT( ( parent < size() ), "Index parent out of bounds" );
    uint idx = size();
    m_child.push_back( ChildrenList() );
    m_parent.push_back( parent );
    m_level.push_back( m_level[parent] + 1 );
    m_child[parent].push_back( idx );
    return idx;
}

void AdjacencyList::pruneLeaves( std::vector<uint>& pruned, std::vector<bool>& delete_flag ) {
    pruned.clear();
    delete_flag.clear();
    delete_flag.resize( this->size(), false );
    std::vector<bool> prune_flag = delete_flag;
    for ( uint i = 0; i < this->size(); ++i ) {
        if ( this->isLeaf( i ) && !this->isRoot( i ) ) {
            delete_flag[i] = true;
            prune_flag[i]  = true;
        }
        else {
            pruned.push_back( i );
        }
    }

    for ( uint j = this->size(); j > 0; --j ) {
        const uint i = j - 1;
        if ( prune_flag[i] ) {
            this->m_parent.erase( this->m_parent.begin() + i );
            this->m_child.erase( this->m_child.begin() + i );
            prune_flag.erase( prune_flag.begin() + i );
            ++j;
        }
    }

    for ( uint i = 0; i < this->size(); ++i ) {
        this->m_parent[i] =
            ( ( this->m_parent[i] == -1 ) || ( delete_flag[i] ) ) ? -1 : pruned[this->m_parent[i]];
        for ( auto it = this->m_child[i].begin(); it != this->m_child[i].end(); ++it ) {
            if ( delete_flag[( *it )] ) {
                this->m_child[i].erase( it );
                --it;
            }
            else {
                *it = pruned[*it];
            }
        }
    }
}

void AdjacencyList::pruneLeaves() {
    std::vector<uint> p;
    std::vector<bool> d;
    this->pruneLeaves( p, d );
}

VectorArray<Eigen::Matrix<uint, 2, 1>> AdjacencyList::extractEdgeList( bool include_leaf ) const {
    using Edge = Eigen::Matrix<uint, 2, 1>;
    VectorArray<Edge> edgeList;

    for ( uint i = 0; i < m_child.size(); ++i ) {
        if ( include_leaf && isLeaf( i ) ) {
            Edge e;
            e( 0 ) = i;
            e( 1 ) = i;
            edgeList.push_back( e );
        }
        else {
            for ( const auto& edge : m_child[i] ) {
                Edge e;
                e( 0 ) = i;
                e( 1 ) = edge;
                edgeList.push_back( e );
            }
        }
    }
    return edgeList;
}

AdjacencyList::ConsistencyStatus AdjacencyList::computeConsistencyStatus() const {
    // Note: we abort at the first error found to keep the error backtrace
    if ( m_parent.size() != m_child.size() ) {
        return ConsistencyStatus::IncompatibleChildrenAndParentList;
    }

    for ( uint node = 0; node < size(); ++node ) {
        if ( m_parent.at( node ) >= int( node ) ) { return ConsistencyStatus::WrongParentOrdering; }
        if ( m_parent.at( node ) < -1 ) { return ConsistencyStatus::WrongParentIndex; }

        for ( const auto& child : m_child.at( node ) ) {
            if ( m_parent.at( child ) != int( node ) ) {
                return ConsistencyStatus::InconsistentParentIndex;
            }
        }

        if ( isLeaf( node ) != ( m_child.at( node ).size() == 0 ) ) {
            return ConsistencyStatus::NonLeafNodeWithoutChild;
        }
    }
    return ConsistencyStatus::Valid;
}

std::ofstream& operator<<( std::ofstream& ofs, const AdjacencyList& adj ) {
    const std::string header { "ADJACENCYLIST\n" };
    const std::string comment { "#ID PARENT nCHILDREN CHILDREN\n" };
    const uint size = adj.size();

    ofs << header + comment + std::to_string( size ) + "\n";
    for ( uint i = 0; i < size; ++i ) {
        uint c;
        ofs << std::to_string( i ) + " " + std::to_string( adj.parents()[i] ) + " " +
                   std::to_string( c = adj.children()[i].size() );
        for ( uint j = 0; j < c; ++j ) {
            ofs << " " + std::to_string( adj.children()[i][j] );
        }
        ofs << "\n";
    }
    return ofs;
}

} // namespace Core
} // namespace Ra
