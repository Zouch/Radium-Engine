#pragma once
#include <array>
#include <string>

struct GLFWwindow;

/**
 * This class defines GLFW based OpenGL Context.
 * There are two use cases of such a context :
 *   - headless rendering application.
 *   - simple window with limited interaction
 */
class OpenGLContext
{
  public:
    /** @defgroup context OpenGL context management
     *  These methods allow to create and manipulate an openGLContext.
     *  Using this function, the openGL context created is an offscreen context with no exposed
     *  window.
     *  @{
     */
    /**
     * Create an offscreen openGl context.
     * The created context has the following properties
     *  - OpenGL version requested : >= 4.1
     *  - OpenGL consstext profile : Core Profile
     * @param size
     */
    explicit OpenGLContext( const std::array<int, 2>& size = {1, 1} );
    /// destructor
    ~OpenGLContext();
    /// make the context active
    void makeCurrent() const;
    /// make the context inactive
    void doneCurrent() const;
    /// Check for validity of the context
    bool isValid() const;

    [[nodiscard]] std::string getInfo() const;
    /** @} */

    /** @defgroup window Window management from an openGL context
     *  These methods allow to display and control a simple window associated with the created
     *  OpenGL Context
     *  @{
     */
    /// Show the window
    void show();
    /// Hide the window
    void hide();
    /// Resize the window
    void resize( const std::array<int, 2>& size );
    /// Swap back/front buffers
    void swapbuffers();
    /// Wait for the user to close the window
    void waitForClose();
    /** @} */

  private:
    GLFWwindow* m_offscreenContext {nullptr};
};
