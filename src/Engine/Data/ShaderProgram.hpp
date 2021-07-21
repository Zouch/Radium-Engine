#pragma once

#include <Engine/RaEngine.hpp>

#include <Core/Types.hpp>

#include <Engine/Data/ShaderConfiguration.hpp>

#include <array>
#include <memory>
#include <string>

namespace globjects {
class Shader;
class Program;
class NamedString;
class StaticStringSource;
} // namespace globjects

namespace Ra {
namespace Engine {
namespace Data {

class Texture;

/**
 * Abstraction of OpenGL Shader Program
 * @see globjects::Program and https://www.khronos.org/opengl/wiki/Shader
 *
 */
class RA_ENGINE_API ShaderProgram final
{
  public:
    ShaderProgram();
    explicit ShaderProgram( const Data::ShaderConfiguration& shaderConfig );
    ~ShaderProgram();

    void load( const Data::ShaderConfiguration& shaderConfig );
    void reload();

    Data::ShaderConfiguration getBasicConfiguration() const;

    void bind() const;
    void validate() const;
    void unbind() const;

    /// Uniform setters
    template <typename T>
    void setUniform( const char* name, const T& value ) const;

    void setUniform( const char* name, Texture* tex, int texUnit ) const;

    //! use automatic texture unit computation
    //! if you want to send a particular texture unit, use setUniform.
    //! It binds tex on an "arbitrary" tex unit.
    //! @warning, call a std::map::find (in O(log(active tex unit in the shader)))
    void setUniformTexture( const char* name, Texture* tex ) const;

    globjects::Program* getProgramObject() const;

    ///\todo go private, and update ShaderConfiguration to add from source !
    void addShaderFromSource( Data::ShaderType type,
                              std::unique_ptr<globjects::StaticStringSource>&& source,
                              const std::string& name = "",
                              bool fromFile           = true );

    void link();

  private:
    struct TextureBinding {
        int m_texUnit { -1 };
        int m_location { -1 };
        TextureBinding( int unit, int location ) : m_texUnit { unit }, m_location { location } {}
        TextureBinding() = default;
    };
    using TextureUnits = std::map<std::string, TextureBinding>;
    TextureUnits textureUnits;

    void loadShader( Data::ShaderType type,
                     const std::string& name,
                     const std::set<std::string>& props,
                     const std::vector<std::pair<std::string, Data::ShaderType>>& includes,
                     bool fromFile              = true,
                     const std::string& version = "#version 410" );

    std::string preprocessIncludes( const std::string& name,
                                    const std::string& shader,
                                    int level,
                                    int line = 0 );

  private:
    Data::ShaderConfiguration m_configuration;

    std::array<std::pair<bool, std::unique_ptr<globjects::Shader>>, Data::ShaderType_COUNT>
        m_shaderObjects;
    std::array<std::unique_ptr<globjects::StaticStringSource>, Data::ShaderType_COUNT>
        m_shaderSources;

    std::unique_ptr<globjects::Program> m_program;
};
} // namespace Data

namespace Rendering {} // namespace Rendering
} // namespace Engine
} // namespace Ra

#include "ShaderProgram.inl"
