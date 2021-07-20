// Include Radium base application and its simple Gui
#include <Gui/BaseApplication.hpp>
#include <Gui/RadiumWindow/SimpleWindowFactory.hpp>

// include the Engine/entity/component interface
#include <Core/Geometry/MeshPrimitives.hpp>
#include <Engine/FrameInfo.hpp>
#include <Engine/Scene/EntityManager.hpp>
#include <Engine/Scene/GeometryComponent.hpp>
#include <Engine/Scene/GeometrySystem.hpp>

#include <Core/Tasks/Task.hpp>

#include <QTimer>

class SimpleSimulationSystem : public Ra::Engine::Scene::System
{
    Ra::Engine::Data::PointCloud* m_cloud;
    const std::string colorHandleName = Ra::Engine::Data::AttribArrayDisplayable::getAttribName(
        Ra::Engine::Data::AttribArrayDisplayable::VERTEX_COLOR );

    Ra::Core::Utils::Attrib<Ra::Core::Vector4>& getColorAttrib() {
        auto cHandle =
            m_cloud->getCoreGeometry().getAttribHandle<Ra::Core::Vector4>( colorHandleName );
        return m_cloud->getCoreGeometry().getAttrib( cHandle );
    }

  public:
    inline SimpleSimulationSystem( Ra::Engine::Data::PointCloud* c = nullptr ) : m_cloud( c ) {}

    virtual void generateTasks( Ra::Core::TaskQueue* q,
                                const Ra::Engine::FrameInfo& /*info*/ ) override {

        // Random motion + color update wrt to position
        q->registerTask( new Ra::Core::FunctionTask(
            [/*info,*/ this]() {
                //            auto dt = info.m_dt;
                auto& vbuf = m_cloud->getCoreGeometry().verticesWithLock();

                auto& cAttrib = getColorAttrib();
                auto& cbuf    = cAttrib.getDataWithLock();

                for ( size_t i = 0; i != vbuf.size(); ++i )
                {
                    auto& x = vbuf[i];
                    x       = ( x + 0.02 * Ra::Core::Vector3::Random() )
                            .cwiseMin( 1_ra )
                            .cwiseMax( -1_ra );
                    cbuf[i].template head<3>() = x.array() * 0.5 + 0.5;
                }

                m_cloud->getCoreGeometry().verticesUnlock();
                cAttrib.unlock();
            },
            "randomize" ) );
    }
};

int main( int argc, char* argv[] ) {
    //! [Creating the application]
    Ra::Gui::BaseApplication app( argc, argv );
    app.initialize( Ra::Gui::SimpleWindowFactory {} );
    //! [Creating the application]

    //! [Creating the cloud]
    auto cloud = Ra::Core::Geometry::PointCloud();
    auto& v    = cloud.verticesWithLock();
    v.resize( 100 );
    for ( auto& x : v )
        x = Ra::Core::Vector3::Random();
    cloud.verticesUnlock();
    //! [Creating the cloud]

    //! [Colorize the cloud]
    cloud.addAttrib(
        "in_color",
        Ra::Core::Vector4Array { cloud.vertices().size(), Ra::Core::Utils::Color::Green() } );
    //! [Colorize the cloud]

    //! [Create the engine entity for the cloud]
    auto e = app.m_engine->getEntityManager()->createEntity( "Green cloud" );
    //! [Create the engine entity for the cloud]

    //! [Create a geometry component with the cloud]
    auto c =
        new Ra::Engine::Scene::PointCloudComponent( "cloud Mesh", e, std::move( cloud ), nullptr );
    c->setSplatSize( 0.05f );
    //! [Create a geometry component with the cloud]

    //! [Register the entity/component association to the geometry system ]
    auto geometrySystem = app.m_engine->getSystem( "GeometrySystem" );
    geometrySystem->addComponent( e, c );
    //! [Register the entity/component association to the geometry system ]

    //! [Create and Register the simulation system ]
    SimpleSimulationSystem* simu = new SimpleSimulationSystem( c->getGeometry() );
    app.m_engine->registerSystem( "AnimationSystem", simu );
    //! [Create and Register the simulation system ]

    //! [Tell the window that something is to be displayed]
    app.m_mainWindow->prepareDisplay();
    //! [Tell the window that something is to be displayed]

    // terminate the app after 4 second (approximatively). Camera can be moved using mouse moves.
    auto close_timer = new QTimer( &app );
    close_timer->setInterval( 4000 );
    QObject::connect( close_timer, &QTimer::timeout, [&app]() { app.appNeedsToQuit(); } );
    close_timer->start();

    return app.exec();
}
