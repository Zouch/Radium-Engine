#include <Engine/Data/ShaderConfiguration.hpp>
#include <Engine/Data/ShaderProgram.hpp>
#include <Engine/Data/ShaderProgramManager.hpp>

#include <Core/Containers/MakeShared.hpp>
#include <Core/Utils/Log.hpp>

#include <globjects/NamedString.h>
#include <globjects/Shader.h>
#include <globjects/base/File.h>

namespace Ra {
namespace Engine {
namespace Data {

using namespace Core::Utils; // log
using ShaderProgramPtr = std::shared_ptr<Data::ShaderProgram>;

ShaderProgramManager::ShaderProgramManager() {}

ShaderProgramManager::~ShaderProgramManager() {
    m_shaderPrograms.clear();
}

bool ShaderProgramManager::addNamedString( const std::string& includepath,
                                           const std::string& realfile ) {
    auto el = m_namedStrings.find( includepath );
    if ( el != m_namedStrings.end() ) {
        if ( el->second.first->filePath() != realfile ) {
            LOG( logWARNING )
                << "[ShaderProgramManager] A named string already exists with this key: "
                << includepath << " --> " << el->second.first->filePath();
        }
        else {
            LOG( logINFO ) << "[ShaderProgramManager] Named string already inserted, skipping: "
                           << includepath << " --> " << realfile;
        }

        return false;
    }

    auto file                   = globjects::File::create( realfile );
    m_namedStrings[includepath] = std::make_pair(
        std::move( file ), globjects::NamedString::create( includepath, file.get() ) );

    return true;
}

void ShaderProgramManager::reloadNamedString() {
    for ( auto& el : m_namedStrings ) {
        el.second.first->reload();
        el.second.second.reset( nullptr );
        el.second.second = globjects::NamedString::create( el.first, el.second.first.get() );
    }
}

Core::Utils::optional<const Data::ShaderProgram*>
ShaderProgramManager::addShaderProgram( const Data::ShaderConfiguration& config ) {
    auto found = m_shaderPrograms.find( config );

    if ( found != m_shaderPrograms.end() ) { return found->second.get(); }

    // add named strings
    for ( const auto& p : config.getNamedStrings() ) {
        addNamedString( p.first, p.second );
    }

    // Try to load the shader
    auto prog = Core::make_shared<Data::ShaderProgram>( config );

    if ( prog->getProgramObject()->isLinked() ) {
        insertShader( config, prog );
        return prog.get();
    }
    else {

        LOG( logERROR ) << "Error occurred while loading shader program "
                        << config.getName().c_str() << ":\nDefault shader program used instead.";

        for ( const auto& strings : config.getNamedStrings() ) {
            auto el = m_namedStrings.find( strings.first );
            if ( el != m_namedStrings.end() ) {
                if ( el->second.first->filePath() == strings.second ) {
                    LOG( logINFO ) << "[ShaderProgramManager] Removing named string "
                                   << strings.first << " --> " << strings.second;
                }
            }
        }

        // insert in the failed shaders list
        m_shaderFailedConfs.push_back( config );

        return {};
    }
}

const Data::ShaderProgram* ShaderProgramManager::getShaderProgram( const std::string& id ) {
    auto found = m_shaderProgramIds.find( id );

    if ( found != m_shaderProgramIds.end() ) { return getShaderProgram( found->second ); }
    return nullptr;
}

const Data::ShaderProgram*
ShaderProgramManager::getShaderProgram( const Data::ShaderConfiguration& config ) {
    auto res = addShaderProgram( config );
    return bool( res ) ? *res : nullptr;
}

void ShaderProgramManager::reloadAllShaderPrograms() {
    // update the include registry
    reloadNamedString();

    // For each shader in the map
    for ( auto& shader : m_shaderPrograms ) {
        shader.second->reload();
    }

    // and also try the failed ones
    reloadNotCompiledShaderPrograms();
}

void ShaderProgramManager::reloadNotCompiledShaderPrograms() {
    // for each shader in the failed map, try to reload
    for ( const auto& conf : m_shaderFailedConfs ) {
        auto prog = Core::make_shared<Data::ShaderProgram>( conf );

        if ( prog->getProgramObject()->isValid() ) {
            insertShader( conf, prog );
            // m_shaderFailedConfs.erase(conf);
        }
    }
}

void ShaderProgramManager::insertShader( const Data::ShaderConfiguration& config,
                                         const ShaderProgramPtr& shader ) {
    m_shaderProgramIds.insert( { config.getName(), config } );
    m_shaderPrograms.insert( { config, shader } );
}

} // namespace Data
} // namespace Engine
} // namespace Ra
