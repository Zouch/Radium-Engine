#pragma once

#include <Core/Utils/Index.hpp>
#include <Engine/RaEngine.hpp>

#include <Engine/Data/ShaderConfiguration.hpp>

#include <functional>
#include <map>
#include <memory>

namespace Ra {
namespace Engine {
namespace Data {
class Material;
class ShaderProgram;
class ShaderParameterProvider;
} // namespace Data

namespace Rendering {

/**
 * Default render pass ids
 */
enum DefaultRenderingPasses : int {
    /**
     * The pass to use as default pass for any renderer.
     * The default pass will be used to draw debug or ui objects.
     * When a renderer define its specific passes the pass at index 0 will be used to draw debug or
     * ui objects.
     */
    DEFAULT_PASS = 0,
    /**
     * The main pass of Radium default renderer.
     * This pass render, with lighting, the fully opaque fragments
     */
    LIGHTING_OPAQUE = DEFAULT_PASS,
    /**
     * The prepass of Radium default renderer.
     * This pass fill in the depth buffer and initialize feature buffers
     */
    Z_PREPASS,
    /**
     * The transparency pass of Radium default renderer.
     * This pass render, with lighting, the transparent fragments using LIT-OIT algorithm
     * @see Ra::Engine::ForwardRenderer documentation
     */
    LIGHTING_TRANSPARENT,
    /**
     * The volumetric pass of Radium default renderer.
     * This pass render, with lighting, an object with volumetric material (density matrix)
     * @see Ra::Engine::ForwardRenderer documentation
     */
    LIGHTING_VOLUMETRIC

}; // enum DefaultRenderingPasses

/**
 * Set of shaders to be used by the renderer to render objects with associated data.
 * @see Render technique and materials section in the Material management in the Radium Engine
 * documentation
 */
class RA_ENGINE_API RenderTechnique final
{
  public:
    RenderTechnique();

    /// copy constuctor, only active pass and active pass parameters are copied.
    RenderTechnique( const RenderTechnique& );
    ~RenderTechnique();

    /**
     * Set the shader configuration for the given pass.
     * m_numActivePass is updated to be the max of all indices that are set this way.
     * @param newConfig The pass shader configuration
     * @param pass The index of the pass to set
     */
    void setConfiguration( const Data::ShaderConfiguration& newConfig,
                           Core::Utils::Index pass = DefaultRenderingPasses::LIGHTING_OPAQUE );

    /**
     * Test if the given pass was configured
     * @param pass The index of the pass
     * @return true if the pass was configured
     */
    inline bool
    hasConfiguration( Core::Utils::Index pass = DefaultRenderingPasses::LIGHTING_OPAQUE ) const;

    /**
     * Mark the given configured.
     * @param pass The index of the pass
     */
    inline void
    setConfiguration( Core::Utils::Index pass = DefaultRenderingPasses::LIGHTING_OPAQUE );

    /**
     * Mark the given not configured.
     * @param pass The index of the pass
     */
    inline void
    clearConfiguration( Core::Utils::Index pass = DefaultRenderingPasses::LIGHTING_OPAQUE );

    /**
     * Get the configuration of the given pass
     * @param pass The index of the pass
     * @return The pass shader configuration
     */
    inline const Data::ShaderConfiguration&
    getConfiguration( Core::Utils::Index pass = DefaultRenderingPasses::LIGHTING_OPAQUE ) const;

    /**
     * Get the Data::ShaderProgram associated with the pass
     * @param pass The index of the pass
     * @return  The pass shader program if the pass is configured, nullptr otherwise.
     */
    const Data::ShaderProgram*
    getShader( Core::Utils::Index pass = DefaultRenderingPasses::LIGHTING_OPAQUE ) const;

    /**
     * Set the shader parameter provider for the given pass.
     * @note all passes must be configured first using RenderTechnique::setConfiguration() before
     * setting the parameter provider for each pass.
     * @param provider The rendering parameter provider
     * @param pass The index of the pass to set. If this -1, all active passes will share the same
     * provider.
     */
    void setParametersProvider( std::shared_ptr<Data::ShaderParameterProvider> provider,
                                Core::Utils::Index pass = Core::Utils::Index( -1 ) );

    /**
     * Get the Data::ShaderProgram parameters associated with the pass
     * @param pass The index of the pass
     * @return  The pass Data::ShaderParameterProvider if the pass is configured, nullptr otherwise.
     */
    const Data::ShaderParameterProvider* getParametersProvider(
        Core::Utils::Index pass = DefaultRenderingPasses::LIGHTING_OPAQUE ) const;

    /**
     * Update all openGL status of active passes.
     */
    void updateGL();

    /**
     * Test if the given pass is dirty (openGL state not updated)
     * @param pass The index of the pass
     * @return Tru if pass must be updated.
     */
    inline bool isDirty( Core::Utils::Index pass = DefaultRenderingPasses::LIGHTING_OPAQUE ) const;

    /**
     * Set the given pass dirty (openGL state not updated)
     * @param pass The index of the pass
     */
    inline void setDirty( Core::Utils::Index pass = DefaultRenderingPasses::LIGHTING_OPAQUE );

    /**
     * Clear the dirty bit for the given pass (openGL state updated)
     * @param pass The index of the pass
     */
    inline void clearDirty( Core::Utils::Index pass = DefaultRenderingPasses::LIGHTING_OPAQUE );

    /**
     * Add properties (several #define in the shader) for the given pass
     * @param props the properties list, only strings without the #define
     * @param pass the pass. If left by default, all active passes will get the properties
     */
    void addPassProperties( const std::list<std::string>& props,
                            Core::Utils::Index pass = Core::Utils::Index {} );
    /**
     * Creates a default technique based on the ForwarRenderer sementic.
     *  pass 1 --> Z_PREPASS
     *  pass 0 --> LIGHTING_OPAQUE
     *  pass 2 --> LIGHTING_TRANSPARENT
     * @return the default technique
     */
    static RenderTechnique createDefaultRenderTechnique();

  private:
    /// Maximum number of passses in the technique
    static constexpr int s_maxNbPasses { 32 };

    /// A pass configuration is a pair of Data::ShaderConfiguration and a Data::ShaderProgram
    using PassConfiguration = std::pair<Data::ShaderConfiguration, const Data::ShaderProgram*>;
    /// A Configuration set is a fixed-sized array of m_maxNbPasses (32) passes
    using ConfigurationSet = std::array<PassConfiguration, s_maxNbPasses>;
    /// Rendering parameters associated to the configurationSet
    using PassesParameters =
        std::array<std::shared_ptr<Data::ShaderParameterProvider>, s_maxNbPasses>;

    /// Configuration for all active passes of the technique
    ConfigurationSet m_activePasses;
    /// Parameters for the passes
    PassesParameters m_passesParameters;

    /// Number of active passes in the technique
    Core::Utils::Index m_numActivePass;

    // Change this if there is more than 32 configurations
    /// Dirty bits representing the compiled state of passes. bit i is 1 if pass i is dirty
    unsigned int m_dirtyBits { 0xFFFFFFFF };
    /// Bit arrays indicating which pass is active. Bit i is 1 if pass i is active
    unsigned int m_setPasses { 0x00000000 };
};

///////////////////////////////////////////////
////        Radium defined technique        ///
///////////////////////////////////////////////
namespace EngineRenderTechniques {

/// A default technique builder is a function that will fill the given RenderTechnique with the
/// default configurations associated with a material
/// This function will take as param the render technique to fill and a boolean
/// set to true if the material might be transparent under some parameter values
/// \param rt Render technique to fill
/// \param isTransparent true fi material might be transparent
using DefaultTechniqueBuilder = std::function<void( RenderTechnique& rt, bool isTransparent )>;

/** register a new default builder for a material class
 *  @return true if builder added, false else (e.g, a builder with the same name already exists)
 */
RA_ENGINE_API bool registerDefaultTechnique( const std::string& name,
                                             DefaultTechniqueBuilder builder );

/** remove a default builder
 *  @return true if builder removed, false else (e.g, a builder with the same name does't exists)
 */
RA_ENGINE_API bool removeDefaultTechnique( const std::string& name );

/**
 * @param name name of the technique to construct
 * @return a pair containing the search result and, if true, the functor to call to build the
 * technique.
 */
RA_ENGINE_API std::pair<bool, DefaultTechniqueBuilder>
getDefaultTechnique( const std::string& name );

RA_ENGINE_API bool cleanup();
} // namespace EngineRenderTechniques

} // namespace Rendering
} // namespace Engine
} // namespace Ra

#include <Engine/Rendering/RenderTechnique.inl>
