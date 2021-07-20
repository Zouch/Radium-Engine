#pragma once

#include <Engine/RaEngine.hpp>

#include <array>
#include <chrono>
#include <memory>
#include <mutex>
#include <vector>

#include <Core/Types.hpp>
#include <Core/Utils/Color.hpp>
#include <Core/Utils/Timer.hpp>
#include <Engine/Data/DisplayableObject.hpp>

namespace globjects {
class Framebuffer;
}

namespace Ra {
namespace Asset {
class FileData;
}

namespace Engine {

namespace Data {
class Texture;
struct ViewingParameters;
class ShaderProgram;
class ShaderProgramManager;
class TextureManager;
} // namespace Data

namespace Scene {
class Light;
class LightManager;
} // namespace Scene

namespace Rendering {
class RenderObject;
class RenderObjectManager;

/**
 * Abstract renderer for the engine.
 * @see Radium Engine default rendering informations
 */
class RA_ENGINE_API Renderer
{
  protected:
    using RenderObjectPtr = std::shared_ptr<RenderObject>;

  public:
    /**
     * Instrumentation structure that allow to extract timings from the rendering
     */
    struct TimerData {
        Core::Utils::TimePoint renderStart;
        Core::Utils::TimePoint updateEnd;
        Core::Utils::TimePoint feedRenderQueuesEnd;
        Core::Utils::TimePoint mainRenderEnd;
        Core::Utils::TimePoint postProcessEnd;
        Core::Utils::TimePoint renderEnd;
    };

    /**
     * Picking mode
     */
    enum PickingMode {
        RO = 0,     ///< Pick a mesh
        VERTEX,     ///< Pick a vertex of a mesh
        EDGE,       ///< Pick an edge of a mesh
        TRIANGLE,   ///< Pick a triangle of a mesh
        C_VERTEX,   ///< Picks all vertices of a mesh within a screen space circle
        C_EDGE,     ///< Picks all edges of a mesh within a screen space circle
        C_TRIANGLE, ///< Picks all triangles of a mesh within a screen space circle
        NONE,       ///< Do not pick ;)
    };

    /**
     * Picking purpose
     * Used to identify what picking operation is realized
     */
    enum PickingPurpose { SELECTION = 0, MANIPULATION };

    /**
     * Picking query
     */
    struct PickingQuery {
        Core::Vector2 m_screenCoords;
        PickingPurpose m_purpose;
        PickingMode m_mode;
    };

    /**
     * \brief Result of a PickingQuery
     *
     * For each selected pixel, store the following indices: `<elementIdx, vertexIdx, edgeIdx>`
     *  - `elementIdx`: Index of the selected face or edge (for mesh and line mesh respectively).
     * Set to -1 for point clouds.
     *  - `vertexIdx`: On meshes/lines: index of the closest vertex on the selected face/edge. On
     *  point-clouds: index of the selected vertex
     *  - `edgeIdx`: On meshes/lines: index of the closest edge on the selected face/edge. In
     * meshes, `edgeIdx` is defined using the index of the opposite vertex. Set to -1 for point
     * clouds.
     *
     *  For performance reasons, there is no duplicate check when filling PickingResult. When
     *  required, call removeDuplicatedIndices() before processing indices().
     *
     *  \see getPickingResults
     */
    class PickingResult
    {
      public:
        /// Read access to the collected ids
        inline const std::vector<std::tuple<int, int, int>>& getIndices() const;

        /// Add new ids to the result
        inline void addIndex( const std::tuple<int, int, int>& idx );

        /// Reserve size of ids container
        inline void reserve( size_t s );

        /// Remove duplicates in m_indices
        inline void removeDuplicatedIndices() const;

        /// Reset query to default
        inline void clear();

        inline void setRoIdx( Core::Utils::Index idx );
        inline Core::Utils::Index getRoIdx() const;
        inline void setMode( PickingMode mode );
        inline PickingMode getMode() const;
        //

        /// return depth read during picking query (only done during doPickinNow())
        ///\todo fixup for all picking query, \todo improve picking.
        inline Scalar getDepth() const;
        inline void setDepth( Scalar depth );

      private:
        /// Picking mode of the query
        PickingMode m_mode { Engine::Rendering::Renderer::RO };
        /// Idx of the picked RO
        Core::Utils::Index m_roIdx { Core::Utils::Index::Invalid() };

        /// Query result, stored as: [vertexId, elementId, edgeId]
        /// \see removeDuplicatedIndices()
        ///
        /// \note Set as mutable to be able to call removeDuplicatedIndices() in const context.
        mutable std::vector<std::tuple<int, int, int>> m_indices;
        Scalar m_depth;
    };

  public:
    /** Abstract rendere constructor
     *
     * could be called without openGL context.
     * Call initialize once the openGL rendering context is available before using the renderer
     */
    Renderer();

    virtual ~Renderer();

    // -=-=-=-=-=-=-=-=- FINAL -=-=-=-=-=-=-=-=- //

    /**
     * Extract the timings from las render
     */
    inline const TimerData& getTimerData() const;

    /**
     * Get the currently displayed texture
     */
    inline Data::Texture* getDisplayTexture() const;

    // Lock the renderer (for MT access)
    /**
     * Lock rendering. Usefull if there is multithread update of the rendering data
     */
    inline void lockRendering();

    /**
     * Unlock the rendering.
     */
    inline void unlockRendering();

    /**
     * Toggle the fill/wireframe rendering mode
     */
    inline void toggleWireframe();

    /**
     * set the fill/wireframe rendering mode
     * @param enabled true if rendering mode must be wireframe, false for fill render mode
     */
    inline void enableWireframe( bool enabled );

    /**
     * Toggle debug rendering
     */
    inline void toggleDrawDebug();

    /**
     * Set the debug rendering mode
     * @param enabled true if rendering mode must include debug objects, false else
     */
    inline void enableDebugDraw( bool enabled );

    /**
     * set the post-process mode
     * @param enabled true if post processing must bve applied before display.
     */
    inline void enablePostProcess( bool enabled );

    /**
     * @brief Tell the renderer it needs to render.
     * This method does the following steps :
     * <ol>
     *   <li>call @see updateRenderObjectsInternal method</li>
     *   <li>call @see renderInternal method</li>
     *   <li>call @see postProcessInternal method</li>
     *   <li>render the final texture in the right framebuffer*</li>
     * </ol>
     *
     * @param renderData The basic data needed for the rendering :
     * Time elapsed since last frame, camera view matrix, camera projection matrix.
     *
     * @note * What "render in the right buffer" means, is that, for example,
     * when using QOpenGLWidget, Qt binds its own framebuffer before calling
     * updateGL() method.
     * So, render() takes that into account by saving an eventual bound
     * framebuffer, and restores it before drawing the last final texture.
     * If no framebuffer was bound, it draws into GL_BACK.
     */
    void render( const Data::ViewingParameters& renderData );

    /**
     * @brief Initialize renderer
     */
    void initialize( uint width, uint height );

    /**
     * @brief Resize the viewport and all the screen textures, fbos.
     * This function must be overrided as soon as some FBO or screensized
     * texture is used (since the default implementation just resizes its
     * own fbos / textures)
     *
     * @param width The new viewport width
     * @param height The new viewport height
     */
    void resize( uint width, uint height );

    /**
     * Add a new picking query for the next rendering
     * @param query
     */
    inline void addPickingRequest( const PickingQuery& query );

    /**
     * Get the vector of picking results.
     * Results in the returned vector correspond to queries in the return vector by the function
     * getPickingQueries().
     * @return Queries results
     */
    inline const std::vector<PickingResult>& getPickingResults() const;

    /**
     * Get the vector of picking queries.
     * Queries in the returned vector correspond to results in the return vector by the function
     * getPickingResults().
     * @return Queries results
     */
    inline const std::vector<PickingQuery>& getPickingQueries() const;

    inline void setMousePosition( const Core::Vector2& pos );

    inline void setBrushRadius( Scalar brushRadius );

    /// Tell if the renderer has an usable light.
    bool hasLight() const;

    /// Update the background color (does not trigger a redraw)
    inline void setBackgroundColor( const Core::Utils::Color& color );

    inline const Core::Utils::Color& getBackgroundColor() const;

    // -=-=-=-=-=-=-=-=- VIRTUAL -=-=-=-=-=-=-=-=- //
    /** Add a light to the renderer.
     * may be overridden to filter the light or to specialize the way ligths are added to the
     * renderer ...
     * @param light
     */
    virtual void addLight( const Scene::Light* light );

    /**
     * Reload, recompile and relink all shaders and programmed internally used by the renderer.
     */
    virtual void reloadShaders();

    // TODO:    For now the drawn texture takes the whole viewport,
    //          maybe it could be great if we had a way to switch between
    //          the current "fullscreen" debug mode, and some kind of
    //          "windowed" mode (that would show the debugged texture in
    //          its own viewport, without hiding the final texture.)
    /**
     * @brief Change the texture that is displayed on screen.
     * Set m_displayedIsDepth to true if depth linearization is wanted
     *
     * @param texName The texture to display.
     */
    virtual void displayTexture( const std::string& texName );

    /**
     * @brief Return the names of renderer available textures
     * @return A vector of strings, containing the name of the different textures
     */
    virtual std::vector<std::string> getAvailableTextures() const;

    /**
     * @brief Get the name of the renderer, e.g to be displayed in the UI
     * @return
     */
    virtual std::string getRendererName() const = 0;

    /**
     * Define, for the given render object, the render technique cooresponding to the renderer.
     * @param ro the render object to modofy
     * @return True if the renderTechnique was defined.
     */
    virtual bool buildRenderTechnique( RenderObject* ro ) const = 0;

    /**
     *  Loops over all available renderobjects and, build the associated render technique using
     *  buildRenderTechnique(RenderObject *ro)
     * @return the number of render objects initialized
     */
    int buildAllRenderTechniques() const;

    /**
     * get the content of the current frame
     * @param w width of the region to grab
     * @param h heigth oif the region to grab
     * @return the pixel array
     */
    virtual std::unique_ptr<uchar[]> grabFrame( size_t& w, size_t& h ) const;

    /**
     * Read the depth value from m_pickingFbo depth buffer.
     * Need to be overridden to take rendering fbo into account in your own renderer.
     * \return the value of the depth buffer under pixel (x,y) return depth value is in "screen
     * space" [0,1] \see
     * https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glReadPixels.xhtml
     */
    PickingResult doPickingNow( const PickingQuery& query,
                                const Data::ViewingParameters& renderData );

  protected:
    /**
     * @brief initializeInternal
     * Initialize the renderer dependant resources.
     */
    virtual void initializeInternal() = 0;

    /**
     * resize the renderer dependent resources
     */
    virtual void resizeInternal() = 0;

    /**
     * Update the renderer dependent resources for the next frame
     * @param renderData
     */
    virtual void updateStepInternal( const Data::ViewingParameters& renderData ) = 0;

    /**
     * @brief All the scene rendering magics basically happens here.
     *
     * @param renderData The basic data needed for the rendering :
     * Time elapsed since last frame, camera view matrix, camera projection matrix.
     */
    virtual void renderInternal( const Data::ViewingParameters& renderData ) = 0;

    /**
     * @brief Do all post processing stuff. If you override this method,
     * be careful to fill @see m_fancyTexture since it is the texture that
     * will be displayed at the very end of the @see render method.
     *
     * @param renderData The basic data needed for the rendering :
     * Time elapsed since last frame, camera view matrix, camera projection matrix.
     */
    virtual void postProcessInternal( const Data::ViewingParameters& renderData ) = 0;

    /**
     * @brief Add the debug layer with useful informations
     */
    virtual void
    debugInternal( const Data::ViewingParameters& renderData ) = 0; // is viewingParameters useful ?

    /**
     * @brief Draw the UI data
     */
    virtual void uiInternal( const Data::ViewingParameters& renderData ) = 0; // idem ?

  private:
    // 0.
    void saveExternalFBOInternal();
    void restoreExternalFBOInternal();

    // 1.
    void feedRenderQueuesInternal( const Data::ViewingParameters& renderData );

    // 2.0
    void updateRenderObjectsInternal( const Data::ViewingParameters& renderData );

    // 3.
    void splitRenderQueuesForPicking( const Data::ViewingParameters& renderData );
    void splitRQ( const std::vector<RenderObjectPtr>& renderQueue,
                  std::array<std::vector<RenderObjectPtr>, 4>& renderQueuePicking );
    void renderForPicking( const Data::ViewingParameters& renderData,
                           const std::array<const Data::ShaderProgram*, 4>& pickingShaders,
                           const std::array<std::vector<RenderObjectPtr>, 4>& renderQueuePicking );

    void doPicking( const Data::ViewingParameters& renderData );

    // 6.
    void drawScreenInternal();

    // 7.
    void notifyRenderObjectsRenderingInternal();

  protected:
    uint m_width { 0 };
    uint m_height { 0 };

    /// This raw pointer is used as an alias to a std::unique_ptr own by the engine.
    /// It is guaranteed that the lifetime of the engine is longer than the lifetime of a Renderer.
    /// \see https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Ri-raw
    /// \see https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rr-ptr
    Data::ShaderProgramManager* m_shaderProgramManager { nullptr };

    /// This raw pointer is used as an alias to a std::unique_ptr own by the engine.
    /// It is guaranteed that the lifetime of the engine is longer than the lifetime of a Renderer.
    /// \see https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Ri-raw
    /// \see https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rr-ptr
    RenderObjectManager* m_renderObjectManager { nullptr };

    //                It would make more sense if we are able to show the
    //                debugged texture in its own viewport.
    /**
     * @brief The texture that will be displayed on screen. If no call to
     * @see debugTexture has been done, this is just a pointer to
     * @see m_fancyTexture.
     */
    Data::Texture* m_displayedTexture { nullptr };

    /// A renderer could define several LightManager (for instance, one for point light, one other
    /// for infinite light ...)
    std::vector<Ra::Engine::Scene::LightManager*> m_lightmanagers;

    bool m_renderQueuesUpToDate { false };

    std::vector<RenderObjectPtr> m_fancyRenderObjects;
    std::vector<RenderObjectPtr> m_debugRenderObjects;
    std::vector<RenderObjectPtr> m_xrayRenderObjects;
    std::vector<RenderObjectPtr> m_uiRenderObjects;

    // Simple quad mesh, used to render the final image
    std::unique_ptr<Data::Displayable> m_quadMesh;

    bool m_drawDebug { true };          // Should we render debug stuff ?
    bool m_wireframe { false };         // Are we rendering in "real" wireframe mode
    bool m_postProcessEnabled { true }; // Should we do post processing ?

    // derived class could use the already created textures
    /// Depth texture : might be attached to the main framebuffer
    std::unique_ptr<Data::Texture> m_depthTexture;
    /// Final color texture : might be attached to the main framebuffer
    std::unique_ptr<Data::Texture> m_fancyTexture;
    /// Textures exposed in the texture section box to be displayed.
    std::map<std::string, Data::Texture*> m_secondaryTextures;

  private:
    // Renderer timings data
    TimerData m_timerData;

    std::mutex m_renderMutex;

    // PICKING STUFF
    Ra::Core::Vector2 m_mousePosition;
    float m_brushRadius { 0 };
    std::unique_ptr<globjects::Framebuffer> m_pickingFbo;
    std::unique_ptr<Data::Texture> m_pickingTexture;

    static const int NoPickingRenderMode = Data::Displayable::PickingRenderMode::NO_PICKING;
    std::array<std::vector<RenderObjectPtr>, NoPickingRenderMode> m_fancyRenderObjectsPicking;
    std::array<std::vector<RenderObjectPtr>, NoPickingRenderMode> m_debugRenderObjectsPicking;
    std::array<std::vector<RenderObjectPtr>, NoPickingRenderMode> m_xrayRenderObjectsPicking;
    std::array<std::vector<RenderObjectPtr>, NoPickingRenderMode> m_uiRenderObjectsPicking;
    std::array<const Data::ShaderProgram*, NoPickingRenderMode> m_pickingShaders;

    std::vector<PickingQuery> m_pickingQueries;
    std::vector<PickingQuery> m_lastFramePickingQueries;
    std::vector<PickingResult> m_pickingResults;

    Core::Utils::Color m_backgroundColor { Core::Utils::Color::Grey( 0.0392_ra, 0_ra ) };
    void preparePicking( const Data::ViewingParameters& renderData );

    bool m_initialized { false };
};

} // namespace Rendering
} // namespace Engine
} // namespace Ra

#include <Engine/Rendering/Renderer.inl>
