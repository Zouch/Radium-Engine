#pragma once
#include <Engine/Data/RenderParameters.hpp>
#include <Engine/Data/ShaderProgram.hpp>

namespace Ra {
namespace Engine {
namespace Data {

template <typename T>
inline void
RenderParameters::UniformBindableSet<T>::bind( const Data::ShaderProgram* shader ) const {
    for ( const auto& value : *this )
    { value.second.bind( shader ); }
}

template <typename T>
inline void RenderParameters::TParameter<T>::bind( const Data::ShaderProgram* shader ) const {
    shader->setUniform( m_name.c_str(), m_value );
}

template <>
inline void RenderParameters::TParameter<Ra::Core::Utils::Color>::bind(
    const Data::ShaderProgram* shader ) const {
    shader->setUniform( m_name.c_str(), Ra::Core::Utils::Color::VectorType( m_value ) );
}

inline void RenderParameters::TextureParameter::bind( const Data::ShaderProgram* shader ) const {
    if ( m_texUnit == -1 ) { shader->setUniformTexture( m_name.c_str(), m_texture ); }
    else
    { shader->setUniform( m_name.c_str(), m_texture, m_texUnit ); }
}

template <>
inline const RenderParameters::UniformBindableSet<RenderParameters::BoolParameter>&
RenderParameters::getParameterSet() const {
    return m_boolParamsVector;
}

template <>
inline const RenderParameters::UniformBindableSet<RenderParameters::IntParameter>&
RenderParameters::getParameterSet() const {
    return m_intParamsVector;
}

template <>
inline const RenderParameters::UniformBindableSet<RenderParameters::UIntParameter>&
RenderParameters::getParameterSet() const {
    return m_uintParamsVector;
}

template <>
inline const RenderParameters::UniformBindableSet<RenderParameters::ScalarParameter>&
RenderParameters::getParameterSet() const {
    return m_scalarParamsVector;
}

template <>
inline const RenderParameters::UniformBindableSet<RenderParameters::IntsParameter>&
RenderParameters::getParameterSet() const {
    return m_intsParamsVector;
}

template <>
inline const RenderParameters::UniformBindableSet<RenderParameters::UIntsParameter>&
RenderParameters::getParameterSet() const {
    return m_uintsParamsVector;
}

template <>
inline const RenderParameters::UniformBindableSet<RenderParameters::ScalarsParameter>&
RenderParameters::getParameterSet() const {
    return m_scalarsParamsVector;
}

template <>
inline const RenderParameters::UniformBindableSet<RenderParameters::Vec2Parameter>&
RenderParameters::getParameterSet() const {
    return m_vec2ParamsVector;
}

template <>
inline const RenderParameters::UniformBindableSet<RenderParameters::Vec3Parameter>&
RenderParameters::getParameterSet() const {
    return m_vec3ParamsVector;
}

template <>
inline const RenderParameters::UniformBindableSet<RenderParameters::Vec4Parameter>&
RenderParameters::getParameterSet() const {
    return m_vec4ParamsVector;
}

template <>
inline const RenderParameters::UniformBindableSet<RenderParameters::ColorParameter>&
RenderParameters::getParameterSet() const {
    return m_colorParamsVector;
}

template <>
inline const RenderParameters::UniformBindableSet<RenderParameters::Mat2Parameter>&
RenderParameters::getParameterSet() const {
    return m_mat2ParamsVector;
}

template <>
inline const RenderParameters::UniformBindableSet<RenderParameters::Mat3Parameter>&
RenderParameters::getParameterSet() const {
    return m_mat3ParamsVector;
}

template <>
inline const RenderParameters::UniformBindableSet<RenderParameters::Mat4Parameter>&
RenderParameters::getParameterSet() const {
    return m_mat4ParamsVector;
}

template <>
inline const RenderParameters::UniformBindableSet<RenderParameters::TextureParameter>&
RenderParameters::getParameterSet() const {
    return m_texParamsVector;
}

} // namespace Data
} // namespace Engine
} // namespace Ra
