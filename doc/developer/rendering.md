\page develrendering  Rendering pipeline
[TOC]

# Rendering in Radium
Rendering in Radium is managed by the abstract Ra::Engine::Rendering::Renderer class that provides the following main rendering method.

## Main abstract render method

This method is general and cannot be modified, it always does the following

~~~{.cpp}
void Ra::Engine::Rendering::Renderer::render( const Ra::Engine::ViewingParameters& data )
{
  // 0. Save an eventual already bound FBO
  saveExternalFBOInternal();

  // 1. Gather render objects and update them
  renderObjects = m_engine->getRenderObjectManager()->getRenderObjects();
  updateRenderObjectsInternal( data, renderObjects );

  // 2. Feed render queues
  feedRenderQueuesInternal( data, renderObjects );

  // 3. Do picking if needed
  if ( !m_pickingQueries.empty() )
  {
    doPicking();
  }

  // 4. Do the rendering.
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

### 1. Gather render objects and update them
This construct the set of objects that must be drawn for the current frame and update their OpenGL state.

### 2. Feed the render queue
This construct the set of render actions that must be done for the current frame.

### 3. Do picking if needed
If there has been some picking requests since the last frame, `doPicking` is called.
This function just renders all the objects (except _debug_ ones) by drawing them in some color given the ID
of the entity a render object is attached to.

Then, for each picking request done, `glReadPixels` is called at the requested location, and object ID is retrieved.

### 4. Do the rendering
This method does most of the whole rendering work
and outputs one final _render pass_ texture, ready to be post-processed.

### 5. Post-process the rendering result
This method will apply post-processing filters on the computed image.


## Defining a concrete renderer

To define a concrete renderer, the Ra::Engine::Rendering::Renderer class can be inherited to define, at least, the
`renderInternal` method. Note that only two methods can be overridden for the renderer, renderInternal (step 4) and
postProcessInternal (step 5).

A concrete renderer will build its rendering loop, the `renderInternal` method, by decomposing it into passes
configured, for each Ra::engine::RenderObject, using the Ra::Engine::Rendering::RenderTechique class.

A Ra::Engine::Rendering::RenderTechnique consists in a set of named Ra::Engine::Data::ShaderConfiguration that provide the GLSL code used
by each pass to compute the appearance of a Ra::Engine::Rendering::RenderObject and a set of Ra::Engine::Data::ShaderParameterProvider
that provide shader data to be used for each pass.

When rendering, the renderer will loop over all the configured Ra::Engine::Rendering::RenderObject and, for each rendering pass,
will get the OpenGL pipeline configuration and parameters from the Ra::Engine::Rendering::RenderTechnique associated with the
Ra::Engine::Rendering::RenderObject.

The Radium Engine exports a ready to use renderer, Ra::Engine::Rendering::ForwardRenderer, that decomposes the rendering into three
passes as described in the \ref forwardRenderer document.

## Minimal renderer howto
This section explain, through the developement of a simple Rendering, how to configure RenderObjects and RenderTechnique
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
