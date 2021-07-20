#include <Core/Utils/Log.hpp>
#include <Engine/Data/RenderParameters.hpp>
#include <Engine/Data/ShaderProgram.hpp>

namespace Ra {
namespace Engine {
namespace Data {
void RenderParameters::bind( const Data::ShaderProgram* shader ) const {
    m_boolParamsVector.bind( shader );
    m_intParamsVector.bind( shader );
    m_uintParamsVector.bind( shader );
    m_scalarParamsVector.bind( shader );
    m_intsParamsVector.bind( shader );
    m_uintsParamsVector.bind( shader );
    m_scalarsParamsVector.bind( shader );
    m_vec2ParamsVector.bind( shader );
    m_vec3ParamsVector.bind( shader );
    m_vec4ParamsVector.bind( shader );
    m_colorParamsVector.bind( shader );
    m_mat2ParamsVector.bind( shader );
    m_mat3ParamsVector.bind( shader );
    m_mat4ParamsVector.bind( shader );
    m_texParamsVector.bind( shader );
}

void RenderParameters::addParameter( const std::string& name, bool value ) {
    m_boolParamsVector[name] = BoolParameter( name, value );
}

void RenderParameters::addParameter( const std::string& name, int value ) {
    m_intParamsVector[name] = IntParameter( name, value );
}

void RenderParameters::addParameter( const std::string& name, uint value ) {
    m_uintParamsVector[name] = UIntParameter( name, value );
}

void RenderParameters::addParameter( const std::string& name, Scalar value ) {
    m_scalarParamsVector[name] = ScalarParameter( name, value );
}

///!! array version

void RenderParameters::addParameter( const std::string& name, std::vector<int> value ) {
    m_intsParamsVector[name] = IntsParameter( name, value );
}

void RenderParameters::addParameter( const std::string& name, std::vector<uint> value ) {
    m_uintsParamsVector[name] = UIntsParameter( name, value );
}

void RenderParameters::addParameter( const std::string& name, std::vector<float> value ) {
    m_scalarsParamsVector[name] = ScalarsParameter( name, value );
}

///!!

void RenderParameters::addParameter( const std::string& name, const Core::Vector2& value ) {
    m_vec2ParamsVector[name] = Vec2Parameter( name, value );
}

void RenderParameters::addParameter( const std::string& name, const Core::Vector3& value ) {
    m_vec3ParamsVector[name] = Vec3Parameter( name, value );
}

void RenderParameters::addParameter( const std::string& name, const Core::Vector4& value ) {
    m_vec4ParamsVector[name] = Vec4Parameter( name, value );
}

void RenderParameters::addParameter( const std::string& name, const Core::Utils::Color& value ) {
    m_colorParamsVector[name] = ColorParameter( name, value );
}

void RenderParameters::addParameter( const std::string& name, const Core::Matrix2& value ) {
    m_mat2ParamsVector[name] = Mat2Parameter( name, value );
}

void RenderParameters::addParameter( const std::string& name, const Core::Matrix3& value ) {
    m_mat3ParamsVector[name] = Mat3Parameter( name, value );
}

void RenderParameters::addParameter( const std::string& name, const Core::Matrix4& value ) {
    m_mat4ParamsVector[name] = Mat4Parameter( name, value );
}

void RenderParameters::addParameter( const std::string& name, Data::Texture* tex, int texUnit ) {
    m_texParamsVector[name] = TextureParameter( name, tex, texUnit );
}

void RenderParameters::concatParameters( const RenderParameters& params ) {
    for ( const auto& param : params.m_intParamsVector )
    { m_intParamsVector.insert( param ); }

    for ( const auto& param : params.m_uintParamsVector )
    { m_uintParamsVector.insert( param ); }

    for ( const auto& param : params.m_scalarParamsVector )
    { m_scalarParamsVector.insert( param ); }

    for ( const auto& param : params.m_vec2ParamsVector )
    { m_vec2ParamsVector.insert( param ); }

    for ( const auto& param : params.m_vec3ParamsVector )
    { m_vec3ParamsVector.insert( param ); }

    for ( const auto& param : params.m_vec4ParamsVector )
    { m_vec4ParamsVector.insert( param ); }

    for ( const auto& param : params.m_mat2ParamsVector )
    { m_mat2ParamsVector.insert( param ); }

    for ( const auto& param : params.m_mat3ParamsVector )
    { m_mat3ParamsVector.insert( param ); }

    for ( const auto& param : params.m_mat4ParamsVector )
    { m_mat4ParamsVector.insert( param ); }

    for ( const auto& param : params.m_texParamsVector )
    { m_texParamsVector.insert( param ); }
}

} // namespace Data
} // namespace Engine
} // namespace Ra
