#include <Engine/Data/Material.hpp>

namespace Ra {
namespace Engine {
namespace Data {
Material::Material( const std::string& instanceName,
                    const std::string& materialName,
                    MaterialAspect aspect ) :
    ShaderParameterProvider(),
    m_instanceName { instanceName },
    m_aspect { aspect },
    m_materialName { materialName } {}

bool Material::isTransparent() const {
    return m_aspect == MaterialAspect::MAT_TRANSPARENT;
}

std::list<std::string> Material::getPropertyList() const {
    return ShaderParameterProvider::getPropertyList();
}
} // namespace Data
} // namespace Engine
} // namespace Ra
