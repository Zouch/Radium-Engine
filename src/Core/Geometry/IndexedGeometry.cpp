#include <Core/Geometry/IndexedGeometry.hpp>
#include <iterator>

namespace Ra {
namespace Core {
namespace Geometry {

MultiIndexedGeometry::MultiIndexedGeometry( const MultiIndexedGeometry& other ) :
    AttribArrayGeometry( other ) {
    deepCopy( other );
}

MultiIndexedGeometry::MultiIndexedGeometry( MultiIndexedGeometry&& other ) :
    AttribArrayGeometry( std::move( other ) ), m_indices( std::move( other.m_indices ) ) {}

MultiIndexedGeometry::MultiIndexedGeometry( const AttribArrayGeometry& other ) :
    AttribArrayGeometry( other ) {}

MultiIndexedGeometry::MultiIndexedGeometry( AttribArrayGeometry&& other ) :
    AttribArrayGeometry( std::move( other ) ) {}

MultiIndexedGeometry& MultiIndexedGeometry::operator=( const MultiIndexedGeometry& other ) {
    invalidateAabb();
    AttribArrayGeometry::operator=( other );
    deepCopy( other );
    notify();
    return *this;
}

MultiIndexedGeometry& MultiIndexedGeometry::operator=( MultiIndexedGeometry&& other ) {
    invalidateAabb();
    AttribArrayGeometry::operator=( std::move( other ) );
    m_indices                    = std::move( other.m_indices );
    notify();
    return *this;
}

void MultiIndexedGeometry::clear() {
    invalidateAabb();
    AttribArrayGeometry::clear();
    deepClear();
    notify();
}

void MultiIndexedGeometry::copy( const MultiIndexedGeometry& other ) {
    invalidateAabb();
    AttribArrayGeometry::copyBaseGeometry( other );
    deepCopy( other );
    notify();
}

/// \todo Implement MultiIndexedGeometry::checkConsistency
void MultiIndexedGeometry::checkConsistency() const {
#ifdef CORE_DEBUG
#endif
}

bool MultiIndexedGeometry::append( const MultiIndexedGeometry& other ) {
    bool dataHasBeenCopied = false;
    for ( const auto& [key, value] : other.m_indices )
    {
        auto it = m_indices.find( key );
        if ( it == m_indices.end() ) // copy entire layer
        {
            m_indices[key]    = value;
            dataHasBeenCopied = true;
        }
        else
        {
            // try to append to an existing layer: should always work
            if ( it->second.second->append( *( value.second ) ) ) { dataHasBeenCopied = true; }
            else
            {
                CORE_ASSERT( false,
                             "Inconsistency: layers with different semantics shares the same key" );
            }
        }
    }

    if ( dataHasBeenCopied )
    {
        invalidateAabb();
        notify();
    }
    return true;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

bool MultiIndexedGeometry::containsLayer( const LayerSemantic& semanticName ) const {
    for ( const auto& [key, value] : m_indices )
    {
        if ( key.first.find( semanticName ) != key.first.end() ) return true;
    }
    return false;
}

bool MultiIndexedGeometry::containsLayer( const LayerSemanticCollection& semantics ) const {
    for ( const auto& [key, value] : m_indices )
    {
        if ( key.first == semantics ) return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

size_t MultiIndexedGeometry::countLayers( const LayerSemantic& semanticName ) const {
    size_t c = 0;
    for ( const auto& [key, value] : m_indices )
    {
        if ( key.first.find( semanticName ) != key.first.end() ) ++c;
    }
    return c;
}

size_t MultiIndexedGeometry::countLayers( const LayerSemanticCollection& semantics ) const {
    size_t c = 0;
    for ( const auto& [key, value] : m_indices )
    {
        if ( key.first == semantics ) ++c;
    }
    return c;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

std::pair<MultiIndexedGeometry::LayerKeyType, const GeometryIndexLayerBase&>
MultiIndexedGeometry::getFirstLayerOccurrence( const LayerSemantic& semanticName ) const {
    for ( const auto& [key, value] : m_indices )
    {
        if ( key.first.find( semanticName ) != key.first.end() ) return { key, *( value.second ) };
    }
    throw std::out_of_range( "Layer entry not found" );
}

std::pair<MultiIndexedGeometry::LayerKeyType, const GeometryIndexLayerBase&>
MultiIndexedGeometry::getFirstLayerOccurrence( const LayerSemanticCollection& semantics ) const {
    for ( const auto& [key, value] : m_indices )
    {
        if ( key.first == semantics ) return { key, *( value.second ) };
    }
    throw std::out_of_range( "Layer entry not found" );
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

std::pair<MultiIndexedGeometry::LayerKeyType, GeometryIndexLayerBase&>
MultiIndexedGeometry::getFirstLayerOccurrenceWithLock( const LayerSemantic& semanticName ) {
    for ( auto& [key, value] : m_indices )
    {
        if ( key.first.find( semanticName ) != key.first.end() )
        {
            CORE_ASSERT( !value.first, "try to get already locked layer" );
            value.first = true;
            return { key, *( value.second ) };
        }
    }
    throw std::out_of_range( "Layer entry not found" );
}

std::pair<MultiIndexedGeometry::LayerKeyType, GeometryIndexLayerBase&>
MultiIndexedGeometry::getFirstLayerOccurrenceWithLock( const LayerSemanticCollection& semantics ) {
    for ( auto& [key, value] : m_indices )
    {
        if ( key.first == semantics )
        {
            CORE_ASSERT( !value.first, "try to get already locked layer" );
            value.first = true;
            return { key, *( value.second ) };
        }
    }
    throw std::out_of_range( "Layer entry not found" );
}

GeometryIndexLayerBase& MultiIndexedGeometry::getLayerWithLock( const LayerKeyType& layerKey ) {
    auto& p = m_indices.at( layerKey );
    CORE_ASSERT( !p.first, "try to get already locked layer" );
    p.first = true;
    return *( p.second );
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void MultiIndexedGeometry::unlockFirstLayerOccurrence( const LayerSemantic& semanticName ) {
    for ( auto& [key, value] : m_indices )
    {
        if ( key.first.find( semanticName ) != key.first.end() )
        {
            CORE_ASSERT( value.first, "try to release unlocked layer" );
            value.first = false;
            notify();
            return;
        }
    }
    throw std::out_of_range( "Layer entry not found" );
}

void MultiIndexedGeometry::unlockFirstLayerOccurrence( const LayerSemanticCollection& semantics ) {
    for ( auto& [key, value] : m_indices )
    {
        if ( key.first == semantics )
        {
            CORE_ASSERT( value.first, "try to release unlocked layer" );
            value.first = false;
            notify();
            return;
        }
    }
    throw std::out_of_range( "Layer entry not found" );
}

void MultiIndexedGeometry::unlockLayer( const LayerKeyType& layerKey ) {
    auto& p = m_indices.at( layerKey );
    CORE_ASSERT( p.first, "try to release unlocked layer" );
    p.first = true;
    notify();
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

bool MultiIndexedGeometry::addLayer( std::unique_ptr<GeometryIndexLayerBase>&& layer,
                                     const std::string& layerName ) {
    LayerKeyType key { layer->semantics(), layerName };
    if ( m_indices.find( key ) != m_indices.end() ) return false;

    m_indices.insert( { key, std::make_pair( false, layer.release() ) } );

    notify();
    return true;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
void MultiIndexedGeometry::deepCopy( const MultiIndexedGeometry& other ) {
    m_indices = other.m_indices;
    for ( auto& el : m_indices )
        el.second.second = el.second.second->clone(); // replace copied entries by duplicates
}
void MultiIndexedGeometry::deepClear() {
    for ( auto& el : m_indices )
        delete el.second.second;
    m_indices.clear();
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

std::size_t MultiIndexedGeometry::KeyHash::operator()( const LayerKeyType& k ) const {
    // Mix semantic collection into a single identifier string
    std::ostringstream stream;
    std::copy( k.first.begin(), k.first.end(), std::ostream_iterator<std::string>( stream, "" ) );
    std::string result = stream.str();
    std::sort( result.begin(), result.end() );

    // Combine with layer name hash
    return std::hash<std::string> {}( result ) ^ ( std::hash<std::string> {}( k.second ) << 1 );
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void PointCloudIndexLayer::linearIndices( const AttribArrayGeometry& attr ) {
    auto nbVert = attr.vertices().size();
    collection().resize( nbVert );
    collection().getMap() = IndexContainerType::Matrix::LinSpaced( nbVert, 0, nbVert - 1 );
}

} // namespace Geometry
} // namespace Core
} // namespace Ra
