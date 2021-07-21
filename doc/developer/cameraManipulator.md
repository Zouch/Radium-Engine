\page develCameraManipulator  Camera manipulation in Radium
[TOC]

The Radium Engine provides some classes that allow to extend the way one might interact with a camera in an
application.

The class `Ra::Gui::CameraManipulator` defines the general interface for camera manipulator. All manipulators have
common properties that allow to switch from one manipulator to one other without disturbing the visual and interactive
behavior of the application.

# Implementing a CameraManipulator

In order to extend the set of `Ra::Gui::CameraManipulator` available in Radium or to develop a dedicated
`Ra::Gui::CameraManipulator` for a `Ra::Gui::Viewer`-based application, programmers are intended to do the following,
demonstrated by the class `FlightCameraManipulator`.

1. Define the class that must inherits from `Ra::Gui::CameraManipulator` and, in order to receive interaction events,
from `Ra::Gui::KeyMappingManageable`. Note that a CameraManipulator is a `Q_OBJECT`

 \snippet Gui/Viewer/FlightCameraManipulator.hpp Declare class

2. Implement the constructors (default, copy). Note that it is also very important to implement a constructor
that will take any `Ra::Gui::CameraManipulator` and will copy the base class before initializing the current
manipulator.

 \snippet Gui/Viewer/FlightCameraManipulator.cpp Constructor


3. Implement the `Ra::Gui::KeyMappingManageable` part of the class. This implies defining the method
`void FlightCameraManipulator::configureKeyMapping_impl()` according to the semantic imposed
by `Ra::Gui::KeyMappingManageable`. It is recommended that, when implementing this keymapping initialisation callback,
a default configuration is defined and saved to the xml keymapping configuration file if this later does not already
contains a configuration. This will allow the users to edit and customize the proposed keymapping configuration.

 \snippet Gui/Viewer/FlightCameraManipulator.cpp Implement KeyMappingManageable

4. Implement the inherited abstract method according to the wanted behavior of the `Ra::Gui::CameraManipulator`

# Extending/Specializing an existing CameraManipulator

The example application `CustomCameraManipulator` Demonstrate how to extend an existing manipulator and specialize
its behavior for a given context.
In the following code, the `Ra::Gui::TrackballCameraManipulator` class is used to define a simple pan and zoom
manipulator, simply by ignoring rotation events:

\snippet ExampleApps/CustomCameraManipulator/main.cpp extend trackball

# Using a CameraManipulator
Using a `Ra::Gui::CameraManipulator` in a `Ra::Gui::Viewer`-based application is quite straightforward.

If one wants to set a first camera manipulator to a viewer
~~~{.cpp}
myViewer->setCameraManipulator(
        new Ra::Gui::FlightCameraManipulator( width, height );
~~~

If one wants to change the manipulator while keeping the actual visual state
~~~{.cpp}
myViewer->setCameraManipulator(
        new Ra::Gui::FlightCameraManipulator( *( m_viewer->getCameraManipulator() ) ) );
~~~
