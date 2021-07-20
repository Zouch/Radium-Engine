#include <catch2/catch.hpp>

#include <Engine/RadiumEngine.hpp>
#include <Engine/Scene/Component.hpp>
#include <Engine/Scene/Entity.hpp>
#include <Engine/Scene/EntityManager.hpp>
#include <Engine/Scene/SignalManager.hpp>

using namespace Ra::Engine::Scene;

class Foo
{
  public:
    Ra::Core::Utils::Index i { 0 };
    void incr( const ItemEntry& /*entity*/ ) { ++i; }
    void decr( const ItemEntry& /*entity*/ ) { --i; }
};

class Bar
{
  public:
    explicit Bar( Foo& f ) : m_f { f } {};
    void operator()( const ItemEntry& entity ) { m_f.incr( entity ); }

  private:
    Foo& m_f;
};

class FooBarComponent : public Component
{
  public:
    using Component::Component;
    void initialize() override {};
};

TEST_CASE( "Engine/Scene/SignalManager/ON", "[Engine][Engine/Scene][SignalManager][ON]" ) {

    auto engine = Ra::Engine::RadiumEngine::createInstance();
    engine->initialize();

    SignalManager signalmanager;
    signalmanager.setOn( true );

    Foo entitytest;
    Foo componenttest;
    Foo renderobjecttest;
    Foo eoftest;

    auto entity = engine->getEntityManager()->createEntity( "test entity" );

    SECTION( "Entities signals" ) {
        ItemEntry item { entity };
        auto& addNotifier = signalmanager.getEntityCreatedNotifier();
        auto& delNotifier = signalmanager.getEntityDestroyedNotifier();

        Bar bar { entitytest };

        int ia = addNotifier.attach( bar );
        int id = delNotifier.attachMember( &entitytest, &Foo::decr );

        REQUIRE( entitytest.i == 0 );

        signalmanager.fireEntityCreated( item );
        REQUIRE( entitytest.i == 1 );
        REQUIRE( componenttest.i == 0 );
        REQUIRE( renderobjecttest.i == 0 );
        REQUIRE( eoftest.i == 0 );

        signalmanager.fireEntityDestroyed( item );
        REQUIRE( entitytest.i == 0 );
        REQUIRE( componenttest.i == 0 );
        REQUIRE( renderobjecttest.i == 0 );
        REQUIRE( eoftest.i == 0 );

        addNotifier.detach( ia );
        signalmanager.fireEntityCreated( item );
        REQUIRE( entitytest.i == 0 );
        REQUIRE( componenttest.i == 0 );
        REQUIRE( renderobjecttest.i == 0 );
        REQUIRE( eoftest.i == 0 );

        delNotifier.detach( id );
        signalmanager.fireEntityDestroyed( item );
        REQUIRE( entitytest.i == 0 );
        REQUIRE( componenttest.i == 0 );
        REQUIRE( renderobjecttest.i == 0 );
        REQUIRE( eoftest.i == 0 );
    }

    SECTION( "Components signals" ) {

        auto component = new FooBarComponent( "test component", entity );

        ItemEntry item { entity, component };
        auto& addNotifier = signalmanager.getComponentCreatedNotifier();
        auto& delNotifier = signalmanager.getComponentDestroyedNotifier();

        auto bar = [&componenttest]( const ItemEntry& e ) { componenttest.incr( e ); };

        int ia = addNotifier.attach( bar );
        int id = delNotifier.attachMember( &componenttest, &Foo::decr );

        REQUIRE( componenttest.i == 0 );

        signalmanager.fireComponentAdded( item );
        REQUIRE( componenttest.i == 1 );
        REQUIRE( entitytest.i == 0 );
        REQUIRE( renderobjecttest.i == 0 );
        REQUIRE( eoftest.i == 0 );

        signalmanager.fireComponentRemoved( item );
        REQUIRE( componenttest.i == 0 );
        REQUIRE( entitytest.i == 0 );
        REQUIRE( renderobjecttest.i == 0 );
        REQUIRE( eoftest.i == 0 );

        addNotifier.detach( ia );
        signalmanager.fireComponentAdded( item );
        REQUIRE( componenttest.i == 0 );
        REQUIRE( entitytest.i == 0 );
        REQUIRE( renderobjecttest.i == 0 );
        REQUIRE( eoftest.i == 0 );

        delNotifier.detach( id );
        signalmanager.fireComponentRemoved( item );
        REQUIRE( componenttest.i == 0 );
        REQUIRE( entitytest.i == 0 );
        REQUIRE( renderobjecttest.i == 0 );
        REQUIRE( eoftest.i == 0 );
    }
    SECTION( "RenderObjects signals" ) {

        auto component = new FooBarComponent( "test component", entity );

        ItemEntry item { entity, component, 1 };
        auto& addNotifier = signalmanager.getRenderObjectCreatedNotifier();
        auto& delNotifier = signalmanager.getRenderObjectDestroyedNotifier();

        int ia = addNotifier.attach(
            [&renderobjecttest]( const ItemEntry& e ) { renderobjecttest.incr( e ); } );
        int id = delNotifier.attachMember( &renderobjecttest, &Foo::decr );

        REQUIRE( renderobjecttest.i == 0 );

        signalmanager.fireRenderObjectAdded( item );
        REQUIRE( renderobjecttest.i == 1 );
        REQUIRE( componenttest.i == 0 );
        REQUIRE( entitytest.i == 0 );
        REQUIRE( eoftest.i == 0 );

        signalmanager.fireRenderObjectRemoved( item );
        REQUIRE( renderobjecttest.i == 0 );
        REQUIRE( componenttest.i == 0 );
        REQUIRE( entitytest.i == 0 );
        REQUIRE( eoftest.i == 0 );

        addNotifier.detach( ia );
        signalmanager.fireRenderObjectAdded( item );
        REQUIRE( renderobjecttest.i == 0 );
        REQUIRE( componenttest.i == 0 );
        REQUIRE( entitytest.i == 0 );
        REQUIRE( eoftest.i == 0 );

        delNotifier.detach( id );
        signalmanager.fireRenderObjectRemoved( item );
        REQUIRE( renderobjecttest.i == 0 );
        REQUIRE( componenttest.i == 0 );
        REQUIRE( entitytest.i == 0 );
        REQUIRE( eoftest.i == 0 );
    }

    SECTION( "End of Frame signal" ) {

        auto component = new FooBarComponent( "test component", entity );

        // this one is just to check it compiles ... we can  add some REQUIREs in a near futur.
        auto& eofNotifier = signalmanager.getEndFrameNotifier();

        ItemEntry item { entity, component, 1 };

        int ia = eofNotifier.attach( [&eoftest, item]() { eoftest.incr( item ); } );

        REQUIRE( eoftest.i == 0 );

        signalmanager.fireFrameEnded();
        REQUIRE( eoftest.i == 1 );
        signalmanager.fireFrameEnded();
        REQUIRE( eoftest.i == 2 );

        eofNotifier.detach( ia );
        signalmanager.fireFrameEnded();
        REQUIRE( eoftest.i == 2 );
    }
    engine->cleanup();
    Ra::Engine::RadiumEngine::destroyInstance();
}

TEST_CASE( "Engine/Scene/SignalManager/OFF/", "[Engine][Engine/Scene][SignalManager][OFF]" ) {

    auto engine = Ra::Engine::RadiumEngine::createInstance();
    engine->initialize();

    SignalManager signalmanager;
    signalmanager.setOn( false );

    auto entity = engine->getEntityManager()->createEntity( "test entity" );

    Foo entitytest;
    Foo componenttest;
    Foo renderobjecttest;
    Foo eoftest;

    SECTION( "Entities signals" ) {
        ItemEntry item { entity };
        auto& addNotifier = signalmanager.getEntityCreatedNotifier();
        auto& delNotifier = signalmanager.getEntityDestroyedNotifier();

        Bar bar { entitytest };

        addNotifier.attach( bar );
        delNotifier.attachMember( &entitytest, &Foo::decr );

        REQUIRE( entitytest.i == 0 );

        signalmanager.fireEntityCreated( item );
        REQUIRE( entitytest.i == 0 );
        REQUIRE( componenttest.i == 0 );
        REQUIRE( renderobjecttest.i == 0 );
        REQUIRE( eoftest.i == 0 );

        signalmanager.fireEntityDestroyed( item );
        REQUIRE( entitytest.i == 0 );
        REQUIRE( componenttest.i == 0 );
        REQUIRE( renderobjecttest.i == 0 );
        REQUIRE( eoftest.i == 0 );
    }

    SECTION( "Components signals" ) {
        auto component = new FooBarComponent( "test component", entity );

        ItemEntry item { entity, component };
        auto& addNotifier = signalmanager.getComponentCreatedNotifier();
        auto& delNotifier = signalmanager.getComponentDestroyedNotifier();

        auto bar = [&componenttest]( const ItemEntry& e ) { componenttest.incr( e ); };

        addNotifier.attach( bar );
        delNotifier.attachMember( &componenttest, &Foo::decr );

        REQUIRE( componenttest.i == 0 );

        signalmanager.fireComponentAdded( item );
        REQUIRE( componenttest.i == 0 );
        REQUIRE( entitytest.i == 0 );
        REQUIRE( renderobjecttest.i == 0 );
        REQUIRE( eoftest.i == 0 );

        signalmanager.fireComponentRemoved( item );
        REQUIRE( componenttest.i == 0 );
        REQUIRE( entitytest.i == 0 );
        REQUIRE( renderobjecttest.i == 0 );
        REQUIRE( eoftest.i == 0 );
    }

    SECTION( "RenderObjects signals" ) {

        auto component = new FooBarComponent( "test component", entity );

        ItemEntry item { entity, component, 1 };
        auto& addNotifier = signalmanager.getRenderObjectCreatedNotifier();
        auto& delNotifier = signalmanager.getRenderObjectDestroyedNotifier();

        addNotifier.attach(
            [&renderobjecttest]( const ItemEntry& e ) { renderobjecttest.incr( e ); } );
        delNotifier.attachMember( &renderobjecttest, &Foo::decr );

        REQUIRE( renderobjecttest.i == 0 );

        signalmanager.fireRenderObjectAdded( item );
        REQUIRE( renderobjecttest.i == 0 );
        REQUIRE( componenttest.i == 0 );
        REQUIRE( entitytest.i == 0 );
        REQUIRE( eoftest.i == 0 );

        signalmanager.fireRenderObjectRemoved( item );
        REQUIRE( renderobjecttest.i == 0 );
        REQUIRE( componenttest.i == 0 );
        REQUIRE( entitytest.i == 0 );
        REQUIRE( eoftest.i == 0 );
    }

    SECTION( "End of Frame signal" ) {
        auto component = new FooBarComponent( "test component", entity );

        // this one is just to check it compiles ... we can  add some REQUIREs in a near futur.
        auto& eofNotifier = signalmanager.getEndFrameNotifier();

        ItemEntry item { entity, component, 1 };

        eofNotifier.attach( [&eoftest, item]() { eoftest.incr( item ); } );

        REQUIRE( eoftest.i == 0 );

        signalmanager.fireFrameEnded();
        REQUIRE( eoftest.i == 0 );
    }
    engine->cleanup();
    Ra::Engine::RadiumEngine::destroyInstance();
}
