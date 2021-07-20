#include <Engine/Data/ShaderProgram.hpp>

#include <globjects/NamedString.h>
#include <globjects/Program.h>
#include <globjects/Shader.h>
#include <globjects/Texture.h>
#include <globjects/base/File.h>
#include <globjects/base/StaticStringSource.h>

#include <Core/Utils/Log.hpp>
#include <Engine/Data/Texture.hpp>

#include <numeric> // for std::accumulate
#include <regex>

namespace Ra {
namespace Engine {
namespace Data {

using namespace Core::Utils; // log

// The two following methods are independent of any ShaderProgram object.
// Fixed : made them local function to remove dependency on openGL.h for the class header
GLenum getTypeAsGLEnum( ShaderType type ) {
    switch ( type )
    {
    case ShaderType_VERTEX:
        return GL_VERTEX_SHADER;
    case ShaderType_FRAGMENT:
        return GL_FRAGMENT_SHADER;
    case ShaderType_GEOMETRY:
        return GL_GEOMETRY_SHADER;
    case ShaderType_TESS_EVALUATION:
        return GL_TESS_EVALUATION_SHADER;
    case ShaderType_TESS_CONTROL:
        return GL_TESS_CONTROL_SHADER;
#ifndef OS_MACOS
        // GL_COMPUTE_SHADER requires OpenGL >= 4.2, Apple provides OpenGL 4.1
    case ShaderType_COMPUTE:
        return GL_COMPUTE_SHADER;
#endif
    default:
        CORE_ERROR( "Wrong ShaderType" );
    }

    // Should never get there
    return GL_ZERO;
}

ShaderType getGLenumAsType( GLenum type ) {
    switch ( type )
    {
    case GL_VERTEX_SHADER:
        return ShaderType_VERTEX;
    case GL_FRAGMENT_SHADER:
        return ShaderType_FRAGMENT;
    case GL_GEOMETRY_SHADER:
        return ShaderType_GEOMETRY;
    case GL_TESS_EVALUATION_SHADER:
        return ShaderType_TESS_EVALUATION;
    case GL_TESS_CONTROL_SHADER:
        return ShaderType_TESS_CONTROL;
#ifndef OS_MACOS
    case GL_COMPUTE_SHADER:
        return ShaderType_COMPUTE;
#endif
    default:
        CORE_ERROR( "Wrong GLenum" );
    }

    // Should never get there
    return ShaderType_COUNT;
}

ShaderProgram::ShaderProgram() : m_program { nullptr } {
    std::generate( m_shaderObjects.begin(), m_shaderObjects.end(), []() {
        return std::pair<bool, std::unique_ptr<globjects::Shader>> { false, nullptr };
    } );
    std::fill( m_shaderSources.begin(), m_shaderSources.end(), nullptr );
}

ShaderProgram::ShaderProgram( const ShaderConfiguration& config ) : ShaderProgram() {
    load( config );
}

ShaderProgram::~ShaderProgram() {
    // first delete shader objects (before program and source) since it refer to
    // them during delete
    // See ~Shader (setSource(nullptr)
    for ( auto& s : m_shaderObjects )
    { s.second.reset( nullptr ); }
    for ( auto& s : m_shaderSources )
    { s.reset( nullptr ); }
    m_program.reset( nullptr );
}

void ShaderProgram::loadShader( ShaderType type,
                                const std::string& name,
                                const std::set<std::string>& props,
                                const std::vector<std::pair<std::string, ShaderType>>& includes,
                                bool fromFile,
                                const std::string& version ) {
#ifdef OS_MACOS
    if ( type == ShaderType_COMPUTE )
    {
        LOG( logERROR ) << "No compute shader on OsX !";
        return;
    }
#endif
    // Radium V2 --> for the moment : standard includepaths. Might be controlled per shader ...
    // Paths in which globjects will be looking for shaders includes.
    // "/" refer to the root of the directory structure conaining the shader (i.e. the Shaders/
    // directory).

    // header string that contains #version and pre-declarations ...
    std::string shaderHeader;
    if ( type == ShaderType_VERTEX )
    {
        shaderHeader = std::string( version + "\n\n"
                                              "out gl_PerVertex {\n"
                                              "    vec4 gl_Position;\n"
                                              "    float gl_PointSize;\n"
                                              "    float gl_ClipDistance[];\n"
                                              "};\n\n" );
    }
    else
    { shaderHeader = std::string( version + "\n\n" ); }

    // Add properties at the beginning of the file.
    shaderHeader = std::accumulate(
        props.begin(), props.end(), shaderHeader, []( std::string a, const std::string& b ) {
            return std::move( a ) + b + std::string( "\n" );
        } );

    // Add includes, depending on the shader type.
    shaderHeader = std::accumulate(
        includes.begin(),
        includes.end(),
        shaderHeader,
        [type]( std::string a, const std::pair<std::string, ShaderType>& b ) -> std::string {
            if ( b.second == type ) { return std::move( a ) + b.first + std::string( "\n" ); }
            else
            { return a; }
        } );

    std::unique_ptr<globjects::StaticStringSource> fullsource { nullptr };
    if ( fromFile )
    {
        auto loadedSource = globjects::Shader::sourceFromFile( name );
        fullsource = globjects::Shader::sourceFromString( shaderHeader + loadedSource->string() );
    }
    else
    { fullsource = globjects::Shader::sourceFromString( shaderHeader + name ); }

    // Radium V2 : allow to define global replacement per renderer, shader, rendertechnique ...
    auto shaderSource = globjects::Shader::applyGlobalReplacements( fullsource.get() );

#if 1
    // Workaround for #include directive
    // Radium VB2 : rely on GL_ARB_shading_language_include to manage includes
    std::string preprocessedSource = preprocessIncludes( name, shaderSource->string(), 0 );

    auto ptrSource = globjects::Shader::sourceFromString( preprocessedSource );
#else
    // this code do not work, include are not processed :( maybe a globjects bug.
    auto ptrSource = globjects::Shader::sourceFromString( shaderSource->string() );
#endif
    addShaderFromSource( type, std::move( ptrSource ), name, fromFile );
}

void ShaderProgram::addShaderFromSource( ShaderType type,
                                         std::unique_ptr<globjects::StaticStringSource>&& ptrSource,
                                         const std::string& name,
                                         bool fromFile ) {

    auto shader = globjects::Shader::create( getTypeAsGLEnum( type ) );

    shader->setName( name );
    shader->setSource( ptrSource.get() );
    shader->compile();

    GL_CHECK_ERROR;
    m_shaderObjects[type].first = fromFile;
    m_shaderObjects[type].second.swap( shader );

    m_shaderSources[type].swap( ptrSource );
    // ^^^ raw ptrSource are stored in shader object, need to keep them valid during
    // shader life
}

void ShaderProgram::load( const ShaderConfiguration& shaderConfig ) {
    m_configuration = shaderConfig;

    CORE_ERROR_IF( m_configuration.isComplete(),
                   ( "Shader program " + shaderConfig.m_name +
                     " is incomplete (e.g. misses vertex or fragment shader)." )
                       .c_str() );

    for ( size_t i = 0; i < ShaderType_COUNT; ++i )
    {
        if ( !m_configuration.m_shaders[i].first.empty() )
        {
            loadShader( ShaderType( i ),
                        m_configuration.m_shaders[i].first,
                        m_configuration.getProperties(),
                        m_configuration.getIncludes(),
                        m_configuration.m_shaders[i].second,
                        m_configuration.m_version );
        }
    }

    link();
}

void ShaderProgram::link() {
    m_program = globjects::Program::create();

    for ( unsigned int i = 0; i < ShaderType_COUNT; ++i )
    {
        if ( m_shaderObjects[i].second ) { m_program->attach( m_shaderObjects[i].second.get() ); }
    }

    m_program->setParameter( GL_PROGRAM_SEPARABLE, GL_TRUE );

    m_program->link();
    GL_CHECK_ERROR;
    int texUnit = 0;
    auto total  = GLuint( m_program->get( GL_ACTIVE_UNIFORMS ) );
    textureUnits.clear();

    for ( GLuint i = 0; i < total; ++i )
    {
        auto name = m_program->getActiveUniformName( i );
        auto type = m_program->getActiveUniform( i, GL_UNIFORM_TYPE );

        //!\todo add other sampler type (or manage all type of sampler automatically)
        if ( type == GL_SAMPLER_1D || type == GL_SAMPLER_2D || type == GL_SAMPLER_3D ||
             type == GL_SAMPLER_CUBE || type == GL_SAMPLER_1D_SHADOW ||
             type == GL_SAMPLER_2D_SHADOW || type == GL_SAMPLER_CUBE_SHADOW ||
             type == GL_SAMPLER_2D_RECT || type == GL_SAMPLER_2D_RECT_SHADOW ||
             type == GL_SAMPLER_1D_ARRAY || type == GL_SAMPLER_2D_ARRAY ||
             type == GL_SAMPLER_BUFFER || type == GL_SAMPLER_1D_ARRAY_SHADOW ||
             type == GL_SAMPLER_2D_ARRAY_SHADOW || type == GL_INT_SAMPLER_1D ||
             type == GL_INT_SAMPLER_2D || type == GL_INT_SAMPLER_3D ||
             type == GL_INT_SAMPLER_CUBE || type == GL_INT_SAMPLER_2D_RECT ||
             type == GL_INT_SAMPLER_1D_ARRAY || type == GL_INT_SAMPLER_2D_ARRAY ||
             type == GL_INT_SAMPLER_BUFFER || type == GL_UNSIGNED_INT_SAMPLER_1D ||
             type == GL_UNSIGNED_INT_SAMPLER_2D || type == GL_UNSIGNED_INT_SAMPLER_3D ||
             type == GL_UNSIGNED_INT_SAMPLER_CUBE || type == GL_UNSIGNED_INT_SAMPLER_2D_RECT ||
             type == GL_UNSIGNED_INT_SAMPLER_1D_ARRAY || type == GL_UNSIGNED_INT_SAMPLER_2D_ARRAY ||
             type == GL_UNSIGNED_INT_SAMPLER_BUFFER || type == GL_SAMPLER_CUBE_MAP_ARRAY ||
             type == GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW || type == GL_INT_SAMPLER_CUBE_MAP_ARRAY ||
             type == GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY || type == GL_SAMPLER_2D_MULTISAMPLE ||
             type == GL_INT_SAMPLER_2D_MULTISAMPLE ||
             type == GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE ||
             type == GL_SAMPLER_2D_MULTISAMPLE_ARRAY ||
             type == GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY ||
             type == GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY )
        {
            auto location      = m_program->getUniformLocation( name );
            textureUnits[name] = TextureBinding( texUnit++, location );
        }
    }
}

void ShaderProgram::bind() const {
    m_program->use();
}

void ShaderProgram::validate() const {
    m_program->validate();
    if ( !m_program->isValid() ) { LOG( logDEBUG ) << m_program->infoLog(); }
}

void ShaderProgram::unbind() const {
    m_program->release();
}

void ShaderProgram::reload() {
    for ( auto& s : m_shaderObjects )
    {
        if ( s.second != nullptr )
        {
            if ( s.first ) { LOG( logDEBUG ) << "Reloading shader " << s.second->name(); }

            m_program->detach( s.second.get() );
            loadShader( getGLenumAsType( s.second->type() ),
                        s.second->name(),
                        m_configuration.getProperties(),
                        m_configuration.getIncludes(),
                        s.first,
                        m_configuration.m_version );
        }
    }

    link();
}

ShaderConfiguration ShaderProgram::getBasicConfiguration() const {
    ShaderConfiguration basicConfig;

    basicConfig.m_shaders = m_configuration.m_shaders;
    basicConfig.m_name    = m_configuration.m_name;

    return basicConfig;
}

template <>
void ShaderProgram::setUniform( const char* name, const Core::Vector2d& value ) const {
    m_program->setUniform( name, value.cast<GL_SCALAR_PLAIN>().eval() );
}

template <>
void ShaderProgram::setUniform( const char* name, const Core::Vector3d& value ) const {
    m_program->setUniform( name, value.cast<GL_SCALAR_PLAIN>().eval() );
}

template <>
void ShaderProgram::setUniform( const char* name, const Core::Vector4d& value ) const {
    m_program->setUniform( name, value.cast<GL_SCALAR_PLAIN>().eval() );
}

template <>
void ShaderProgram::setUniform( const char* name, const Core::Matrix2d& value ) const {
    m_program->setUniform( name, value.cast<GL_SCALAR_PLAIN>().eval() );
}

template <>
void ShaderProgram::setUniform( const char* name, const Core::Matrix3d& value ) const {
    m_program->setUniform( name, value.cast<GL_SCALAR_PLAIN>().eval() );
}

template <>
void ShaderProgram::setUniform( const char* name, const Core::Matrix4d& value ) const {
    m_program->setUniform( name, value.cast<GL_SCALAR_PLAIN>().eval() );
}

template <>
void ShaderProgram::setUniform( const char* name, const Scalar& value ) const {
    m_program->setUniform( name, static_cast<GL_SCALAR_PLAIN>( value ) );
}

void ShaderProgram::setUniform( const char* name, Texture* tex, int texUnit ) const {
    tex->bind( texUnit );

    m_program->setUniform( name, texUnit );
}

void ShaderProgram::setUniformTexture( const char* name, Texture* tex ) const {
    auto itr = textureUnits.find( std::string( name ) );
    if ( itr != textureUnits.end() )
    {
        tex->bind( itr->second.m_texUnit );
        m_program->setUniform( itr->second.m_location, itr->second.m_texUnit );
    }
}

globjects::Program* ShaderProgram::getProgramObject() const {
    return m_program.get();
}

/****************************************************
 * Include workaround due to globject bugs
 ****************************************************/
std::string ShaderProgram::preprocessIncludes( const std::string& name,
                                               const std::string& shader,
                                               int level,
                                               int line ) {
    CORE_UNUSED( line ); // left for radium v2 ??
    CORE_ERROR_IF( level < 32, "Shader inclusion depth limit reached." );

    std::string result {};
    std::vector<std::string> finalStrings;

    uint nline = 0;

    static const std::regex reg( "^[ ]*#[ ]*include[ ]+[\"<](.*)[\">].*" );

    // source: https://www.fluentcpp.com/2017/04/21/how-to-split-a-string-in-c/
    std::istringstream iss( shader );
    std::string codeline;
    while ( std::getline( iss, codeline, '\n' ) )
    {
        std::smatch match;
        if ( std::regex_search( codeline, match, reg ) )
        {
            // Radium V2 : for composable shaders, use the includePaths set elsewhere.
            auto includeNameString =
                globjects::NamedString::getFromRegistry( std::string( "/" ) + match[1].str() );
            if ( includeNameString != nullptr )
            {

                codeline =
                    preprocessIncludes( match[1].str(), includeNameString->string(), level + 1, 0 );
            }
            else
            {
                LOG( logWARNING ) << "Cannot open included file " << match[1].str() << " at line"
                                  << nline << " of file " << name << ". Ignored.";
                continue;
            }
            // Radium V2, adapt this if globjct includes bug is still present
            /*
             std::string inc;
             std::string file = m_filepath + match[1].str();
             if (parseFile(file, inc))
             {
             sublerr.start = nline;
             sublerr.name  = file;
             lerr.subfiles.push_back(sublerr);

             line  = preprocessIncludes(inc, level + 1, lerr.subfiles.back());
             nline = lerr.subfiles.back().end;
             }
             else
             {
             LOG(logWARNING) << "Cannot open included file " << file << " from " << m_filename << ".
             Ignored."; continue;
             }
             */
        }

        finalStrings.push_back( codeline );
        ++nline;
    }

    // Build final shader string
    for ( const auto& l : finalStrings )
    {
        result.append( l );
        result.append( "\n" );
    }

    result.append( "\0" );

    return result;
}

} // namespace Data
} // namespace Engine
} // namespace Ra
