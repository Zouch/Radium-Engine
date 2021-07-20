#pragma once

#include <Engine/Rendering/Renderer.hpp>

namespace globjects {
class Framebuffer;
}

namespace Ra {
namespace Engine {
namespace Data {
class Texture;
}
namespace Rendering {

/** Default renderer for the Radium Engine
 * This classe implements aforward rendering algorithm with Z-prepass, multipass light accumulation
 * for opaque and transperent objects. Once renderer, the final is composited with Ui, debug and
 * X-ray objects renderings on demand.
 *
 * @see rendering.md for description of the renderer
 */
class RA_ENGINE_API ForwardRenderer : public Renderer
{
  public:
    ForwardRenderer();
    ~ForwardRenderer() override;

    std::string getRendererName() const override { return "Forward Renderer"; }
    bool buildRenderTechnique( RenderObject* ro ) const override;

  protected:
    void initializeInternal() override;
    void resizeInternal() override;

    void updateStepInternal( const Data::ViewingParameters& renderData ) override;

    void postProcessInternal( const Data::ViewingParameters& renderData ) override;
    void renderInternal( const Data::ViewingParameters& renderData ) override;
    void debugInternal( const Data::ViewingParameters& renderData ) override;
    void uiInternal( const Data::ViewingParameters& renderData ) override;

  private:
    void initShaders();
    void initBuffers();

    void updateShadowMaps();

  protected:
    enum RendererTextures {
        RendererTextures_Depth = 0,
        RendererTextures_HDR,
        RendererTextures_Normal,
        RendererTextures_Diffuse,
        RendererTextures_Specular,
        RendererTextures_OITAccum,
        RendererTextures_OITRevealage,
        RendererTextures_Volume,
        RendererTexture_Count
    };

    // Default renderer logic here, no need to be accessed by overriding renderers.
    std::unique_ptr<globjects::Framebuffer> m_fbo;
    std::unique_ptr<globjects::Framebuffer> m_postprocessFbo;
    std::unique_ptr<globjects::Framebuffer> m_oitFbo;
    std::unique_ptr<globjects::Framebuffer> m_uiXrayFbo;
    std::unique_ptr<globjects::Framebuffer> m_volumeFbo;

    std::vector<RenderObjectPtr> m_transparentRenderObjects;
    size_t m_fancyTransparentCount { 0 };

    std::vector<RenderObjectPtr> m_volumetricRenderObjects;
    size_t m_fancyVolumetricCount { 0 };

    size_t m_pingPongSize { 0 };

    std::array<std::unique_ptr<Data::Texture>, RendererTexture_Count> m_textures;

    static const size_t ShadowMapSize { 1024 };
    std::vector<std::shared_ptr<Data::Texture>> m_shadowMaps;
    std::vector<Core::Matrix4> m_lightMatrices;

    using WireMap = std::map<RenderObject*, std::shared_ptr<Data::Displayable>>;
    WireMap m_wireframes;
};

} // namespace Rendering
} // namespace Engine
} // namespace Ra
