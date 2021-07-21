#pragma once
#include <Core/Asset/BlinnPhongMaterialData.hpp>
#include <Core/Utils/Log.hpp>

namespace Ra {
namespace Core {
namespace Asset {

///////////////////
/// BLINN PHONG ///
///////////////////
inline bool BlinnPhongMaterialData::hasDiffuse() const {
    return m_hasDiffuse;
}

inline bool BlinnPhongMaterialData::hasSpecular() const {
    return m_hasSpecular;
}

inline bool BlinnPhongMaterialData::hasShininess() const {
    return m_hasShininess;
}

inline bool BlinnPhongMaterialData::hasOpacity() const {
    return m_hasOpacity;
}

inline bool BlinnPhongMaterialData::hasDiffuseTexture() const {
    return m_hasTexDiffuse;
}

inline bool BlinnPhongMaterialData::hasSpecularTexture() const {
    return m_hasTexSpecular;
}

inline bool BlinnPhongMaterialData::hasShininessTexture() const {
    return m_hasTexShininess;
}

inline bool BlinnPhongMaterialData::hasNormalTexture() const {
    return m_hasTexNormal;
}

inline bool BlinnPhongMaterialData::hasOpacityTexture() const {
    return m_hasTexOpacity;
}

/// DEBUG
inline void BlinnPhongMaterialData::displayInfo() const {
    using namespace Core::Utils; // log
    auto print = []( bool ok, const std::string& name, const auto& value ) {
        if ( ok ) { LOG( logINFO ) << name << value; }
        else {
            LOG( logINFO ) << name << "NO";
        }
    };

    LOG( logINFO ) << "======== MATERIAL INFO ========";
    print( hasDiffuse(), " Kd             : ", m_diffuse.transpose() );
    print( hasSpecular(), " Ks             : ", m_specular.transpose() );
    print( hasShininess(), " Ns             : ", m_shininess );
    print( hasOpacity(), " Opacity        : ", m_opacity );
    print( hasDiffuseTexture(), " Kd Texture     : ", m_texDiffuse );
    print( hasSpecularTexture(), " Ks Texture     : ", m_texSpecular );
    print( hasShininessTexture(), " Ns Texture     : ", m_texShininess );
    print( hasNormalTexture(), " Normal Texture : ", m_texNormal );
    print( hasOpacityTexture(), " Alpha Texture  : ", m_texOpacity );
}

} // namespace Asset
} // namespace Core
} // namespace Ra
