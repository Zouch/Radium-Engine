// include the Engine/entity/component interface
#include <Core/Geometry/MeshPrimitives.hpp>
#include <Engine/Scene/EntityManager.hpp>
#include <Engine/Scene/GeometryComponent.hpp>
#include <Engine/Scene/GeometrySystem.hpp>
// Include Radium base application and its simple Gui
#include <Gui/BaseApplication.hpp>
#include <Gui/RadiumWindow/SimpleWindowFactory.hpp>
#include <Gui/Viewer/TrackballCameraManipulator.hpp>
#include <Gui/Viewer/Viewer.hpp>

#include <QTimer>

//! [extend trackball]
// Add simple Camera Manipulator with only translation and zoom
class CameraManipulator2D : public Ra::Gui::TrackballCameraManipulator
{
  public:
    /// Default constructor
    inline CameraManipulator2D() : Ra::Gui::TrackballCameraManipulator() {}

    /// Copy constructor used when switching camera manipulator
    /// Requires that m_target is on the line of sight of the camera.
    inline explicit CameraManipulator2D( const CameraManipulator& other ) :
        Ra::Gui::TrackballCameraManipulator( other ) {}

    inline bool handleMousePressEvent( QMouseEvent* event,
                                       const Qt::MouseButtons& buttons,
                                       const Qt::KeyboardModifiers& modifiers,
                                       int key ) {
        m_lastMouseX = event->pos().x();
        m_lastMouseY = event->pos().y();

        m_currentAction = Ra::Gui::KeyMappingManager::getInstance()->getAction(
            Ra::Gui::TrackballCameraManipulator::KeyMapping::getContext(),
            buttons,
            modifiers,
            key,
            false );

        // ignore rotate
        if ( m_currentAction == TRACKBALLCAMERA_ROTATE )
        { m_currentAction = Ra::Core::Utils::Index::Invalid(); }

        return m_currentAction.isValid();
    }
};
//! [extend trackball]

int main( int argc, char* argv[] ) {
    //! [Creating the application]
    Ra::Gui::BaseApplication app( argc, argv );
    app.initialize( Ra::Gui::SimpleWindowFactory {} );
    //! [Creating the application]

    //! [Creating the cube]
    auto cube = Ra::Core::Geometry::makeSharpBox( { 0.1f, 0.1f, 0.1f } );
    //! [Creating the cube]

    //! [Colorize the Cube]
    cube.addAttrib(
        "in_color",
        Ra::Core::Vector4Array { cube.vertices().size(), Ra::Core::Utils::Color::Green() } );
    //! [Colorize the Cube]

    //! [Create the engine entity for the cube]
    auto e = app.m_engine->getEntityManager()->createEntity( "Green cube" );
    //! [Create the engine entity for the cube]

    //! [Create a geometry component with the cube]
    auto c =
        new Ra::Engine::Scene::TriangleMeshComponent( "Cube Mesh", e, std::move( cube ), nullptr );
    //! [Create a geometry component with the cube]

    //! [Register the entity/component association to the geometry system ]
    auto geometrySystem = app.m_engine->getSystem( "GeometrySystem" );
    geometrySystem->addComponent( e, c );
    //! [Register the entity/component association to the geometry system ]

    //! [Tell the window that something is to be displayed]
    app.m_mainWindow->prepareDisplay();
    //! [Tell the window that something is to be displayed]

    //! [Install new manipulator]
    auto viewer = app.m_mainWindow->getViewer();
    viewer->setCameraManipulator( new CameraManipulator2D( *( viewer->getCameraManipulator() ) ) );
    //! [Install new manipulator]

    // terminate the app after 4 second (approximatively). Camera can be moved using mouse moves.
    auto close_timer = new QTimer( &app );
    close_timer->setInterval( 4000 );
    QObject::connect( close_timer, &QTimer::timeout, [&app]() { app.appNeedsToQuit(); } );
    close_timer->start();

    return app.exec();
}
