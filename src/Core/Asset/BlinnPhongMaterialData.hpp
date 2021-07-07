#pragma once

#include <Core/Asset/MaterialData.hpp>
#include <Core/Utils/Color.hpp>

namespace Ra {
namespace Core {
namespace Asset {

// RADIUM SUPPORTED MATERIALS
class RA_CORE_API BlinnPhongMaterialData : public MaterialData
{
  public:
    explicit BlinnPhongMaterialData( const std::string& name = "" );

    /// DEBUG
    inline void displayInfo() const final;

    /// QUERY
    inline bool hasDiffuse() const;

    inline bool hasSpecular() const;

    inline bool hasShininess() const;

    inline bool hasOpacity() const;

    inline bool hasDiffuseTexture() const;

    inline bool hasSpecularTexture() const;

    inline bool hasShininessTexture() const;

    inline bool hasNormalTexture() const;

    inline bool hasOpacityTexture() const;

    /// DATA MEMBERS
    Core::Utils::Color m_diffuse;
    Core::Utils::Color m_specular;
    Scalar m_shininess;
    Scalar m_opacity;
    std::string m_texDiffuse;
    std::string m_texSpecular;
    std::string m_texShininess;
    std::string m_texNormal;
    std::string m_texOpacity;
    bool m_hasDiffuse;
    bool m_hasSpecular;
    bool m_hasShininess;
    bool m_hasOpacity;
    bool m_hasTexDiffuse;
    bool m_hasTexSpecular;
    bool m_hasTexShininess;
    bool m_hasTexNormal;
    bool m_hasTexOpacity;
    bool m_diffuseFromMesh;
};

} // namespace Asset
} // namespace Core
} // namespace Ra

#include <Core/Asset/BlinnPhongMaterialData.inl>
