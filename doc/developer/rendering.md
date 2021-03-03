\page develrendering  Rendering pipeline
[TOC]

# Rendering in Radium
Rendering in Radium is managed by the abstract Ra::Engine::Rendering::Renderer class that provides the following main rendering method. 

## Main abstract render method

This method is general and cannot be modified, it always does the following

~~~{.cpp}
void Ra::Engine::Rendering::Renderer::render( const Ra::Engine::Data::ViewingParameters& data )
{
  // 0. Save eventual already bound FBO (e.g. QtOpenGLWidget) and viewport
  saveExternalFBOInternal();

  // 1. Gather render objects
  feedRenderQueuesInternal( data );

  // 2. Update them (from an opengl point of view)
  updateRenderObjectsInternal( data );

  // 3. Do picking if needed
  if ( !m_pickingQueries.empty() )
  {
    doPicking();
  }

  // 4. Do the rendering.
  // 4.1 update Renderer internal state
  updateStepInternal( data );
  // 4.2 render the scene
  renderInternal( data );

  // 5. Post processing
  postProcessInternal( data );

  // 6. Write final texture to framebuffer / backbuffer
  // (depending on wether anything was saved on step 0. or not)
  drawScreenInternal();
}
~~~
The meaning of each rendering steps is described below.

### 0. Save an eventual already bound FBO
In some cases (like with `QtOpenGLWidget`), you do not draw directly on the screen, but you have instead
to feed an already bound FBO. Since the default renderer uses multiple FBOs, the Qt's one must be saved.

_This behaviour cannot be modified._

### 1. Gather render objects
This construct the set of objects that must be drawn for the current frame.

### 2. Update the internal state of RenderObjects
This updates, if needed, the OpenGL internal state of RenderObjects.

### 3. Do picking if needed
If there has been some picking requests since the last frame, `doPicking` is called.
This function just renders all the objects (except _debug_ ones) by drawing them in a Id buffer to collect the ids 
of the entity, the render object, the polygon, ....

Then, for each picking request done, `glReadPixels` is called at the requested location, and object Ids are retrieved.

### 4. Do the rendering
This method does most of the whole rendering work in two steps
#### 4.1 Setup renderer internal state for rendering
This basically populate the render loop, in a renderer dependent way with objects to be drawn.
#### 4.2 Render the scene
This execute the render loop in a renderer dependent way.

### 5. Post-process the rendering result
This method will apply post-processing effects on the computed image.

## Defining a concrete renderer

To define a concrete renderer, the Ra::Engine::Rendering::Renderer class must be derived to define all the method that 
implements renderer specific tasks and are declared a pure virtual methods in Ra::Engine::Rendering::Renderer.

A concrete renderer will build its rendering loop, the `renderInternal` method, by decomposing it into passes 
configured, for each Ra::Engine::Rendering::RenderObject available in the scene, using the 
Ra::Engine::Rendering::RenderTechnique class.

A Ra::Engine::Rendering::RenderTechnique consists in a set of named Ra::Engine::Data::ShaderConfiguration that provide 
the GLSL code used by each pass to compute the appearance of a Ra::Engine::Rendering::RenderObject and a set of 
Ra::Engine::Data::ShaderParameterProvider that provide shader data to be used for each pass.

When rendering, and for each rendering pass, the renderer will first set the global state of the OpenGL pipeline 
corresponding to the pass then loop over all the Ra::Engine::Rendering::RenderObject to be drawn.
For each Ra::Engine::Rendering::RenderObject, the renderObject OpenGL pipeline configuration and parameters are fetched 
from the Ra::Engine::Rendering::RenderTechnique associated with the Ra::Engine::Rendering::RenderObject, and the 
render object is drawn according to the Ra::Engine::Data::ViewingParameters for the current frame.

The Radium Engine exports a ready to use concrete renderer, Ra::Engine::Rendering::ForwardRenderer, used by default by
the Radium-based applications, that decomposes the rendering into three passes as described in the @ref forwardRenderer
document.

## Minimal renderer howto
This section explain, through the development of a simple Renderer, how to configure RenderObjects and RenderTechnique 
to be used by a renderer.

### Developping the Rendering
Help wanted for this section.

### Configuring the RenderObjects and their RenderTechnique
Help wanted for this section.
 


# Radium Engine default rendering information

See [Material management](@ref develmaterials) documentation for a complete description of the 
interactions geometry, material and shader._



# Potential extensions for the renderer
*   Ambient occlusion
*   Shadow mapping
*   Skybox
*   Reflection / refraction
*   Tonemapping (only gamma correction is applied for now)
*   Bloom
*   Motion blur
*   FOV
*   Physically based rendering
