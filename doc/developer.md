\page develmanual Developer manual
[TOC]


There are three main options to work and develop using Radium:
1. Write a plugin: full access to the Radium data structures, perfect to implement a new functionality: mesh processing, rendering.
2. Write an application: give total control over the GUI, the camera settings, etc...
3. Contribute to Radium libraries: to improve/extend core components of Radium, add tests.

Radium offers several cmake functions to configure and build your extension. 

- \subpage cmakeutilities
- \subpage develplugin
- \subpage apps


- \subpage develWithRadium
- \subpage develCodingConvention
- \subpage develbuildchain
- \subpage api
- \subpage develoldmanual

\page api API
- \subpage core
- \subpage engine
- \subpage gui
- \subpage io

\page core Core
- \subpage develmeshes
- \subpage develanimation
   
\page engine Engine
- \subpage develrendering
- \subpage develmaterials
- \subpage develLights
- \subpage develpicking
- \subpage develtimeline

\page gui Gui
- \subpage develkeymapping
- \subpage develCameraManipulator

\page io [TODO] IO
\todo write IO doc 

\page apps How to write your own application
\todo write application dev doc, merging ExampleApp and Apps docs.
# Radium Application
Tutorial here: [https://github.com/STORM-IRIT/Radium-AppExample](https://github.com/STORM-IRIT/Radium-AppExample)
