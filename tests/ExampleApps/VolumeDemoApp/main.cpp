// Include Radium base application and its simple Gui
#include <Gui/BaseApplication.hpp>
#include <Gui/RadiumWindow/SimpleWindowFactory.hpp>

// include the Engine/entity/component interface
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

    //! [Creating the Volume]
    auto density = new Ra::Core::Geometry::VolumeGrid();
    int sx       = 100;
    int sy       = 100;
    int sz       = 75;

    density->setSize( Ra::Core::Vector3i( sx, sy, sz ) );
    auto& data = density->data();
    // Radial density with value 1 at center
    auto densityField = [sx, sy, sz]( int i, int j, int k, int r ) {
        i -= sx / 2;
        j -= sy / 2;
        k -= sz / 2;
        Scalar d = ( std::sqrt( i * i + j * j + k * k ) - r ) / r;
        if ( d > 0 ) { return 0_ra; }
        else
        { return -d; }
    };
    for ( int i = 0; i < sx; ++i )
    {
        for ( int j = 0; j < sy; ++j )
        {
            for ( int k = 0; k < sz; ++k )
            { data[i + sx * ( j + sy * k )] = densityField( i, j, k, 50 ); }
        }
    }
    auto volume     = new Ra::Core::Asset::VolumeData( "DemoVolume" );
    volume->volume  = density;

    // from measured media at https://github.com/mmp/pbrt-v3/blob/master/src/core/medium.cpp
    // unit :  mm-1
/*
    // Skin2
    volume->sigma_s = Ra::Core::Utils::Color{ 1.09_ra, 1.59_ra, 1.79_ra};
    volume->sigma_a = Ra::Core::Utils::Color{ 0.013_ra, 0.070_ra, 0.145_ra};
*/
    /*
    // Skin1
    volume->sigma_s = Ra::Core::Utils::Color{ 0.74_ra, 0.88_ra, 1.01_ra};
    volume->sigma_a = Ra::Core::Utils::Color{ 0.032_ra, 0.17_ra, 0.48_ra};
    */
    /*
    // Ketchup
    volume->sigma_s = Ra::Core::Utils::Color{ 0.18_ra, 0.07_ra, 0.03_ra};
    volume->sigma_a = Ra::Core::Utils::Color{ 0.061_ra, 0.97_ra, 1.45_ra};
    */
    /*
    // Chardonnay
    volume->sigma_s = Ra::Core::Utils::Color{ 1.7982e-05_ra, 1.3758e-05_ra, 1.2023e-05_ra };
    volume->sigma_a = Ra::Core::Utils::Color{ 0.010782_ra, 0.011855_ra, 0.023997_ra };
    */
/*
    // As the scene unit is meter, multiply by 1000
    volume->sigma_s *= 1000;
    volume->sigma_a *= 1000;
*/

    // keep default absorption/diffusion  coeffficients : default to Air

    Scalar maxDim   = std::max( std::max( sx, sy ), sz );
    Ra::Core::Vector3 p0( 0, 0, 0 );
    Ra::Core::Vector3 p1( sx, sy, sz );
    volume->boundingBox    = Ra::Core::Aabb( p0, p1 );
    volume->densityToModel = Ra::Core::Transform::Identity();
    volume->modelToWorld = Eigen::Scaling( 1_ra / maxDim ); // In the scene, the volume has size 1^3
    //! [Creating the Volume]

    //! [Create the engine entity for the Volume]
    auto e = app.m_engine->getEntityManager()->createEntity( "Volume demo" );
    e->setTransform( Ra::Core::Transform { Ra::Core::Translation( 0_ra, 0_ra, 0.5_ra ) } );
    //! [Create the engine entity for the Volume]

    //! [Create a geometry component with the Volume]
    auto c = new Ra::Engine::Scene::VolumeComponent( "Volume Demo", e, volume );
    //! [Create a geometry component with the Volume]

    //! [Register the entity/component association to the geometry system ]
    auto geometrySystem = app.m_engine->getSystem( "GeometrySystem" );
    geometrySystem->addComponent( e, c );
    //! [Register the entity/component association to the geometry system ]

    //! [Creating the cube]
    auto cube = Ra::Core::Geometry::makeSharpBox( {0.5f, 0.5f, 1.f} );
    //! [Creating the cube]

    //! [Colorize the Cube]
    cube.addAttrib(
        "in_color",
        Ra::Core::Vector4Array {cube.vertices().size(), Ra::Core::Utils::Color::White()} );
    //! [Colorize the Cube]

    //! [Create the engine entity for the cube]
    auto ce = app.m_engine->getEntityManager()->createEntity( "Green cube" );
    //! [Create the engine entity for the cube]

    //! [Create a geometry component with the cube]
    auto cc =
        new Ra::Engine::Scene::TriangleMeshComponent( "Cube Mesh", ce, std::move( cube ), nullptr );
    //! [Create a geometry component with the cube]

    //! [Register the entity/component association to the geometry system ]
    geometrySystem->addComponent( ce, cc );
    //! [Register the entity/component association to the geometry system ]


    //! [Tell the window that something is to be displayed]
    app.m_mainWindow->prepareDisplay();
    //! [Tell the window that something is to be displayed]

    // terminate the app after 8 second (approximatively). Camera can be moved using mouse moves.
    auto close_timer = new QTimer( &app );
    close_timer->setInterval( 8000 );
    QObject::connect( close_timer, &QTimer::timeout, [&app]() { app.appNeedsToQuit(); } );
    //close_timer->start();

    return app.exec();
}
