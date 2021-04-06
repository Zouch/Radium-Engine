// Include Radium base application and its simple Gui
#include <Gui/BaseApplication.hpp>
#include <Gui/RadiumWindow/SimpleWindowFactory.hpp>

// include the core geometry/appearance interface
#include <Core/Asset/BlinnPhongMaterialData.hpp>
#include <Core/Geometry/MeshPrimitives.hpp>

// include the Engine/entity/component/system/animation interface
#include <Engine/FrameInfo.hpp>
#include <Engine/Scene/EntityManager.hpp>
#include <Engine/Scene/GeometryComponent.hpp>
#include <Engine/Scene/System.hpp>

// include the keyframe animation interface
#include <Core/Animation/KeyFramedValueController.hpp>
#include <Core/Animation/KeyFramedValueInterpolators.hpp>
#include <Core/Tasks/Task.hpp>

// include the render object interface to keyframe the material
#include <Engine/Rendering/RenderObjectManager.hpp>

// To terminate the demo after 4 seconds
#include <QTimer>

//! [Define a key-framed geometry component]
/**
 * KeyFramedGeometryComponent : example on extending a geometry component to animate its
 * material color.
 *
 *   When creating a KeyFramedGeometryComponent with a Core::TriangleMesh, this associate an
 *   animated Blinn-Phong material to the geometry and update both the transformation and the
 *   diffuse color at each time step.
 *
 *   @note This is only for demo purpose. A more simpler approach could be :
 *      - Create and add a geometry component to the Engine geometry system (like in HelloRadium)
 *      - Get the renderobject associated with this component and create an animation component
 *              that associates the RenderObject with an animation controller
 *      - Add the animation component to the animation system
 *
 */
class KeyFramedGeometryComponent : public Ra::Engine::Scene::TriangleMeshComponent
{
  public:
    /*!
     * Constructor from an existing core mesh
     * \warning Moves the mesh and takes its ownership
     */
    inline KeyFramedGeometryComponent( const std::string& name,
                                       Ra::Engine::Scene::Entity* entity,
                                       Ra::Core::Geometry::TriangleMesh&& mesh ) :
        Ra::Engine::Scene::TriangleMeshComponent( name,
                                                  entity,
                                                  std::move( mesh ),
                                                  new Ra::Core::Asset::BlinnPhongMaterialData {} ),
        m_transform( 0_ra, Ra::Core::Transform::Identity() ) {
        //! [Creating the transform KeyFrames]
        Ra::Core::Transform T = Ra::Core::Transform::Identity();
        T.rotate( Ra::Core::AngleAxis( Ra::Core::Math::PiDiv3, Ra::Core::Vector3::UnitY() ) );
        m_transform.insertKeyFrame( 1_ra, T );
        T.rotate( Ra::Core::AngleAxis( Ra::Core::Math::PiDiv3, Ra::Core::Vector3::UnitY() ) );
        m_transform.insertKeyFrame( 2_ra, T );
        T.rotate( Ra::Core::AngleAxis( Ra::Core::Math::PiDiv3, Ra::Core::Vector3::UnitY() ) );
        m_transform.insertKeyFrame( 3_ra, T );
        //! [Creating the transform KeyFrames]
        //! [Creating the color KeyFrames]
        auto colors = new Ra::Core::Animation::KeyFramedValue<Ra::Core::Utils::Color>(
            0_ra, Ra::Core::Utils::Color::Green() );
        colors->insertKeyFrame( 1_ra, Ra::Core::Utils::Color::Red() );
        colors->insertKeyFrame( 2_ra, Ra::Core::Utils::Color::Blue() );
        colors->insertKeyFrame( 3_ra, Ra::Core::Utils::Color::Green() );
        //! [Attach the color KeyFrames to a controller of the Render object material]
        m_colorController.m_value = colors;
        m_ro                      = getRoMgr()->getRenderObject( m_roIndex );
        auto material =
            dynamic_cast<Ra::Engine::Data::BlinnPhongMaterial*>( m_ro->getMaterial().get() );
        m_colorController.m_updater = [colors, material]( const Scalar& t ) {
            auto C =
                colors->at( t, Ra::Core::Animation::linearInterpolate<Ra::Core::Utils::Color> );
            material->m_kd = C;
            material->needUpdate();
        };
        //! [Attach the color KeyFrames to a controller]
        //! [Creating the color KeyFrames]
    }
    /// This function uses the keyframes to update the cube to time \p t.
    void update( Scalar t ) {
        //! [Fetch transform from KeyFramedValue]
        auto T = m_transform.at( t, Ra::Core::Animation::linearInterpolate<Ra::Core::Transform> );
        //! [Fetch transform from KeyFramedValue]
        m_ro->setLocalTransform( T );

        //! [Update color from KeyFramedValue]
        m_colorController.updateKeyFrame( t );
        //! [Update color from KeyFramedValue]
    }

    /// The render object to animate
    std::shared_ptr<Ra::Engine::Rendering::RenderObject> m_ro;

    /// The Keyframes for the render object's tranform.
    Ra::Core::Animation::KeyFramedValue<Ra::Core::Transform> m_transform;

    /// The Keyframes for the render object's color.
    Ra::Core::Animation::KeyFramedValueController m_colorController;
};
//! [Define a key-framed geometry component]

//! [Define a simple animation system]
/// This system will be added to the engine. Every frame it will
/// add a task to be executed, calling the update function of the component.
/// \note This system makes time loop around.
class SimpleAnimationSystem : public Ra::Engine::Scene::System
{
  public:
    virtual void generateTasks( Ra::Core::TaskQueue* q,
                                const Ra::Engine::FrameInfo& info ) override {
        KeyFramedGeometryComponent* c =
            static_cast<KeyFramedGeometryComponent*>( m_components[0].second );

        // Create a new task which wil call c->spin() when executed.
        q->registerTask( new Ra::Core::FunctionTask(
            std::bind( &KeyFramedGeometryComponent::update, c, info.m_animationTime ), "spin" ) );
    }
};

int main( int argc, char* argv[] ) {
    //! [Creating the application]
    Ra::Gui::BaseApplication app( argc, argv );
    app.initialize( Ra::Gui::SimpleWindowFactory {} );

    //![Parameterize the Engine  time loop]
    app.m_engine->setEndTime( 3_ra ); // <-- 3 relates to the keyframes of the demo component.
    app.m_engine->setRealTime( true );
    app.m_engine->play( true );

    //! [Create the demo animation system]
    SimpleAnimationSystem* sys = new SimpleAnimationSystem;
    app.m_engine->registerSystem( "Minimal system", sys );

    //! [Create the demo animated component]
    //! [Creating the cube]
    auto cube = Ra::Core::Geometry::makeSharpBox( {0.1f, 0.1f, 0.1f} );
    //! [Creating the cube]

    //! [Create the engine entity for the cube]
    auto e = app.m_engine->getEntityManager()->createEntity( "Green cube" );
    //! [Create the engine entity for the cube]

    //! [Create a KeyFramedGeometryComponent component with the cube]
    auto c = new KeyFramedGeometryComponent( "Cube Mesh", e, std::move( cube ) );
    //! [Create a KeyFramedGeometryComponent component with the cube]
    //! [Create the demo animated component]

    //! [add the component to the animation system]
    sys->addComponent( e, c );
    //! [add the component to the animation system]

    //! [Tell the window that something is to be displayed]
    app.m_mainWindow->prepareDisplay();
    //! [Tell the window that something is to be displayed]

    // terminate the app after 6 second (approximatively). Camera can be moved using mouse moves.
    auto close_timer = new QTimer( &app );
    close_timer->setInterval( 6000 );
    QObject::connect( close_timer, &QTimer::timeout, [&app]() { app.appNeedsToQuit(); } );
    close_timer->start();

    return app.exec();
}
