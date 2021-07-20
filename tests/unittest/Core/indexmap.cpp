#include <Core/Utils/IndexMap.hpp>
#include <catch2/catch.hpp>
#include <unittestUtils.hpp>

using Ra::Core::Utils::Index;
using Ra::Core::Utils::IndexMap;

// Just a standard test structure
struct Foo {
    explicit Foo( int x ) : value( x ) {}
    int value;
};

TEST_CASE( "Core/Utils/IndexMap", "[Core][Core/Utils][IndexMap]" ) {

    SECTION( "Sanity checks" ) {
        IndexMap<Foo> map1;
        // New map should be empty
        REQUIRE( map1.empty() );
        // New map should be empty (size)
        REQUIRE( map1.size() == 0 );
        // New map should not be full
        REQUIRE( !map1.full() );
    }

    SECTION( "Test Copyable" ) {
        IndexMap<Foo> map1;
        Index i1, i2;
        i1 = map1.insert( Foo( 12 ) );
        REQUIRE( i1.isValid() );

        // We expect to have one element now.
        REQUIRE( !map1.empty() );
        REQUIRE( map1.size() == 1 );

        // Test value read and write
        REQUIRE( map1.at( i1 ).value == 12 );
        REQUIRE( map1[i1].value == 12 );

        map1.access( i1 ).value = 24;
        REQUIRE( map1.at( i1 ).value == 24 );

        map1[i1].value = 32;
        REQUIRE( map1.at( i1 ).value == 32 );

        i2 = map1.insert( Foo( 42 ) );
        REQUIRE( map1.at( i2 ).value == 42 );
        REQUIRE( map1.size() == 2 );

        //////////////////////////////////////////////////
        // Test range-based for loop (relies on iterators)
        uint counter = 0;
        for ( const Foo& f : map1 )
        {
            REQUIRE( ( f.value == 32 || f.value == 42 ) );
            ++counter;
        }
        REQUIRE( counter == 2 );

        // Test non-const loop
        counter = 0;
        for ( Foo& f : map1 )
        {
            f.value = 2 * f.value;
            REQUIRE( ( f.value == 2 * 32 || f.value == 2 * 42 ) );
            ++counter;
        }
        REQUIRE( counter == 2 );

        // Test index iterators
        counter = 0;
        for ( auto it = map1.cbegin_index(); it != map1.cend_index(); ++it )
        {
            REQUIRE( ( *it == i1 || *it == i2 ) );
            ++counter;
        }

        ///////////////////////////////////
        // Test methods contains and remove

        REQUIRE( map1.contains( i1 ) );
        REQUIRE( map1.contains( i2 ) );
        REQUIRE( !map1.contains( Index( 12000 ) ) );

        // Remove an item once.
        bool result = map1.remove( i1 );
        REQUIRE( result );
        REQUIRE( map1.size() == 1 );

        // Removing it twice should not work.
        result = map1.remove( i1 );
        REQUIRE( !result );

        // Remove the other item
        result = map1.remove( Index( 1000 ) );
        REQUIRE( !result );

        result = map1.remove( i2 );
        REQUIRE( result );

        REQUIRE( map1.size() == 0 );
        REQUIRE( map1.empty() );
    }

    SECTION( "Test Non-Copyable" ) {
        using Ra::Core::Utils::Index;
        using Ra::Core::Utils::IndexMap;
        // Now try to insert non-copyable elements

        IndexMap<NonCopy> map2;
        Index i1 = map2.emplace( std::move( 12 ) );
        // map insert (inplace)
        REQUIRE( i1.isValid() );

        Index i2 = map2.emplace( std::move( 42 ) );
        // map insert (inplace)
        REQUIRE( i2.isValid() );

        // map access (inplace)
        REQUIRE( map2[i1].value == 12 );
        REQUIRE( map2[i2].value == 42 );

        // Test the clear function
        map2.clear();
        REQUIRE( map2.size() == 0 );
        REQUIRE( map2.empty() );
    }
}

template <typename T>
void testType() {
    T step = std::numeric_limits<T>::max() / T { 1000 };
    for ( T i = static_cast<T>( std::numeric_limits<Index::IntegerType>::max() ) + 1;
          i < static_cast<T>( std::numeric_limits<T>::max() - 2 * step );
          i += step )
    {
        // Index is more than max, so it is invalid
        Index idx { i };
        REQUIRE( idx.isInvalid() );
    }
}

TEST_CASE( "Core/Utils/Index/Ctor", "[Core][Core/Utils][Index]" ) {

    Index idxInvalid;
    REQUIRE( idxInvalid.isInvalid() );
    // testing everything is too long
    const int step = std::numeric_limits<Index::IntegerType>::max() / 1000;
    for ( Index::IntegerType i = 0; i < std::numeric_limits<Index::IntegerType>::max() - 2 * step;
          i += step )
    {
        Index idx { i };
        REQUIRE( idx.isValid() );
        auto idxUl = Index { static_cast<unsigned long int>( i ) };
        auto idxL  = Index { static_cast<long int>( i ) };
        auto idxU  = Index { static_cast<unsigned int>( i ) };
        REQUIRE( idxUl.isValid() );
        REQUIRE( idxU.isValid() );
        REQUIRE( idxL.isValid() );
        REQUIRE( idx == idxUl );
        REQUIRE( idx == idxU );
        REQUIRE( idx == idxL );
    }

    testType<unsigned long int>();
    testType<long int>();
    testType<unsigned int>();
    testType<size_t>();
}
