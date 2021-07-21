\page pluginSystem Plugin System
\todo check and update
[TOC]

The radium engine basically accept plugins to add functionnalities. An example
could be a plug-in to support animation. ImGui, for instance, is implemented as
a plugin.

Current possibilities include:
+ Export QWidgets to the main interface. Will appear in the **toolbox**.
+ Export RenderObjects to be rendered by the application renderer.
+ Communicate with other plugins via the `ComponentMessenger` class.

# Basic

A plugin is integrated by placing the corresponding `.so` into `./Plugins/bin`.

A plugin generally contains the following classes:
+ `FooPlugin`: defines the interface functions needed to be accepted by radium,
and exports the widgets.
+ `FooSystem`: the system will manage and update every component.
+ `FooComponent`: object that has the ability to communicate with other components.

One could also add:
+ `FooTask`: add some tasks (threads) to the `TaskManager`.
+ `FooRenderObject`: define a new render object that will be rendered just like
any others thanks to `RenderObjectManager`.

# Plugin

    // register the plugin to the application
    virtual void registerPlugin( Ra::Engine::RadiumEngine* engine ) override;

    // notify the need for a place in the toolbox
    virtual bool doAddWidget( QString& name ) override;
    virtual QWidget* getWidget() override;

    // notify the need for a new menu entry
    virtual bool doAddMenu() override;
    virtual QMenu* getMenu() override;

# System

...

# Component

Will communicate with others components through Entities.

Please see `Entity`, `EntityManager`, `Component`, `ComponentMessenger`.

# Task

...
