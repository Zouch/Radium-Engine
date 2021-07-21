#pragma once

#include <Engine/RaEngine.hpp>

#include <Core/Asset/MaterialData.hpp>
#include <Engine/Data/Material.hpp>

namespace Ra {
namespace Engine {
namespace Data {
// Predeclare the External/Internal material representation converter
class RawShaderMaterialConverter;
} // namespace Data
} // namespace Engine

namespace Core {
namespace Asset {

/**
 * External shaderMaterial representation
 */
class RA_ENGINE_API RawShaderMaterialData : public MaterialData
{
    /// allow converter to access private members
    friend class Ra::Engine::Data::RawShaderMaterialConverter;

  public:
    /**
     * Construct a shaderMaterialData from shader source and parameter provider
     * @param instanceName The name of the material instance
     * @param shaders The shaders to use for each pipeline stage (only one source per stage is
     * allowed)
     * @param paramProvider The parameter provider for the resulting program
     */
    RawShaderMaterialData(
        const std::string& instanceName,
        const std::vector<std::pair<Ra::Engine::Data::ShaderType, std::string>>& shaders,
        std::shared_ptr<Ra::Engine::Data::ShaderParameterProvider> paramProvider ) :
        MaterialData( instanceName, "Ra::Engine::Data::RawShaderMaterial" ),
        m_shaders { shaders },
        m_paramProvider { std::move( paramProvider ) } {}
    RawShaderMaterialData()                               = delete;
    RawShaderMaterialData( const RawShaderMaterialData& ) = delete;
    ~RawShaderMaterialData()                              = default;

  private:
    std::vector<std::pair<Ra::Engine::Data::ShaderType, std::string>> m_shaders;
    std::shared_ptr<Ra::Engine::Data::ShaderParameterProvider> m_paramProvider;
};

} // namespace Asset
} // namespace Core

namespace Engine {
namespace Data {
/**
 * Converter from an external representation (comming from IO or ...) to internal representation.
 * Such a converter is used when initializing the RenderObjects from external representation of
 * materials
 */
class RA_ENGINE_API RawShaderMaterialConverter final
{
  public:
    RawShaderMaterialConverter()  = default;
    ~RawShaderMaterialConverter() = default;
    inline Material* operator()( const Ra::Core::Asset::MaterialData* toconvert );
};

/**
 * Definition of the Custom Material to compatible with the Radium Management of
 * RenderObjects/GeometryComponents
 */
class RA_ENGINE_API RawShaderMaterial : public Material
{
  public:
    /**
     * Construct a named material from raw shaders
     * @param instanceName The name of the material instance
     * @param shaders The shaders to use for each pipeline stage (only one source per stage is
     * allowed)
     * @param paramProvider The parameter provider for the resulting program
     */
    explicit RawShaderMaterial(
        const std::string& instanceName,
        const std::vector<std::pair<Data::ShaderType, std::string>>& shaders,
        std::shared_ptr<Data::ShaderParameterProvider> paramProvider );

    /**
     * Destructor.
     */
    ~RawShaderMaterial() override;

    /**
     * RawShaderMaterial is not copyable
     */
    RawShaderMaterial( const RawShaderMaterial& ) = delete;

    /**
     * Override of the `getParameters()` method from Material
     * @return
     */
    inline Data::RenderParameters& getParameters() override;

    /**
     * Override of the `const getParameters() const` method from Material
     * @return
     */
    inline const Data::RenderParameters& getParameters() const override;

    /**
     * Register shader configuration and the ForwardRenderer (default) technique for this instance
     */
    void registerDefaultTechnique();

    /**
     * Update the shaders of the material.
     * @note the given shaders array is considered as a remplacement of the current shader array.
     * @param instanceName The name of the material instance
     * @param shaders The shaders to use for each pipeline stage (only one source per stage is
     * allowed)
     * @param paramProvider The parameter provider for the resulting program. If not given, the old
     * provider will be kept.
     */
    void updateShaders( const std::vector<std::pair<Data::ShaderType, std::string>>& shaders,
                        std::shared_ptr<Data::ShaderParameterProvider> paramProvider = nullptr );

    /**
     * Update the openGL state of the material.
     * This state also consist in updating the parameter provider of the Material
     */
    void updateGL() override final;

    /**
     * Register the material in the material library.
     * After registration, the material could be instantiated by any Radium system, renderer,
     * plugin, ...
     */
    static void registerMaterial();

    /**
     * Remove the material from the material library.
     * After removal, the material is no more available, ...
     */
    static void unregisterMaterial();

  private:
    /**
     * Compute the unique key that identify this material in shaders and technique factories
     * @return
     */
    std::string computeKey();

    std::vector<std::pair<Data::ShaderType, std::string>> m_shaders;
    std::shared_ptr<Data::ShaderParameterProvider> m_paramProvider;
    std::string m_materialKey {};
};

} // namespace Data
} // namespace Engine
} // namespace Ra
#include <Engine/Data/RawShaderMaterial.inl>
