\page forwardRenderer Forward Render
[TOC]

The main renderer of Radium, implemented in the class Ra::Engine::Rendering::ForwardRenderer implements a Z-pre-pass forward
rendering loop.
Even if the material association to a drawable object, realized by the so called Ra::Engine::Rendering::RenderTechnique is tightly
coupled with the main rendering loop, Plugins might define new renderers and interact differently with shader and
materials properties (see chapter on [Materials](@ref develmaterials) ).

Here is a summary of the different passes of the Ra::Engine::Rendering::ForwardRenderer

# 1. Depth, ambient color and "deferred info" pass
This pass
*   only concerns opaque objects AND opaque fragment on transparent objects.
It is mainly used to fill in the Z-buffer, allowing to activate early z-test for next passes since the depth
buffer is already filled,

*   initialize the color buffer by computing e.g. the ambient color for each object,

*   generate several pictures of the scene allowing to implement composition effects later :
    *   saves the world-space normals for each object,
    *   saves the "Diffuse" aspect of the object,
    *   saves the "Specular" aspect of the object.


In this pass, each Ra::Engine::Rendering::RenderObject is drawn with the Ra::Engine::Rendering::Z_PREPASS argument so that
the corresponding shader (if it exists) will be activated before draw call.
(``ro->render( renderParameters, viewingParameters, DefaultRenderingPasses::Z_PREPASS );``)

Note that the  shader associated to the Ra::Engine::Rendering::Z_PREPASS pass must draw only fully opaque fragments.
Fully transparent ones (rejected by a masking information such as mask texture) and blend-able ones
(those with an opacity factor alpha les than one) must be discarded.

# 2. Lighting pass
This pass is a classic forward lighting pass that accumulates the color of each light source.
Before this pass, blending (one, one) is enabled and it does
~~~text
for each light do
  Get light parameters
  for each object
    bind shader
    bind material & light
    render mesh
  done
done
~~~

In this pass, each RenderObject   drawn with the Ra::Engine::Rendering::LIGHTING_OPAQUE argument so that the
corresponding shader will be activated before draw call.
(``ro->render( renderParameters, viewingParameters, DefaultRenderingPasses::LIGHTING_OPAQUE );``)

Note that the  shader associated to  the Ra::Engine::Rendering::LIGHTING_OPAQUE pass must lit and draw only fully opaque fragments.
Fully transparent ones (rejected by a masking information such as mask texture) and blend-able ones
(those with an opacity factor alpha les than one) must be discarded.

# 3. Ordered independent transparency
Rendering transparent objects in Radium is done according to the algorithm described in
*   Weighted Blended Order-Independent Transparency,
    Morgan McGuire, Louis Bavoil - NVIDIA,
    Journal of Computer Graphics Techniques (JCGT), vol. 2, no. 2, 122-141, 2013,
    <http://jcgt.org/published/0002/02/09/>

This pass contains one scene rendering pass and one composition pass.

*   The scene rendering pass must compute both the accumulation buffer and the coverage buffer as described in the paper
(see the Material documentation for example of shader.). It is realized the same way than the lighting pass but only
fragments that are transparent must be lit and drawn.

*   The composition pass then adds to the color buffer the resulting blended color.

# 4. Post-process the whole _render pass_
This pass takes the color buffer, representing colors in linear RGB space) and apply gamma correction to the image

# 5. Debug and UI render objects
Those objects are drawn with their own shader, and without lighting.
They do not write in the depth map but they have different depth testing behaviour :
*   func is `LESS` for debug objects (drawn only if visible)
*   func is `ALWAYS` for UI objects (drawn in front of everything else)

# 6. Write final texture to framebuffer / backbuffer
This method is just responsible for displaying the final stuff on screen or on the saved FBO.
