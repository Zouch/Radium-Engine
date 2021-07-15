#pragma once
#include <RadiumHeadless/CLIBaseApplication.hpp>
#include <RadiumHeadless/Headless.hpp>
#include <RadiumHeadless/OpenGLContext/OpenGLContext.hpp>

#include <functional>

namespace Ra {
namespace Core {
namespace Asset {
class FileLoaderInterface;
class Camera;
} // namespace Asset
} // namespace Core
namespace Engine {
class RadiumEngine;
namespace Rendering {
class Renderer;
}
} // namespace Engine
} // namespace Ra

/**
 * Base class for radium based cmdline application
 */
class HEADLESS_API CLIViewer : public CLIBaseApplication
{
  public:
    struct ViewerParameters {
        /// Load animation system at startup
        bool m_animationEnable {false};
        /// Size of the image
        std::array<int, 2> m_size {512, 512};
        /// image name prefix
        std::string m_imgPrefix {"frame"};
    };

  private:
    /// Headless OpenGLContext
    OpenGLContext m_glContext;
    /// Instance of the radium engine.
    std::unique_ptr<Ra::Engine::RadiumEngine> m_engine;
    /// Shared instance of the renderer
    std::shared_ptr<Ra::Engine::Rendering::Renderer> m_renderer;

    /// The camera for rendering
    Ra::Core::Asset::Camera* m_camera {nullptr};

    /// The application parameters
    ViewerParameters m_parameters;

    /// is the engine initialized ?
    bool m_engineInitialized {false};

    /// is the window shown ?
    bool m_exposedWindow {false};

  public:
    /// Base constructor.
    CLIViewer();
    /// Base destructor
    virtual ~CLIViewer();

    /**
     * \brief Application initialization method.
     *
     * This method is called by the main function to initialize the app giving its parameters.
     * @param argc number of parameter
     * @param argv array of string representing the parameters
     * @return 0 if the application is correctly initialized or an application dependant error code
     * if something went wrong.
     *
     * Supported command line parameters are --help and --file <filename> from CLIRadiumApplication
     * and the following parameters :
     *   - --size <width x height> : the size of the rendered picture
     *   - --animation : load the Radium animation system
     *   - --env <env_map> : load and use the given environment map.
     */
    int init( int argc, const char* argv[] ) override;

    /**
     * Run the application.
     *  This application compute one image of the scene attached to the engine for the given time
     * stamp.
     *
     * @return 0 if the application was correctly ran or an application dependant error code if
     * something went wrong.
     */
    int run( float timeStep = 1.f / 60.f ) override { return oneFrame( timeStep ); }

    /**
     * Run the application.
     *  This application compute one image of the scene attached to the engine for the given time
     * stamp.
     *
     * @return 0 if the application was correctly ran or an application dependant error code if
     * something went wrong.
     */
    int oneFrame( float timeStep = 1.f / 60.f );

    /**
     * Set the renderer to use to compute an image
     *  The app takes ownership of the give pointer.
     */
    void setRenderer( Ra::Engine::Rendering::Renderer* renderer );

    /**
     * Add a data loader to the engine
     */
    void addDataLoader( Ra::Core::Asset::FileLoaderInterface* loader );

    /** Instantiate the scene.
     * This method load the optionally given datafile from the command line using the available
     * loader (if any) and prepare the scene to be displayed.
     */
    void compileScene();

    /** Register OpenGL addons
     *  The given functor will be called with the OpenGL context bound
     */
    void openGlAddOns( std::function<void()> f );

    /** Activate/deactivate the OpenGL context */
    void bindOpenGLContext( bool on = true );

    /** Define a default camera */
    void setDefaultCamera();

    /** Set the image prefix ("frame" by default) */
    void setImageNamePrefix( std::string s );

    /** get the actual parameter set of the application */
    const ViewerParameters& getViewerParameters() const;

    /** grab the laste rendered frame */
    std::unique_ptr<unsigned char[]> grabFrame( size_t& w, size_t& h ) const;

    /** show/hide the opengl window */
    void showWindow( bool on = true );

    /** swaps the opengl buffer on window */
    void swapBuffers();

    /** When a window is shown, wait for its closing by the user */
    void waitForClose();
};
