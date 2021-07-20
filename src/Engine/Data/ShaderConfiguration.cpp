#include <Core/Resources/Resources.hpp>
#include <Engine/Data/ShaderConfiguration.hpp>
#include <Engine/Data/ShaderProgramManager.hpp>

/**
 * Plain will be the default shader program
 */
static const std::string defaultVertexShader {
    Ra::Core::Resources::getRadiumResourcesPath().value_or(
        "[[Default resrouces path not found]]" ) +
    "Shaders/Materials/Plain/Plain.vert.glsl" };
static const std::string defaultFragmentShader {
    Ra::Core::Resources::getRadiumResourcesPath().value_or(
        "[[Default resrouces path not found]]" ) +
    "Shaders/Materials/Plain/Plain.frag.glsl" };

namespace Ra {
namespace Engine {
namespace Data {

std::ostream& operator<<( std::ostream& stream, const ShaderConfiguration& config ) {

    stream << " -- shader configuration [" << config.m_name << "]\n";

    for ( const auto& s : config.m_shaders )
    { stream << " shaders  [" << s.first << " " << s.second << " ]\n"; }

    for ( const auto& s : config.m_properties )
    { stream << "props  [" << s << "]\n"; }

    for ( const auto& s : config.m_includes )
    { stream << "inc  [" << s.first << "]\n"; }
    for ( const auto& s : config.m_named_strings )
    { stream << "props  [" << s.first << "]\n"; }
    return stream;
}

ShaderConfiguration ShaderConfiguration::m_defaultShaderConfig( "Default Program",
                                                                defaultVertexShader,
                                                                defaultFragmentShader );

ShaderConfiguration::ShaderConfiguration( const std::string& name ) :
    m_name { name }, m_version { "#version 410" } {}

ShaderConfiguration::ShaderConfiguration( const std::string& name,
                                          const std::string& vertexShader,
                                          const std::string& fragmentShader ) :
    m_name { name }, m_version { "#version 410" } {
    m_shaders[ShaderType_VERTEX]   = { vertexShader, true };
    m_shaders[ShaderType_FRAGMENT] = { fragmentShader, true };
}

void ShaderConfiguration::addShader( ShaderType type, const std::string& name ) {
    m_shaders[type] = { name, true };
}

void ShaderConfiguration::addShaderSource( ShaderType type, const std::string& source ) {
    m_shaders[type] = { source, false };
}

void ShaderConfiguration::addProperty( const std::string& prop ) {
    m_properties.insert( "#define " + prop );
}

void ShaderConfiguration::addProperties( const std::list<std::string>& props ) {
    for ( const auto& prop : props )
    { m_properties.insert( "#define " + prop ); }
}

void ShaderConfiguration::removeProperty( const std::string& prop ) {
    m_properties.erase( "#define " + prop );
}

void ShaderConfiguration::addInclude( const std::string& incl, ShaderType type ) {
    m_includes.emplace_back( "#include " + incl, type );
}

void ShaderConfiguration::addIncludes( const std::list<std::string>& incls, ShaderType type ) {
    for ( const auto& incl : incls )
    { m_includes.emplace_back( "#include " + incl, type ); }
}

void ShaderConfiguration::removeInclude( const std::string& incl, ShaderType type ) {
    CORE_UNUSED( type );
    m_properties.erase( "#include " + incl );
}

void ShaderConfiguration::addNamedString( const std::string& includepath,
                                          const std::string& realfile ) {
    m_named_strings.emplace_back( includepath, realfile );
}

bool ShaderConfiguration::isComplete() const {
    return ( ( !m_shaders[ShaderType_VERTEX].first.empty() ) &&
             ( !m_shaders[ShaderType_FRAGMENT].first.empty() ) ) ||
           !m_shaders[ShaderType_COMPUTE].first.empty();
}

bool ShaderConfiguration::operator<( const ShaderConfiguration& o ) const {
    bool res = false;

    for ( size_t i = 0; i < ShaderType_COUNT; ++i )
    {
        if ( m_shaders[i] != o.m_shaders[i] ) { return m_shaders[i] < o.m_shaders[i]; }
    }

    if ( m_properties.size() == o.m_properties.size() )
    {
        if ( m_properties.empty() )
        {
            if ( m_includes.size() == o.m_includes.size() )
            {
                if ( m_includes.empty() ) { res = false; }
                else
                {
                    auto lit = m_includes.begin();
                    auto rit = o.m_includes.begin();

                    for ( ; ( lit != m_includes.end() ) && ( *lit == *rit ); ++lit, ++rit )
                        ;

                    if ( lit == m_includes.end() ) { res = false; }
                    else
                    { res = *lit < *rit; }
                }
            }
        }
        else
        {
            auto lit = m_properties.begin();
            auto rit = o.m_properties.begin();

            for ( ; ( lit != m_properties.end() ) && ( *lit == *rit ); ++lit, ++rit )
                ;

            if ( lit == m_properties.end() ) { res = false; }
            else
            { res = *lit < *rit; }
        }
    }
    else
    { res = m_properties.size() < o.m_properties.size(); }

    return res;
}

std::set<std::string> ShaderConfiguration::getProperties() const {
    return m_properties;
}

const std::vector<std::pair<std::string, ShaderType>>& ShaderConfiguration::getIncludes() const {
    return m_includes;
}

const std::vector<std::pair<std::string, std::string>>&
ShaderConfiguration::getNamedStrings() const {
    return m_named_strings;
}

} // namespace Data
} // namespace Engine
} // namespace Ra
