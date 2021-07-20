#include <Engine/Rendering/RenderTechnique.hpp>

#include <Engine/Data/BlinnPhongMaterial.hpp>
#include <Engine/Data/RenderParameters.hpp>
#include <Engine/Data/ShaderConfigFactory.hpp>
#include <Engine/Data/ShaderProgramManager.hpp>
#include <Engine/RadiumEngine.hpp>

#include <Core/Utils/Log.hpp>

namespace Ra {
namespace Engine {
namespace Rendering {

using namespace Core::Utils; // log

std::shared_ptr<Ra::Engine::Rendering::RenderTechnique> RadiumDefaultRenderTechnique { nullptr };

RenderTechnique::RenderTechnique() : m_numActivePass { 0 } {
    for ( auto p = Index( 0 ); p < s_maxNbPasses; ++p )
    {
        m_activePasses[p]     = PassConfiguration( Data::ShaderConfiguration(), nullptr );
        m_passesParameters[p] = nullptr;
    }
}

RenderTechnique::RenderTechnique( const RenderTechnique& o ) :
    m_numActivePass { o.m_numActivePass },
    m_dirtyBits { o.m_dirtyBits },
    m_setPasses { o.m_setPasses } {
    for ( auto p = Index( 0 ); p < m_numActivePass; ++p )
    {
        if ( hasConfiguration( p ) )
        {
            m_activePasses[p]     = o.m_activePasses[p];
            m_passesParameters[p] = o.m_passesParameters[p];
        }
    }
}

RenderTechnique::~RenderTechnique() = default;

void RenderTechnique::setConfiguration( const Data::ShaderConfiguration& newConfig,
                                        Core::Utils::Index pass ) {
    m_numActivePass      = std::max( m_numActivePass, pass + 1 );
    m_activePasses[pass] = PassConfiguration( newConfig, nullptr );
    setDirty( pass );
    setConfiguration( pass );
}

const Data::ShaderProgram* RenderTechnique::getShader( Core::Utils::Index pass ) const {
    if ( hasConfiguration( pass ) ) { return m_activePasses[pass].second; }
    return nullptr;
}

void RenderTechnique::setParametersProvider(
    std::shared_ptr<Data::ShaderParameterProvider> provider,
    Core::Utils::Index pass ) {
    if ( m_numActivePass == 0 )
    {
        LOG( logERROR )
            << "Unable to set pass parameters : is passes configured using setConfiguration ? ";
        return;
    }
    if ( pass.isValid() )
    {
        if ( hasConfiguration( pass ) ) { m_passesParameters[pass] = provider; }
    }
    else
    {
        for ( int i = 0; i < m_numActivePass; ++i )
        {
            if ( hasConfiguration( i ) ) { m_passesParameters[i] = provider; }
        }
    }
    // add the provider specific properties to the configuration
    addPassProperties( provider->getPropertyList() );
}

void RenderTechnique::addPassProperties( const std::list<std::string>& props,
                                         Core::Utils::Index pass ) {
    if ( m_numActivePass == 0 )
    {
        LOG( logERROR )
            << "Unable to set pass properties : is passes configured using setConfiguration ? ";
        return;
    }
    if ( pass.isValid() && hasConfiguration( pass ) )
    {
        m_activePasses[pass].first.addProperties( props );
        setDirty( pass );
    }
    else
    {
        for ( int i = 0; i < m_numActivePass; ++i )
        {
            if ( hasConfiguration( i ) )
            {
                m_activePasses[i].first.addProperties( props );
                setDirty( i );
            }
        }
    }
}

const Data::ShaderParameterProvider*
RenderTechnique::getParametersProvider( Core::Utils::Index pass ) const {
    if ( hasConfiguration( pass ) ) { return m_passesParameters[pass].get(); }
    return nullptr;
}

void RenderTechnique::updateGL() {
    auto shaderProgramManager = RadiumEngine::getInstance()->getShaderProgramManager();

    for ( auto p = Index( 0 ); p < m_numActivePass; ++p )
    {
        if ( hasConfiguration( p ) && ( ( nullptr == m_activePasses[p].second ) || isDirty( p ) ) )
        {
            m_activePasses[p].second =
                shaderProgramManager->getShaderProgram( m_activePasses[p].first );
            clearDirty( p );
        }
    }
    for ( auto p = Index( 0 ); p < m_numActivePass; ++p )
    {
        if ( m_passesParameters[p] ) { m_passesParameters[p]->updateGL(); }
    }
}

///////////////////////////////////////////////
RenderTechnique RenderTechnique::createDefaultRenderTechnique() {
    if ( RadiumDefaultRenderTechnique != nullptr )
    { return *( RadiumDefaultRenderTechnique.get() ); }
    std::shared_ptr<Data::Material> mat( new Data::BlinnPhongMaterial( "DefaultGray" ) );
    auto rt      = new RenderTechnique;
    auto builder = EngineRenderTechniques::getDefaultTechnique( "BlinnPhong" );
    if ( !builder.first )
    { LOG( logERROR ) << "Unable to create the default technique : is the Engine initialized ? "; }
    builder.second( *rt, false );
    rt->setParametersProvider( mat );
    RadiumDefaultRenderTechnique.reset( rt );
    return *( RadiumDefaultRenderTechnique.get() );
}

///////////////////////////////////////////////
////        Radium defined technique        ///
///////////////////////////////////////////////
namespace EngineRenderTechniques {

/// Map that stores each technique builder function
static std::map<std::string, DefaultTechniqueBuilder> EngineTechniqueRegistry;

/** register a new default builder for a technique
 *  @return true if builder added, false else (e.g, a builder with the same name exists)
 */
bool registerDefaultTechnique( const std::string& name, DefaultTechniqueBuilder builder ) {
    auto result = EngineTechniqueRegistry.insert( { name, builder } );
    return result.second;
}

/** remove a default builder
 *  @return true if builder removed, false else (e.g, a builder with the same name does't exists)
 */
bool removeDefaultTechnique( const std::string& name ) {
    std::size_t removed = EngineTechniqueRegistry.erase( name );
    return ( removed == 1 );
}

/**
 * @param name name of the technique to construct
 * @return a pair containing the search result and, if true, the functor to call to build the
 * technique.
 */
std::pair<bool, DefaultTechniqueBuilder> getDefaultTechnique( const std::string& name ) {
    auto search = EngineTechniqueRegistry.find( name );
    if ( search != EngineTechniqueRegistry.end() ) { return { true, search->second }; }
    auto result = std::make_pair( false, [name]( RenderTechnique&, bool ) -> void {
        LOG( logERROR ) << "Undefined default technique for " << name << " !";
    } );
    return result;
}

bool cleanup() {
    EngineTechniqueRegistry.clear();
    return true;
}

} // namespace EngineRenderTechniques

} // namespace Rendering
} // namespace Engine
} // namespace Ra
