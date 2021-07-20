#include <Core/Utils/Observable.hpp>
#include <catch2/catch.hpp>

using Ra::Core::Utils::Observable;

// This class can notify observers with no args
class ObservableVoid : public Observable<>
{};

// This class can notify observers with a int
class ObservableInt : public Observable<int>
{};

// This class have two observable "hooks"
class PeterPan
{

  public:
    void setName( std::string arg ) {
        m_name = arg;
        m_hookString.notify( m_name );
        m_hook.notify();
    }
    void setBestFriendNameAndAge( std::string name, int age ) {
        m_best    = name;
        m_bestAge = age;
        m_hookIntString.notify( m_bestAge, m_best );
        m_hook.notify();
    }
    Observable<>& getHook() { return m_hook; }
    Observable<std::string>& getHookName() { return m_hookString; }
    Observable<int, std::string>& getHookBest() { return m_hookIntString; }

  private:
    Observable<> m_hook;
    Observable<std::string> m_hookString;
    Observable<int, std::string> m_hookIntString;
    std::string m_name {};
    std::string m_best {};
    int m_bestAge;
};

class Spy
{
  public:
    void spyPeter( PeterPan& p ) {
        p.getHook().attach( [this]() { m_cpt++; } );
        p.getHookName().attachMember( this, &Spy::addName );
        p.getHookBest().attachMember( this, &Spy::addBest );
    }

  private:
    void addName( std::string n ) { m_hisName.push_back( n ); }
    void addBest( int a, std::string n ) { m_hisBests.emplace_back( n, a ); }

    // public to allow easy tests
  public:
    std::vector<std::string> m_hisName;
    std::vector<std::pair<std::string, int>> m_hisBests;
    int m_cpt { 0 };
};

class A
{
  public:
    void f() { m_a++; }
    void f2( int a ) { m_a = a; }
    void f3( int a, int b ) { m_a = a; }
    static void g() { m_b++; }
    static void g2( int b ) { m_b = b; }

    int m_a { 0 };
    static int m_b;
};

int A::m_b = 0;

TEST_CASE( "Core/Utils/Observable", "[Core][Core/Utils][Observable]" ) {
    ObservableVoid observableVoid;
    ObservableInt observableInt;

    A a;
    int c  = 0;
    A::m_b = 0;

    SECTION( "attach member to hook" ) {
        // this one is just to check it compiles ... we can  add some REQUIREs in a near futur.
        Observable<> hookVoid;
        Observable<int> hookInt;
        Observable<int, int> hookInt2;

        hookVoid.attachMember( &a, &A::f );
        hookInt.attachMember( &a, &A::f2 );
        hookInt2.attachMember( &a, &A::f3 );
        hookVoid.notify();
        hookInt.notify( 7 );
    }

    SECTION( "Spy Peter Pan" ) {
        PeterPan peter;
        Spy badGuy;
        badGuy.spyPeter( peter );

        peter.setName( "PeterPan !" );
        REQUIRE( "PeterPan !" == badGuy.m_hisName.back() );
        peter.setBestFriendNameAndAge( "Tinker Bell", 150 );
        peter.setBestFriendNameAndAge( "John", 11 );
        peter.setBestFriendNameAndAge( "Michael", 8 );
        peter.setName( "PeterPan" );

        REQUIRE( "PeterPan" == badGuy.m_hisName.back() );
        peter.setBestFriendNameAndAge( "Wendy", 12 );
        peter.setName( "Mr. Pan" );

        REQUIRE( "Mr. Pan" == badGuy.m_hisName.back() );
        peter.setBestFriendNameAndAge( "Wendy", 18 );

        REQUIRE( badGuy.m_hisBests.size() == 5 );
    }

    SECTION( "observe me" ) {
        using Observer = std::function<void( void )>;
        ///\todo add more tests with
        // using Observer2 = std::function<void( int )>;

        auto bf  = std::bind( &A::f, &a );
        auto bf2 = std::bind( &A::f2, &a, std::placeholders::_1 );

        Observer obf         = bf;
        auto& observerTarget = obf.target_type();
        // "Type failed."
        REQUIRE( obf.target_type() == observerTarget );

        auto gid = observableVoid.attach( A::g );
        observableVoid.attach( bf );
        observableVoid.attach( [&c]() { c++; } );
        observableVoid.notify();

        REQUIRE( c == 1 );
        REQUIRE( a.m_a == 1 );
        REQUIRE( A::m_b == 1 );

        observableVoid.detach( gid );
        observableVoid.notify();

        REQUIRE( c == 2 );
        REQUIRE( a.m_a == 2 );
        // Test detach
        REQUIRE( A::m_b == 1 );

        observableVoid.attach( A::g );
        observableVoid.attach( A::g );
        observableVoid.attach( bf );
        observableVoid.attach( [&c]() { c++; } );
        observableVoid.notify();

        REQUIRE( c == 4 );
        REQUIRE( a.m_a == 4 );
        REQUIRE( A::m_b == 3 );

        observableVoid.detachAll();
        observableVoid.notify();

        // Test detach
        REQUIRE( c == 4 );
        REQUIRE( a.m_a == 4 );
        REQUIRE( A::m_b == 3 );

        observableInt.attachMember( &a, &A::f2 );
        observableInt.attach( &A::g2 );
        observableInt.attach( [&c]( int pc ) { c = pc; } );

        observableInt.notify( 5 );

        REQUIRE( c == 5 );
        REQUIRE( a.m_a == 5 );
        REQUIRE( A::m_b == 5 );

        observableInt.notify( 6 );

        REQUIRE( c == 6 );
        REQUIRE( a.m_a == 6 );
        REQUIRE( A::m_b == 6 );

        ObservableInt test2copy;
        observableInt.copyObserversTo( test2copy );

        observableInt.detachAll();
        observableInt.notify( 7 );

        REQUIRE( c == 6 );
        REQUIRE( a.m_a == 6 );
        REQUIRE( A::m_b == 6 );

        test2copy.notify( 7 );

        REQUIRE( c == 7 );
        REQUIRE( a.m_a == 7 );
        REQUIRE( A::m_b == 7 );
    }
}
