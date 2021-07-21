#include <Core/Types.hpp>
#include <Core/Utils/Attribs.hpp>
#include <Core/Utils/Log.hpp>

namespace Ra {
namespace Core {
namespace Utils {

AttribBase::~AttribBase() {
    notify();
}

template <>
size_t Attrib<float>::getElementSize() const {
    return 1;
}

template <>
size_t Attrib<double>::getElementSize() const {
    return 1;
}

AttribManager::~AttribManager() {
    clear();
}

void AttribManager::clear() {
    m_attribs.clear();
    m_attribsIndex.clear();
}

void AttribManager::copyAllAttributes( const AttribManager& m ) {
    for ( const auto& attr : m.m_attribs ) {
        if ( attr == nullptr ) continue;
        if ( attr->isFloat() ) {
            auto h = addAttrib<Scalar>( attr->getName() );
            getAttrib( h ).setData( static_cast<Attrib<Scalar>*>( attr.get() )->data() );
        }
        else if ( attr->isVector2() ) {
            auto h = addAttrib<Vector2>( attr->getName() );
            getAttrib( h ).setData( static_cast<Attrib<Vector2>*>( attr.get() )->data() );
        }
        else if ( attr->isVector3() ) {
            auto h = addAttrib<Vector3>( attr->getName() );
            getAttrib( h ).setData( static_cast<Attrib<Vector3>*>( attr.get() )->data() );
        }
        else if ( attr->isVector4() ) {
            auto h = addAttrib<Vector4>( attr->getName() );
            getAttrib( h ).setData( static_cast<Attrib<Vector4>*>( attr.get() )->data() );
        }
        else
            LOG( logWARNING ) << "Warning, copy of mesh attribute " << attr->getName()
                              << " type is not supported (only float, vec2, vec3 nor vec4 are "
                                 "supported) [from AttribManager::copyAllAttribute()]";
    }
}

bool AttribManager::hasSameAttribs( const AttribManager& other ) {
    // one way
    for ( const auto& attr : m_attribsIndex ) {
        if ( other.m_attribsIndex.find( attr.first ) == other.m_attribsIndex.cend() ) {
            return false;
        }
    }
    // the other way
    for ( const auto& attr : other.m_attribsIndex ) {
        if ( m_attribsIndex.find( attr.first ) == m_attribsIndex.cend() ) { return false; }
    }
    return true;
}

} // namespace Utils
} // namespace Core
} // namespace Ra
