#pragma once
namespace Ra {
namespace Core {
namespace Utils {

AttribBase::AttribBase( const std::string& name ) : m_name { name } {}

std::string AttribBase::getName() const {
    return m_name;
}

void AttribBase::setName( const std::string& name ) {
    m_name = name;
}

bool inline AttribBase::operator==( const AttribBase& rhs ) {
    return m_name == rhs.getName();
}

template <typename T>
Attrib<T>& AttribBase::cast() {
    return static_cast<Attrib<T>&>( *this );
}

template <typename T>
const Attrib<T>& AttribBase::cast() const {
    return static_cast<const Attrib<T>&>( *this );
}

bool AttribBase::isLocked() const {
    return m_isLocked;
}

void AttribBase::unlock() {
    lock( false );
}

void AttribBase::lock( bool isLocked ) {
    CORE_ASSERT( isLocked != m_isLocked, "double (un)lock" );
    m_isLocked = isLocked;
    if ( !m_isLocked ) notify();
}

/////////////// Attrib ///////////////////

template <typename T>
Attrib<T>::Attrib( const std::string& name ) : AttribBase( name ) {}

template <typename T>

Attrib<T>::~Attrib() {
    m_data.clear();
}
template <typename T>
void Attrib<T>::resize( size_t s ) {
    m_data.resize( s );
}
template <typename T>
typename Attrib<T>::Container& Attrib<T>::getDataWithLock() {
    lock();
    return m_data;
}

template <typename T>
const void* Attrib<T>::dataPtr() const {
    return m_data.data();
}

template <typename T>
void Attrib<T>::setData( const Container& data ) {
    CORE_ASSERT( !isLocked(), "try to set onto locked data" );
    m_data = data;
    notify();
}

template <typename T>
void Attrib<T>::setData( Container&& data ) {
    CORE_ASSERT( !isLocked(), "try to set onto locked data" );
    m_data = std::move( data );
    notify();
}

template <typename T>
const typename Attrib<T>::Container& Attrib<T>::data() const {
    return m_data;
}

template <typename T>
size_t Attrib<T>::getSize() const {
    return m_data.size();
}

template <typename T>
int Attrib<T>::getStride() const {
    return sizeof( value_type );
}

template <typename T>
size_t Attrib<T>::getBufferSize() const {
    return m_data.size() * sizeof( value_type );
}

template <typename T>
bool Attrib<T>::isFloat() const {
    return std::is_same<Scalar, T>::value;
}

template <typename T>
bool Attrib<T>::isVector2() const {
    return std::is_same<Eigen::Matrix<Scalar, 2, 1>, T>::value;
}

template <typename T>
bool Attrib<T>::isVector3() const {
    return std::is_same<Eigen::Matrix<Scalar, 3, 1>, T>::value;
}

template <typename T>
bool Attrib<T>::isVector4() const {
    return std::is_same<Eigen::Matrix<Scalar, 4, 1>, T>::value;
}

template <typename T>
template <typename U>
bool Attrib<T>::isType() {
    return std::is_same<U, T>::value;
}

// fully specialization defined in .cpp def for both float and double.
// Specialize for scalar only might raise error if
// user wants to have a specific attrib precision, while Radium Scalar is the other precision.
// Do we need to specialize for all floating point types ?
template <>
RA_CORE_API size_t Attrib<float>::getElementSize() const;
template <>
RA_CORE_API size_t Attrib<double>::getElementSize() const;
// template specialization defined in header.
template <typename T>
size_t Attrib<T>::getElementSize() const {
    return Attrib<T>::Container::Vector::RowsAtCompileTime;
}

/////////////////// AttribManager ///////////////////

AttribManager::AttribManager() {}

AttribManager::AttribManager( AttribManager&& m ) :
    m_attribs( std::move( m.m_attribs ) ),
    m_attribsIndex( std::move( m.m_attribsIndex ) ),
    m_numAttribs( std::move( m.m_numAttribs ) ) {}

AttribManager& AttribManager::operator=( AttribManager&& m ) {
    m_attribs      = std::move( m.m_attribs );
    m_attribsIndex = std::move( m.m_attribsIndex );
    m_numAttribs   = std::move( m.m_numAttribs );
    return *this;
}

void AttribManager::copyAttributes( const AttribManager& m ) {
    m_numAttribs = m.m_numAttribs;
}

template <class T, class... Handle>
void AttribManager::copyAttributes( const AttribManager& m,
                                    const AttribHandle<T>& attr,
                                    Handle... attribs ) {
    if ( m.isValid( attr ) ) {
        // get attrib to copy
        auto& a = m.getAttrib( attr );
        // add new attrib
        auto h = addAttrib<T>( a.getName() );
        // copy attrib data
        getAttrib( h ).setData( a.data() );
    }
    // deal with other attribs
    copyAttributes( m, attribs... );
}

template <typename T>
bool AttribManager::isValid( const AttribHandle<T>& h ) const {
    auto itr = m_attribsIndex.find( h.attribName() );
    return h.m_idx != Index::Invalid() && itr != m_attribsIndex.end() && itr->second == h.m_idx;
}

inline bool AttribManager::contains( const std::string& name ) const {
    return m_attribsIndex.find( name ) != m_attribsIndex.end();
}

template <typename T>
inline AttribHandle<T> AttribManager::findAttrib( const std::string& name ) const {
    auto c = m_attribsIndex.find( name );
    AttribHandle<T> handle;
    if ( c != m_attribsIndex.end() ) {
        handle.m_idx  = c->second;
        handle.m_name = c->first;
    }
    return handle;
}

template <typename T>
inline Attrib<T>& AttribManager::getAttrib( const AttribHandle<T>& h ) {
    return *static_cast<Attrib<T>*>( m_attribs.at( h.m_idx ).get() );
}

template <typename T>
inline Attrib<T>* AttribManager::getAttribPtr( const AttribHandle<T>& h ) {
    return static_cast<Attrib<T>*>( m_attribs.at( h.m_idx ).get() );
}

template <typename T>
inline void AttribManager::setAttrib( const AttribHandle<T>& h,
                                      const typename AttribHandle<T>::Container& data ) {
    static_cast<Attrib<T>*>( m_attribs.at( h.m_idx ).get() )->setData( data );
}

template <typename T>
inline void AttribManager::setAttrib( const AttribHandle<T>& h,
                                      typename AttribHandle<T>::Container&& data ) {
    static_cast<Attrib<T>*>( m_attribs.at( h.m_idx ).get() )->setData( data );
}

template <typename T>
inline const Attrib<T>& AttribManager::getAttrib( const AttribHandle<T>& h ) const {
    return *static_cast<Attrib<T>*>( m_attribs.at( h.m_idx ).get() );
}

AttribBase* AttribManager::getAttribBase( const std::string& name ) {
    auto c = m_attribsIndex.find( name );
    if ( c != m_attribsIndex.end() ) return m_attribs[c->second].get();

    return nullptr;
}

AttribBase* AttribManager::getAttribBase( const Index& idx ) {

    if ( idx.isValid() ) return m_attribs[idx].get();

    return nullptr;
}

template <typename T>
AttribHandle<T> AttribManager::addAttrib( const std::string& name ) {
    // does the attrib already exist?
    AttribHandle<T> h = findAttrib<T>( name );
    if ( isValid( h ) ) return h;

    // create the attrib
    smart_pointer_type attrib = std::make_unique<Attrib<T>>( name );

    // look for a free slot
    auto it = std::find_if(
        m_attribs.begin(), m_attribs.end(), []( const auto& attr ) { return !attr; } );
    if ( it != m_attribs.end() ) {
        it->swap( attrib );
        h.m_idx = std::distance( m_attribs.begin(), it );
    }
    else {
        m_attribs.push_back( std::move( attrib ) );
        h.m_idx = m_attribs.size() - 1;
    }
    m_attribsIndex[name] = h.m_idx;
    h.m_name             = name;
    ++m_numAttribs;

    notify( name );
    return h;
}

template <typename T>
void AttribManager::removeAttrib( AttribHandle<T>& h ) {
    auto c = m_attribsIndex.find( h.m_name );
    if ( c != m_attribsIndex.end() ) {
        Index idx = c->second;
        m_attribs[idx].reset( nullptr );
        m_attribsIndex.erase( c );
    }
    h.m_idx   = Index::Invalid(); // invalidate whatever!
    auto name = h.m_name;
    h.m_name  = ""; // invalidate whatever!
    --m_numAttribs;
    notify( name );
}

template <typename F>
void AttribManager::for_each_attrib( const F& func ) const {
    for ( const auto& attr : m_attribs )
        if ( attr != nullptr ) func( attr.get() );
}

template <typename F>
void AttribManager::for_each_attrib( const F& func ) {
    for ( auto& attr : m_attribs )
        if ( attr != nullptr ) func( attr.get() );
}

int AttribManager::getNumAttribs() const {
    return m_numAttribs;
}

} // namespace Utils
} // namespace Core
} // namespace Ra
