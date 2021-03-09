// Include Radium base application and its simple Gui
#include <Gui/BaseApplication.hpp>
#include <Gui/RadiumWindow/SimpleWindowFactory.hpp>

// include the Engine/entity/component interface
#include <Core/Asset/BlinnPhongMaterialData.hpp>
#include <Core/Geometry/MeshPrimitives.hpp>
#include <Engine/Scene/EntityManager.hpp>
#include <Engine/Scene/GeometryComponent.hpp>
#include <Engine/Scene/GeometrySystem.hpp>

#include <QTimer>

int main( int argc, char* argv[] ) {
    //! [Creating the application]
    Ra::Gui::BaseApplication app( argc, argv );
    app.initialize( Ra::Gui::SimpleWindowFactory {} );
    //! [Creating the application]

    //! [Creating the cube]
    auto cube = Ra::Core::Geometry::makeSharpBox( {0.1f, 0.1f, 0.1f} );
    //! [Creating the cube]

    //! [Colorize the Cube]
    cube.addAttrib(
        "in_color",
        Ra::Core::Vector4Array {cube.vertices().size(), Ra::Core::Utils::Color::Green()} );
    //! [Colorize the Cube]

    //! [Creating the Material]
    Ra::Core::Asset::BlinnPhongMaterialData mat( "simpleBlinnPhong" );
    // set the specular color to Cyan
    mat.m_specular    = Ra::Core::Utils::Color::Cyan();
    mat.m_hasSpecular = true;
    // diffuse is taken from mesh attribute,
    mat.m_diffuseFromMesh = true;
    //! [Creating the Material]

    //! [Create the engine entity for the cube]
    auto e = app.m_engine->getEntityManager()->createEntity( "Green cube" );
    //! [Create the engine entity for the cube]

    //! [Create a geometry component with the cube]
    auto c =
        new Ra::Engine::Scene::TriangleMeshComponent( "Cube Mesh", e, std::move( cube ), &mat );
    //! [Create a geometry component with the cube]

    //! [Register the entity/component association to the geometry system ]
    auto geometrySystem = app.m_engine->getSystem( "GeometrySystem" );
    geometrySystem->addComponent( e, c );
    //! [Register the entity/component association to the geometry system ]

    //! [Tell the window that something is to be displayed]
    app.m_mainWindow->postLoadFile( "Cube" );
    //! [Tell the window that something is to be displayed]

    // terminate the app after 4 second (approximatively). Camera can be moved using mouse moves.
    auto close_timer = new QTimer( &app );
    close_timer->setInterval( 4000 );
    QObject::connect( close_timer, &QTimer::timeout, [&app]() { app.appNeedsToQuit(); } );
    close_timer->start();

    return app.exec();
}
