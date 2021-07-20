#include <Core/Animation/HandleWeightOperation.hpp>
//! [include keyframed]
#include <Core/Animation/KeyFramedValue.hpp>
#include <Core/Animation/KeyFramedValueInterpolators.hpp>
//! [include keyframed]
//! [include keyframedvaluecontroller]
#include <Core/Animation/KeyFramedValueController.hpp>
//! [include keyframedvaluecontroller]

//! [include DualQuaternionSkinning ]
#include <Core/Animation/DualQuaternionSkinning.hpp>
//! [include DualQuaternionSkinning ]

#include <Core/Animation/PoseOperation.hpp>
#include <Core/Animation/Skeleton.hpp>

#include <catch2/catch.hpp>

using namespace Ra::Core;
using namespace Ra::Core::Animation;

TEST_CASE( "Core/Animation/HandleWeightOperation",
           "[Core][Core/Animation][HandleWeightOperation]" ) {
    static const constexpr int w = 50;
    static const constexpr int h = w;

    WeightMatrix matrix1 { w, h };
    matrix1.setIdentity();

    SECTION( "Test normalization" ) {
        // Matrix1 is already normalized
        REQUIRE( !normalizeWeights( matrix1 ) );

        matrix1.coeffRef( w / 3, h / 2 ) = 0.8_ra;

        // Matrix1 normalization ok
        REQUIRE( normalizeWeights( matrix1 ) );

        WeightMatrix matrix2 = matrix1;
        matrix2 *= 0.5_ra;

        WeightMatrix matrix3 = partitionOfUnity( matrix2 );

        // Matrix2 needs to be normalized
        REQUIRE( normalizeWeights( matrix2 ) );
        // Matrix3 is already normalized
        REQUIRE( !normalizeWeights( matrix3 ) );
        // Two matrices are equivalent after normalization
        REQUIRE( matrix1.isApprox( matrix2 ) );
        // Two matrices are equivalent after partition of unity
        REQUIRE( matrix1.isApprox( matrix3 ) );

        matrix2.coeffRef( w / 3, h / 2 ) = std::nanf( "" );
        // Should not find NaN in this matrix
        REQUIRE( checkWeightMatrix( matrix1, false ) );
        // Should find NaN in this matrix
        REQUIRE( !checkWeightMatrix( matrix2, false ) );
    }
}

TEST_CASE( "Core/Animation/KeyFramedValue", "[Core][Core/Animation][KeyFramedValue]" ) {

    KeyFramedValue<Scalar> kf { 2_ra, 2_ra };

    auto checkValues = []( auto& p, Scalar time, Scalar value ) {
        REQUIRE( Math::areApproxEqual( p.first, time ) );
        REQUIRE( Math::areApproxEqual( p.second, value ) );
    };

    auto checkSorting = []( auto& lkf ) {
        for ( size_t i = 1; i < lkf.size(); ++i ) {
            REQUIRE( lkf[i - 1].first < lkf[i].first );
        }
    };

    SECTION( "Test keyframe manipulation" ) {
        // There is one keyframe
        REQUIRE( kf.size() == 1 );
        // We cannot remove it
        REQUIRE( !kf.removeKeyFrame( 0 ) );
        // It should be (2,2)
        checkValues( kf[0], 2_ra, 2_ra );

        // adding before first
        kf.insertKeyFrame( 0_ra, 0_ra );
        // There should be 2 keyframes
        REQUIRE( kf.size() == 2 );
        // These should be sorted
        checkSorting( kf );
        // The first one should be (0,0)
        checkValues( kf[0], 0_ra, 0_ra );
        // The second one should be (2,2)
        checkValues( kf[1], 2_ra, 2_ra );

        // adding in between
        kf.insertKeyFrame( 1_ra, 1_ra );
        // There should be 3 keyframes
        REQUIRE( kf.size() == 3 );
        // These should be sorted
        checkSorting( kf );
        // The first one should be (0,0)
        checkValues( kf[0], 0_ra, 0_ra );
        // The second one should be (1,1)
        checkValues( kf[1], 1_ra, 1_ra );
        // The third one should be (2,2)
        checkValues( kf[2], 2_ra, 2_ra );

        // adding after last
        kf.insertKeyFrame( 3_ra, 3_ra );
        // There should still be 4 keyframes
        REQUIRE( kf.size() == 4 );
        // These should be sorted
        checkSorting( kf );
        // The first one should be (0,0)
        checkValues( kf[0], 0_ra, 0_ra );
        // The second one should be (1,1)
        checkValues( kf[1], 1_ra, 1_ra );
        // The third one should be (2,2)
        checkValues( kf[2], 2_ra, 2_ra );
        // The fourth one should be (3,3)
        checkValues( kf[3], 3_ra, 3_ra );

        {
            auto kf2 = kf;
            // There should be 4 keyframes
            REQUIRE( kf2.size() == 4 );

            // Should be able to remove inside
            REQUIRE( kf2.removeKeyFrame( 2 ) );
            // There should be 3 keyframes
            REQUIRE( kf2.size() == 3 );
            // These should be sorted
            checkSorting( kf2 );
            // The first one should be (0,0)
            checkValues( kf2[0], 0_ra, 0_ra );
            // The second one should be (1,1)
            checkValues( kf2[1], 1_ra, 1_ra );
            // The third one should be (3,3)
            checkValues( kf2[2], 3_ra, 3_ra );

            // Should be able to remove last
            REQUIRE( kf2.removeKeyFrame( 2 ) );
            // There should be 2 keyframes
            REQUIRE( kf2.size() == 2 );
            // These should be sorted
            checkSorting( kf2 );
            // The first one should be (0,0)
            checkValues( kf2[0], 0_ra, 0_ra );
            // The second one should be (1,1)
            checkValues( kf2[1], 1_ra, 1_ra );

            // Should be able to remove first
            REQUIRE( kf2.removeKeyFrame( 0 ) );
            // There should be 1 keyframe
            REQUIRE( kf2.size() == 1 );
            // It should be (1,1)
            checkValues( kf2[0], 1_ra, 1_ra );

            // We cannot remove it
            REQUIRE( !kf2.removeKeyFrame( 0 ) );
        }

        // Replacing value
        kf.insertKeyFrame( 3_ra, 2_ra );
        // There should still be 4 keyframes
        REQUIRE( kf.size() == 4 );
        // These should be sorted
        checkSorting( kf );
        // The first one should be (0,0)
        checkValues( kf[0], 0_ra, 0_ra );
        // The second one should be (1,1)
        checkValues( kf[1], 1_ra, 1_ra );
        // The third one should be (2,2)
        checkValues( kf[2], 2_ra, 2_ra );
        // The fourth one should be (3,2)
        checkValues( kf[3], 3_ra, 2_ra );

        // Moving keyframe to the front
        kf.moveKeyFrame( 1, -1_ra );
        // There should still be 4 keyframes
        REQUIRE( kf.size() == 4 );
        // These should be sorted
        checkSorting( kf );
        // The first one should be (-1,1)
        checkValues( kf[0], -1_ra, 1_ra );
        // The second one should be (0,0)
        checkValues( kf[1], 0_ra, 0_ra );
        // The third one should be (2,2)
        checkValues( kf[2], 2_ra, 2_ra );
        // The fourth one should be (3,2)
        checkValues( kf[3], 3_ra, 2_ra );

        // Moving keyframe in between
        kf.moveKeyFrame( 1, 2.5_ra );
        // There should still be 4 keyframes
        REQUIRE( kf.size() == 4 );
        // These should be sorted
        checkSorting( kf );
        // The first one should be (-1,1)
        checkValues( kf[0], -1_ra, 1_ra );
        // The second one should be (2,2)
        checkValues( kf[1], 2_ra, 2_ra );
        // The third one should be (2.5,0)
        checkValues( kf[2], 2.5_ra, 0_ra );
        // The fourth one should be (3,2)
        checkValues( kf[3], 3_ra, 2_ra );

        // Moving keyframe to the end
        kf.moveKeyFrame( 1, 4_ra );
        // There should still be 4 keyframes
        REQUIRE( kf.size() == 4 );
        // These should be sorted
        checkSorting( kf );
        // The first one should be (-1,1)
        checkValues( kf[0], -1_ra, 1_ra );
        // The second one should be (2.5,0)
        checkValues( kf[1], 2.5_ra, 0_ra );
        // The third one should be (3,2)
        checkValues( kf[2], 3_ra, 2_ra );
        // The fourth one should be (4,2)
        checkValues( kf[3], 4_ra, 2_ra );

        // Evaluating before first time should give first value
        REQUIRE( Math::areApproxEqual( kf.at( kf[0].first - 1, linearInterpolate<Scalar> ),
                                       kf[0].second ) );
        // Evaluating at keyframe time should give keyframe value
        for ( size_t i = 0; i < kf.size(); ++i ) {
            REQUIRE( Math::areApproxEqual( kf.at( kf[i].first, linearInterpolate<Scalar> ),
                                           kf[i].second ) );
        }
        // Evaluating after last time should give last value
        REQUIRE(
            Math::areApproxEqual( kf.at( kf[kf.size() - 1].first + 1, linearInterpolate<Scalar> ),
                                  kf[kf.size() - 1].second ) );
        // Check some in-between interpolations
        REQUIRE( Math::areApproxEqual( kf.at( 0_ra, linearInterpolate<Scalar> ), 5_ra / 7 ) );
        REQUIRE( Math::areApproxEqual( kf.at( 2.75_ra, linearInterpolate<Scalar> ), 1_ra ) );
        REQUIRE( Math::areApproxEqual( kf.at( 3.7_ra, linearInterpolate<Scalar> ), 2_ra ) );

        // Insert before first using interpolation
        kf.insertInterpolatedKeyFrame( -2_ra, linearInterpolate<Scalar> );
        // There should still be 5 keyframes
        REQUIRE( kf.size() == 5 );
        // These should be sorted
        checkSorting( kf );
        // The first one should be (-2,1)
        checkValues( kf[0], -2_ra, 1_ra );
        // The second one should be (-1,1)
        checkValues( kf[1], -1_ra, 1_ra );
        // The third one should be (2.5,0)
        checkValues( kf[2], 2.5_ra, 0_ra );
        // The fourth one should be (3,2)
        checkValues( kf[3], 3_ra, 2_ra );
        // The fifth one should be (4,2)
        checkValues( kf[4], 4_ra, 2_ra );

        // Insert in-between using interpolation
        kf.insertInterpolatedKeyFrame( 2.75_ra, linearInterpolate<Scalar> );
        // There should still be 6 keyframes
        REQUIRE( kf.size() == 6 );
        // These should be sorted
        checkSorting( kf );
        // The first one should be (-2,1)
        checkValues( kf[0], -2_ra, 1_ra );
        // The second one should be (-1,1)
        checkValues( kf[1], -1_ra, 1_ra );
        // The third one should be (2.5,0)
        checkValues( kf[2], 2.5_ra, 0_ra );
        // The fourth one should be (2.75,1)
        checkValues( kf[3], 2.75_ra, 1_ra );
        // The fifth one should be (3,2)
        checkValues( kf[4], 3_ra, 2_ra );
        // The sixth one should be (4,2)
        checkValues( kf[5], 4_ra, 2_ra );

        // Insert after last using interpolation
        kf.insertInterpolatedKeyFrame( 5_ra, linearInterpolate<Scalar> );
        // There should still be 7 keyframes
        REQUIRE( kf.size() == 7 );
        // These should be sorted
        checkSorting( kf );
        // The first one should be (-2,1)
        checkValues( kf[0], -2_ra, 1_ra );
        // The second one should be (-1,1)
        checkValues( kf[1], -1_ra, 1_ra );
        // The third one should be (2.5,0)
        checkValues( kf[2], 2.5_ra, 0_ra );
        // The fourth one should be (2.75,1)
        checkValues( kf[3], 2.75_ra, 1_ra );
        // The fifth one should be (3,2)
        checkValues( kf[4], 3_ra, 2_ra );
        // The sixth one should be (4,2)
        checkValues( kf[5], 4_ra, 2_ra );
        // The seventh one should be (5,2)
        checkValues( kf[6], 5_ra, 2_ra );
    }
}

TEST_CASE( "Core/Animation/KeyFramedStruct" ) {
    //! [declare MyStruct]
    struct MyStruct {
        MyStruct() :
            m_nonAnimatedData { 2_ra },   // initialize the non animated data as 2
            m_animatedData { 1_ra, 0_ra } // creating the animated data with value 0 at time 1
        {
            m_animatedData.insertKeyFrame( 3_ra, 2_ra ); // adding a keyframe with value 2 at time 3
        }

        Scalar fetch( Scalar time ) {
            // fetch the interpolated value for the given time
            Scalar v_t = m_animatedData.at( time, Ra::Core::Animation::linearInterpolate<Scalar> );
            // use it
            return m_nonAnimatedData * v_t;
        }

        Scalar m_nonAnimatedData;
        Ra::Core::Animation::KeyFramedValue<Scalar> m_animatedData;
    };
    //! [declare MyStruct]
    //! [declare MyStructAnimator]

    struct MyStructAnimator {
        MyStructAnimator( MyStruct& s ) {
            // create the keyframes for the data
            auto frames = new Ra::Core::Animation::KeyFramedValue<Scalar> { 0_ra, 0_ra };
            frames->insertKeyFrame( 4_ra, 4_ra );
            // create the controller
            m_controller.m_value   = frames;
            m_controller.m_updater = [frames, &s]( const Scalar& t ) {
                // fetch the interpolated value for the given time
                auto v_t = frames->at( t, Ra::Core::Animation::linearInterpolate<Scalar> );
                // update the data
                s.m_nonAnimatedData = v_t;
            };
        }

        void update( Scalar time ) {
            m_controller.updateKeyFrame( time ); // uses the keyframes to update the data.
        }

        Ra::Core::Animation::KeyFramedValueController m_controller;
    };
    //! [declare MyStructAnimator]

    SECTION( "" ) {
        //! [use MyStruct]
        MyStruct a;
        std::cout << a.fetch( 0_ra ) << std::endl; // prints: 0
        std::cout << a.fetch( 2_ra ) << std::endl; // prints: 2
        std::cout << a.fetch( 4_ra ) << std::endl; // prints: 4
                                                   //! [use MyStruct]

        REQUIRE( Math::areApproxEqual( a.fetch( 0_ra ), 0_ra ) );
        REQUIRE( Math::areApproxEqual( a.fetch( 2_ra ), 2_ra ) );
        REQUIRE( Math::areApproxEqual( a.fetch( 4_ra ), 4_ra ) );
    }
    SECTION( "" ) {
        //! [use MyStructAnimator]
        MyStruct a; // a.m_nonAnimatedData = 2 in ctor
        MyStructAnimator b( a );
        std::cout << a.fetch( 0_ra ) << std::endl; // prints: 0
        std::cout << a.fetch( 2_ra ) << std::endl; // prints: 2
        std::cout << a.fetch( 4_ra ) << std::endl; // prints: 4
        b.update( 1_ra );                          // now: a.m_nonAnimatedData = 1
        std::cout << a.fetch( 0_ra ) << std::endl; // prints: 0
        std::cout << a.fetch( 2_ra ) << std::endl; // prints: 1
        std::cout << a.fetch( 4_ra ) << std::endl; // prints: 2
        b.update( 2_ra );                          // now: a.m_nonAnimatedData = 2
        std::cout << a.fetch( 0_ra ) << std::endl; // prints: 0
        std::cout << a.fetch( 2_ra ) << std::endl; // prints: 2
        std::cout << a.fetch( 4_ra ) << std::endl; // prints: 4
        b.update( 4_ra );                          // now: a.m_nonAnimatedData = 4
        std::cout << a.fetch( 0_ra ) << std::endl; // prints: 0
        std::cout << a.fetch( 2_ra ) << std::endl; // prints: 4
        std::cout << a.fetch( 4_ra ) << std::endl; // prints: 8

        //! [use MyStructAnimator]
        b.update( 1_ra );
        REQUIRE( Math::areApproxEqual( a.m_nonAnimatedData, 1_ra ) );
        REQUIRE( Math::areApproxEqual( a.fetch( 0_ra ), 0_ra ) );
        REQUIRE( Math::areApproxEqual( a.fetch( 2_ra ), 1_ra ) );
        REQUIRE( Math::areApproxEqual( a.fetch( 4_ra ), 2_ra ) );
        b.update( 2_ra );
        REQUIRE( Math::areApproxEqual( a.m_nonAnimatedData, 2_ra ) );
        REQUIRE( Math::areApproxEqual( a.fetch( 0_ra ), 0_ra ) );
        REQUIRE( Math::areApproxEqual( a.fetch( 2_ra ), 2_ra ) );
        REQUIRE( Math::areApproxEqual( a.fetch( 4_ra ), 4_ra ) );
        b.update( 4_ra );
        REQUIRE( Math::areApproxEqual( a.m_nonAnimatedData, 4_ra ) );
        REQUIRE( Math::areApproxEqual( a.fetch( 0_ra ), 0_ra ) );
        REQUIRE( Math::areApproxEqual( a.fetch( 2_ra ), 4_ra ) );
        REQUIRE( Math::areApproxEqual( a.fetch( 4_ra ), 8_ra ) );
    }
}

TEST_CASE( "Core/Animation/Skeleton", "[Core][Core/Animation][Skeleton]" ) {
    using Space = HandleArray::SpaceType;
    // build the skeleton in the X direction: > - > - > - > starting at the origin
    Skeleton skel;
    int root             = skel.addRoot( Transform::Identity(), "root" );
    Transform localT     = Transform::Identity();
    localT.translation() = Vector3::UnitX();
    int bone1            = skel.addBone( root, localT, Space::LOCAL, "bone1" );
    int bone2            = skel.addBone( bone1, localT, Space::LOCAL, "bone2" );
    int bone3            = skel.addBone( bone2, localT, Space::LOCAL, "bone3" );

    SECTION( "Test initialization" ) {
        // check root / leaf status
        REQUIRE( skel.m_graph.isRoot( root ) );
        REQUIRE( !skel.m_graph.isRoot( bone1 ) );
        REQUIRE( !skel.m_graph.isRoot( bone2 ) );
        REQUIRE( !skel.m_graph.isRoot( bone3 ) );
        REQUIRE( !skel.m_graph.isLeaf( root ) );
        REQUIRE( !skel.m_graph.isLeaf( bone1 ) );
        REQUIRE( !skel.m_graph.isLeaf( bone2 ) );
        REQUIRE( skel.m_graph.isLeaf( bone3 ) );
        // check hierarchy
        REQUIRE( skel.m_graph.parents().size() == 4 );
        REQUIRE( skel.m_graph.parents()[root] == -1 );
        REQUIRE( skel.m_graph.parents()[bone1] == root );
        REQUIRE( skel.m_graph.parents()[bone2] == bone1 );
        REQUIRE( skel.m_graph.parents()[bone3] == bone2 );
        REQUIRE( skel.m_graph.children().size() == 4 );
        REQUIRE( skel.m_graph.children()[root].size() == 1 );
        REQUIRE( skel.m_graph.children()[root][0] == bone1 );
        REQUIRE( skel.m_graph.children()[bone1].size() == 1 );
        REQUIRE( skel.m_graph.children()[bone1][0] == bone2 );
        REQUIRE( skel.m_graph.children()[bone2].size() == 1 );
        REQUIRE( skel.m_graph.children()[bone2][0] == bone3 );
        REQUIRE( skel.m_graph.children()[bone3].size() == 0 );
        // check Local Pose
        REQUIRE( areEqual( skel.getPose( Space::LOCAL ),
                           { Transform::Identity(), localT, localT, localT } ) );
        REQUIRE( skel.getTransform( root, Space::LOCAL ).isApprox( Transform::Identity() ) );
        REQUIRE( skel.getTransform( bone1, Space::LOCAL ).isApprox( localT ) );
        REQUIRE( skel.getTransform( bone2, Space::LOCAL ).isApprox( localT ) );
        REQUIRE( skel.getTransform( bone3, Space::LOCAL ).isApprox( localT ) );
        // check Model Pose
        Transform T1     = Transform::Identity();
        T1.translation() = Vector3::UnitX();
        Transform T2     = Transform::Identity();
        T2.translation() = 2 * Vector3::UnitX();
        Transform T3     = Transform::Identity();
        T3.translation() = 3 * Vector3::UnitX();
        REQUIRE( areEqual( skel.getPose( Space::MODEL ), { Transform::Identity(), T1, T2, T3 } ) );
        REQUIRE( skel.getTransform( root, Space::MODEL ).isApprox( Transform::Identity() ) );
        REQUIRE( skel.getTransform( bone1, Space::MODEL ).isApprox( T1 ) );
        REQUIRE( skel.getTransform( bone2, Space::MODEL ).isApprox( T2 ) );
        REQUIRE( skel.getTransform( bone3, Space::MODEL ).isApprox( T3 ) );
        // check endPoints
        Vector3 a, b;
        skel.getBonePoints( root, a, b );
        REQUIRE( a.isApprox( Vector3::Zero() ) );
        REQUIRE( b.isApprox( Vector3::UnitX() ) );
        skel.getBonePoints( bone1, a, b );
        REQUIRE( a.isApprox( Vector3::UnitX() ) );
        REQUIRE( b.isApprox( 2 * Vector3::UnitX() ) );
        skel.getBonePoints( bone2, a, b );
        REQUIRE( a.isApprox( 2 * Vector3::UnitX() ) );
        REQUIRE( b.isApprox( 3 * Vector3::UnitX() ) );
        skel.getBonePoints( bone3, a, b );
        REQUIRE( a.isApprox( 3 * Vector3::UnitX() ) );
        REQUIRE( b.isApprox( 3 * Vector3::UnitX() ) );
    }

    SECTION( "Test Forward Manipulation" ) {
        /*                                                          > - - v
         * Here we will pose the skeleton from                 to   |     |
         *                                                          ^     |
         *                                       > - > - > - >      +     <
         * (in the X-Y plane) where + is the origin, using Forward manipulation
         */

        // reminder of the current transforms:
        Transform T       = Transform::Identity();
        Transform T1      = Transform::Identity();
        T1.translation()  = Vector3::UnitX();
        Transform T2      = Transform::Identity();
        T2.translation()  = 2 * Vector3::UnitX();
        Transform T3      = Transform::Identity();
        T3.translation()  = 3 * Vector3::UnitX();
        Transform localT1 = localT;
        Transform localT2 = localT;
        Transform localT3 = localT;

        REQUIRE( skel.m_manipulation == Skeleton::FORWARD );

        SECTION( "using only Model Space" ) {
            /* first rotate and move root:                 =>  ^
             *                              > - > - > - >      + ` > - > - >
             */
            T.rotate( AngleAxis( Math::Pi / 2, Vector3::UnitZ() ) );
            T.translation() = Vector3::UnitY();
            skel.setTransform( root, T, Space::MODEL );
            // check Pose
            localT1 = Transform::Identity();
            localT1.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            localT1.translation() = -Vector3::UnitX() - Vector3::UnitY();
            REQUIRE( areEqual( skel.getPose( Space::LOCAL ), { T, localT1, localT2, localT3 } ) );
            REQUIRE( areEqual( skel.getPose( Space::MODEL ), { T, T1, T2, T3 } ) );

            /*                                      >
             * then move bone1:                 =>  | \
             *                   ^                  ^   \
             *                   + ` > - > - >      +     > - >
             */
            T1               = Transform::Identity();
            T1.translation() = 2 * Vector3::UnitY();
            skel.setTransform( bone1, T1, Space::MODEL );
            // check Pose
            localT1 = Transform::Identity();
            localT1.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            localT1.translation() = Vector3::UnitX();
            localT2               = Transform::Identity();
            localT2.translation() = 2 * Vector3::UnitX() - 2 * Vector3::UnitY();
            REQUIRE( areEqual( skel.getPose( Space::LOCAL ), { T, localT1, localT2, localT3 } ) );
            REQUIRE( areEqual( skel.getPose( Space::MODEL ), { T, T1, T2, T3 } ) );

            /*                             >                > - - v
             * then rotate and move bone2: | \          =>  |
             *                             ^   \            ^       \
             *                             +     > - >      +         >
             */
            T2 = Transform::Identity();
            T2.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            T2.translation() = 2 * Vector3::UnitX() + 2 * Vector3::UnitY();
            skel.setTransform( bone2, T2, Space::MODEL );
            // check Pose
            localT2 = Transform::Identity();
            localT2.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            localT2.translation() = 2 * Vector3::UnitX();
            localT3               = Transform::Identity();
            localT3.rotate( AngleAxis( Math::Pi / 2, Vector3::UnitZ() ) );
            localT3.translation() = 2 * Vector3::UnitX() + Vector3::UnitY();
            REQUIRE( areEqual( skel.getPose( Space::LOCAL ), { T, localT1, localT2, localT3 } ) );
            REQUIRE( areEqual( skel.getPose( Space::MODEL ), { T, T1, T2, T3 } ) );

            /*                        > - - v          > - - v
             * finally rotate bone3:  |            =>  |     |
             *                        ^       \        ^     |
             *                        +         >      +     <
             */
            T3 = Transform::Identity();
            T3.rotate( AngleAxis( Math::Pi, Vector3::UnitZ() ) );
            T3.translation() = 2 * Vector3::UnitX();
            skel.setTransform( bone3, T3, Space::MODEL );
            // check Pose
            localT3 = Transform::Identity();
            localT3.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            localT3.translation() = 2 * Vector3::UnitX();
            REQUIRE( areEqual( skel.getPose( Space::LOCAL ), { T, localT1, localT2, localT3 } ) );
            REQUIRE( areEqual( skel.getPose( Space::MODEL ), { T, T1, T2, T3 } ) );
        }

        SECTION( "using only Local Space" ) {
            /*                                                 ^
             *                                                 |
             *                                                 ^
             *                                                 |
             *                                                 ^
             *                                                 |
             * first rotate and move root:                 =>  ^
             *                              > - > - > - >      +
             */
            T = Transform::Identity();
            T.rotate( AngleAxis( Math::Pi / 2, Vector3::UnitZ() ) );
            T.translation() = Vector3::UnitY();
            skel.setTransform( root, T, Space::LOCAL );
            // check Pose
            REQUIRE( areEqual( skel.getPose( Space::LOCAL ), { T, localT1, localT2, localT3 } ) );
            T1 = Transform::Identity();
            T1.rotate( AngleAxis( Math::Pi / 2, Vector3::UnitZ() ) );
            T1.translation() = 2 * Vector3::UnitY();
            T2               = Transform::Identity();
            T2.rotate( AngleAxis( Math::Pi / 2, Vector3::UnitZ() ) );
            T2.translation() = 3 * Vector3::UnitY();
            T3               = Transform::Identity();
            T3.rotate( AngleAxis( Math::Pi / 2, Vector3::UnitZ() ) );
            T3.translation() = 4 * Vector3::UnitY();
            REQUIRE( areEqual( skel.getPose( Space::MODEL ), { T, T1, T2, T3 } ) );

            /*                     ^
             *                     |
             *                     ^
             *                     |
             *                     ^       > - > - >
             *                     |       |
             * then rotate bone1:  ^   =>  ^
             *                     +       +
             */
            localT1 = Transform::Identity();
            localT1.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            localT1.translation() = Vector3::UnitX();
            skel.setTransform( bone1, localT1, Space::LOCAL );
            // check Pose
            REQUIRE( areEqual( skel.getPose( Space::LOCAL ), { T, localT1, localT2, localT3 } ) );
            T1               = Transform::Identity();
            T1.translation() = 2 * Vector3::UnitY();
            T2               = Transform::Identity();
            T2.translation() = Vector3::UnitX() + 2 * Vector3::UnitY();
            T3               = Transform::Identity();
            T3.translation() = 2 * Vector3::UnitX() + 2 * Vector3::UnitY();
            REQUIRE( areEqual( skel.getPose( Space::MODEL ), { T, T1, T2, T3 } ) );

            /*                              > - > - >      > - - v
             *                              |              |     |
             * then rotate and move bone2:  ^          =>  ^     v
             *                              +              +
             */
            localT2 = Transform::Identity();
            localT2.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            localT2.translation() = 2 * Vector3::UnitX();
            skel.setTransform( bone2, localT2, Space::LOCAL );
            // check Pose
            REQUIRE( areEqual( skel.getPose( Space::LOCAL ), { T, localT1, localT2, localT3 } ) );
            T2 = Transform::Identity();
            T2.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            T2.translation() = 2 * Vector3::UnitX() + 2 * Vector3::UnitY();
            T3               = Transform::Identity();
            T3.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            T3.translation() = 2 * Vector3::UnitX() + Vector3::UnitY();
            REQUIRE( areEqual( skel.getPose( Space::MODEL ), { T, T1, T2, T3 } ) );

            /*                                 > - - v      > - - v
             *                                 |     |      |     |
             * finally rotate and move bone3:  ^     v  =>  ^     |
             *                                 +            +     <
             */
            localT3 = Transform::Identity();
            localT3.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            localT3.translation() = 2 * Vector3::UnitX();
            skel.setTransform( bone3, localT3, Space::LOCAL );
            // check Pose
            REQUIRE( areEqual( skel.getPose( Space::LOCAL ), { T, localT1, localT2, localT3 } ) );
            T3 = Transform::Identity();
            T3.rotate( AngleAxis( Math::Pi, Vector3::UnitZ() ) );
            T3.translation() = 2 * Vector3::UnitX();
            REQUIRE( areEqual( skel.getPose( Space::MODEL ), { T, T1, T2, T3 } ) );
        }

        SECTION( "using Both Spaces" ) {
            /* first rotate and move root in Model Space:                 =>  ^
             *                                             > - > - > - >      + ` > - > - >
             */
            T.rotate( AngleAxis( Math::Pi / 2, Vector3::UnitZ() ) );
            T.translation() = Vector3::UnitY();
            skel.setTransform( root, T, Space::MODEL );
            // check Pose
            localT1 = Transform::Identity();
            localT1.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            localT1.translation() = -Vector3::UnitX() - Vector3::UnitY();
            REQUIRE( areEqual( skel.getPose( Space::LOCAL ), { T, localT1, localT2, localT3 } ) );
            REQUIRE( areEqual( skel.getPose( Space::MODEL ), { T, T1, T2, T3 } ) );

            /*                                                     > - > - >
             *                                                     |
             * then move bone1 in Local Space:  ^              =>  ^
             *                                  + ` > - > - >      +
             */
            localT1 = Transform::Identity();
            localT1.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            localT1.translation() = Vector3::UnitX();
            skel.setTransform( bone1, localT1, Space::LOCAL );
            // check Pose
            REQUIRE( areEqual( skel.getPose( Space::LOCAL ), { T, localT1, localT2, localT3 } ) );
            T1               = Transform::Identity();
            T1.translation() = 2 * Vector3::UnitY();
            T2               = Transform::Identity();
            T2.translation() = Vector3::UnitX() + 2 * Vector3::UnitY();
            T3               = Transform::Identity();
            T3.translation() = 2 * Vector3::UnitX() + 2 * Vector3::UnitY();
            REQUIRE( areEqual( skel.getPose( Space::MODEL ), { T, T1, T2, T3 } ) );

            /*
             *                                             > - > - >      > - - v>
             *                                             |              |
             * then rotate and move bone2 in Model Space:  ^          =>  ^
             *                                             +              +
             */
            T2 = Transform::Identity();
            T2.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            T2.translation() = 2 * Vector3::UnitX() + 2 * Vector3::UnitY();
            skel.setTransform( bone2, T2, Space::MODEL );
            // check Pose
            localT2 = Transform::Identity();
            localT2.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            localT2.translation() = 2 * Vector3::UnitX();
            localT3               = Transform::Identity();
            localT3.rotate( AngleAxis( Math::Pi / 2, Vector3::UnitZ() ) );
            REQUIRE( areEqual( skel.getPose( Space::LOCAL ), { T, localT1, localT2, localT3 } ) );
            REQUIRE( areEqual( skel.getPose( Space::MODEL ), { T, T1, T2, T3 } ) );

            /*                                                > - - v>      > - - v
             *                                                |             |     |
             * finally rotate and move bone3 in Local Space:  ^         =>  ^     |
             *                                                +             +     <
             */
            localT3 = Transform::Identity();
            localT3.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            localT3.translation() = 2 * Vector3::UnitX();
            skel.setTransform( bone3, localT3, Space::LOCAL );
            // check Pose
            REQUIRE( areEqual( skel.getPose( Space::LOCAL ), { T, localT1, localT2, localT3 } ) );
            T3 = Transform::Identity();
            T3.rotate( AngleAxis( Math::Pi, Vector3::UnitZ() ) );
            T3.translation() = 2 * Vector3::UnitX();
            REQUIRE( areEqual( skel.getPose( Space::MODEL ), { T, T1, T2, T3 } ) );
        }
    }

    SECTION( "Test Pseudo-IK Manipulation" ) {
        /*                                                          > - - v
         * Here we will pose the skeleton from                 to   |     |
         *                                                          ^     |
         *                                       > - > - > - >      +     <
         * (in the X-Y plane) where + is the origin, using Pseudo-IK manipulation
         */

        // set Pseudo-IK mode
        skel.m_manipulation = Skeleton::PSEUDO_IK;

        // reminder of the current transforms:
        Transform T       = Transform::Identity();
        Transform T1      = Transform::Identity();
        T1.translation()  = Vector3::UnitX();
        Transform T2      = Transform::Identity();
        T2.translation()  = 2 * Vector3::UnitX();
        Transform T3      = Transform::Identity();
        T3.translation()  = 3 * Vector3::UnitX();
        Transform localT1 = localT;
        Transform localT2 = localT;
        Transform localT3 = localT;

        SECTION( "with Model Space" ) {
            /*                                                 ^
             *                                                 |
             *                                                 ^
             *                                                 |
             *                                                 ^
             *                                                 |
             * first rotate and move root:                 =>  ^
             *                              > - > - > - >      +
             */
            T = Transform::Identity();
            T.rotate( AngleAxis( Math::Pi / 2, Vector3::UnitZ() ) );
            T.translation() = Vector3::UnitY();
            skel.setTransform( root, T, Space::MODEL );
            // check Pose
            REQUIRE( areEqual( skel.getPose( Space::LOCAL ), { T, localT1, localT2, localT3 } ) );
            T1 = Transform::Identity();
            T1.rotate( AngleAxis( Math::Pi / 2, Vector3::UnitZ() ) );
            T1.translation() = 2 * Vector3::UnitY();
            T2               = Transform::Identity();
            T2.rotate( AngleAxis( Math::Pi / 2, Vector3::UnitZ() ) );
            T2.translation() = 3 * Vector3::UnitY();
            T3               = Transform::Identity();
            T3.rotate( AngleAxis( Math::Pi / 2, Vector3::UnitZ() ) );
            T3.translation() = 4 * Vector3::UnitY();
            REQUIRE( areEqual( skel.getPose( Space::MODEL ), { T, T1, T2, T3 } ) );

            /*                              ^
             *                              |
             *                              ^
             *                              |
             *                              ^      > - - v
             *                              |      |     |
             * then rotate and move bone2:  ^  =>  ^     v
             *                              +      +
             * hence rotating bone1
             */
            T2 = Transform::Identity();
            T2.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            T2.translation() = 2 * Vector3::UnitX() + 2 * Vector3::UnitY();
            skel.setTransform( bone2, T2, Space::MODEL );
            // check Pose
            localT1 = Transform::Identity();
            localT1.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            localT1.translation() = Vector3::UnitX();
            localT2               = Transform::Identity();
            localT2.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            localT2.translation() = 2 * Vector3::UnitX();
            REQUIRE( areEqual( skel.getPose( Space::LOCAL ), { T, localT1, localT2, localT3 } ) );
            T1               = Transform::Identity();
            T1.translation() = 2 * Vector3::UnitY();
            T3               = Transform::Identity();
            T3.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            T3.translation() = 2 * Vector3::UnitX() + Vector3::UnitY();
            REQUIRE( areEqual( skel.getPose( Space::MODEL ), { T, T1, T2, T3 } ) );

            /*                                 > - - v      > - - v
             *                                 |     |      |     |
             * finally rotate and move bone3:  ^     v  =>  ^     |
             *                                 +            +     <
             */
            T3 = Transform::Identity();
            T3.rotate( AngleAxis( Math::Pi, Vector3::UnitZ() ) );
            T3.translation() = 2 * Vector3::UnitX();
            skel.setTransform( bone3, T3, Space::MODEL );
            // check Pose
            localT3 = Transform::Identity();
            localT3.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            localT3.translation() = 2 * Vector3::UnitX();
            REQUIRE( areEqual( skel.getPose( Space::LOCAL ), { T, localT1, localT2, localT3 } ) );
            REQUIRE( areEqual( skel.getPose( Space::MODEL ), { T, T1, T2, T3 } ) );
        }

        SECTION( "with Local Space" ) {
            /*                                                 ^
             *                                                 |
             *                                                 ^
             *                                                 |
             *                                                 ^
             *                                                 |
             * first rotate and move root:                 =>  ^
             *                              > - > - > - >      +
             */
            T = Transform::Identity();
            T.rotate( AngleAxis( Math::Pi / 2, Vector3::UnitZ() ) );
            T.translation() = Vector3::UnitY();
            skel.setTransform( root, T, Space::LOCAL );
            // check Pose
            REQUIRE( areEqual( skel.getPose( Space::LOCAL ), { T, localT1, localT2, localT3 } ) );
            T1 = Transform::Identity();
            T1.rotate( AngleAxis( Math::Pi / 2, Vector3::UnitZ() ) );
            T1.translation() = 2 * Vector3::UnitY();
            T2               = Transform::Identity();
            T2.rotate( AngleAxis( Math::Pi / 2, Vector3::UnitZ() ) );
            T2.translation() = 3 * Vector3::UnitY();
            T3               = Transform::Identity();
            T3.rotate( AngleAxis( Math::Pi / 2, Vector3::UnitZ() ) );
            T3.translation() = 4 * Vector3::UnitY();
            REQUIRE( areEqual( skel.getPose( Space::MODEL ), { T, T1, T2, T3 } ) );

            /*                              ^
             *                              |
             *                              ^
             *                              |
             *                              ^      > - - v
             *                              |      |     |
             * then rotate and move bone2:  ^  =>  ^     v
             *                              +      +
             * hence rotating bone1 in the process
             */
            // note: we express localT2 as we expect it to be if bone1 is not rotated!
            localT2 = Transform::Identity();
            localT2.rotate( AngleAxis( -Math::Pi, Vector3::UnitZ() ) );
            localT2.translation() = -2 * Vector3::UnitY();
            skel.setTransform( bone2, localT2, Space::LOCAL );
            // note: the local transform of bone 2 is not localT2 anymore since we
            //       rotated bone1 thanks to the Pseudo-IK!
            // check Pose
            localT1 = Transform::Identity();
            localT1.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            localT1.translation() = Vector3::UnitX();
            localT2               = Transform::Identity();
            localT2.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            localT2.translation() = 2 * Vector3::UnitX();
            REQUIRE( areEqual( skel.getPose( Space::LOCAL ), { T, localT1, localT2, localT3 } ) );
            T1               = Transform::Identity();
            T1.translation() = 2 * Vector3::UnitY();
            T2               = Transform::Identity();
            T2.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            T2.translation() = 2 * Vector3::UnitX() + 2 * Vector3::UnitY();
            T3               = Transform::Identity();
            T3.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            T3.translation() = 2 * Vector3::UnitX() + Vector3::UnitY();
            REQUIRE( areEqual( skel.getPose( Space::MODEL ), { T, T1, T2, T3 } ) );

            /*                                 > - - v      > - - v
             *                                 |     |      |     |
             * finally rotate and move bone3:  ^     v  =>  ^     |
             *                                 +            +     <
             */
            localT3 = Transform::Identity();
            localT3.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            localT3.translation() = 2 * Vector3::UnitX();
            skel.setTransform( bone3, localT3, Space::LOCAL );
            // check Pose
            REQUIRE( areEqual( skel.getPose( Space::LOCAL ), { T, localT1, localT2, localT3 } ) );
            T3 = Transform::Identity();
            T3.rotate( AngleAxis( -Math::Pi, Vector3::UnitZ() ) );
            T3.translation() = 2 * Vector3::UnitX();
            REQUIRE( areEqual( skel.getPose( Space::MODEL ), { T, T1, T2, T3 } ) );
        }

        SECTION( "with Both Spaces" ) {
            /*                                                 ^
             *                                                 |
             *                                                 ^
             *                                                 |
             *                                                 ^
             *                                                 |
             * first rotate and move root:                 =>  ^
             *                              > - > - > - >      +
             */
            T = Transform::Identity();
            T.rotate( AngleAxis( Math::Pi / 2, Vector3::UnitZ() ) );
            T.translation() = Vector3::UnitY();
            skel.setTransform( root, T, Space::MODEL );
            // check Pose
            REQUIRE( areEqual( skel.getPose( Space::LOCAL ), { T, localT1, localT2, localT3 } ) );
            T1 = Transform::Identity();
            T1.rotate( AngleAxis( Math::Pi / 2, Vector3::UnitZ() ) );
            T1.translation() = 2 * Vector3::UnitY();
            T2               = Transform::Identity();
            T2.rotate( AngleAxis( Math::Pi / 2, Vector3::UnitZ() ) );
            T2.translation() = 3 * Vector3::UnitY();
            T3               = Transform::Identity();
            T3.rotate( AngleAxis( Math::Pi / 2, Vector3::UnitZ() ) );
            T3.translation() = 4 * Vector3::UnitY();
            REQUIRE( areEqual( skel.getPose( Space::MODEL ), { T, T1, T2, T3 } ) );

            /*                              ^
             *                              |
             *                              ^
             *                              |
             *                              ^      > - - v
             *                              |      |     |
             * then rotate and move bone2:  ^  =>  ^     v
             *                              +      +
             * hence rotating bone1 in the process
             */
            // note: we express localT2 as we expect it to be if bone1 is not rotated!
            localT2 = Transform::Identity();
            localT2.rotate( AngleAxis( -Math::Pi, Vector3::UnitZ() ) );
            localT2.translation() = -2 * Vector3::UnitY();
            skel.setTransform( bone2, localT2, Space::LOCAL );
            // note: the local transform of bone 2 is not localT2 anymore since we
            //       rotated bone1 thanks to the Pseudo-IK!
            // check Pose
            localT1 = Transform::Identity();
            localT1.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            localT1.translation() = Vector3::UnitX();
            localT2               = Transform::Identity();
            localT2.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            localT2.translation() = 2 * Vector3::UnitX();
            REQUIRE( areEqual( skel.getPose( Space::LOCAL ), { T, localT1, localT2, localT3 } ) );
            T1               = Transform::Identity();
            T1.translation() = 2 * Vector3::UnitY();
            T2               = Transform::Identity();
            T2.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            T2.translation() = 2 * Vector3::UnitX() + 2 * Vector3::UnitY();
            T3               = Transform::Identity();
            T3.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            T3.translation() = 2 * Vector3::UnitX() + Vector3::UnitY();
            REQUIRE( areEqual( skel.getPose( Space::MODEL ), { T, T1, T2, T3 } ) );

            /*                                 > - - v      > - - v
             *                                 |     |      |     |
             * finally rotate and move bone3:  ^     v  =>  ^     |
             *                                 +            +     <
             */
            T3 = Transform::Identity();
            T3.rotate( AngleAxis( Math::Pi, Vector3::UnitZ() ) );
            T3.translation() = 2 * Vector3::UnitX();
            skel.setTransform( bone3, T3, Space::MODEL );
            // check Pose
            localT3 = Transform::Identity();
            localT3.rotate( AngleAxis( -Math::Pi / 2, Vector3::UnitZ() ) );
            localT3.translation() = 2 * Vector3::UnitX();
            REQUIRE( areEqual( skel.getPose( Space::LOCAL ), { T, localT1, localT2, localT3 } ) );
            REQUIRE( areEqual( skel.getPose( Space::MODEL ), { T, T1, T2, T3 } ) );
        }
    }
}

TEST_CASE( "Core/Animation/DualQuaternionSkinning",
           "[Core][Core/Animation][DualQuaternionSkinning]" ) {
    // initialize the pose
    Ra::Core::Animation::Pose pose;
    Ra::Core::Transform t0 { Ra::Core::Transform::Identity() };
    Ra::Core::Transform t1(
        Eigen::AngleAxis( Scalar( -M_PI / 4. ), Ra::Core::Vector3 { 0_ra, 1_ra, 0_ra } ) );
    pose.push_back( t0 );
    pose.push_back( t1 );
    // define a simple mesh
    Ra::Core::Vector3Array vertices { { 0_ra, 0_ra, 0_ra },
                                      { 0_ra, 1_ra, 0_ra },
                                      { 1_ra, 0_ra, 0_ra },
                                      { 1_ra, 1_ra, 0_ra },
                                      { 2_ra, 0_ra, 0_ra },
                                      { 2_ra, 1_ra, 0_ra } };
    // define the weight matrix
    Ra::Core::Animation::WeightMatrix weights( 6, 2 );
    // M( i, j ) = w   , if vertex i is influenced by transform j ;
    // influence of bone 0
    weights.insert( 0, 0 ) = 1_ra;
    weights.insert( 1, 0 ) = 1_ra;
    weights.insert( 2, 0 ) = 0.5_ra;
    weights.insert( 3, 0 ) = 0.5_ra;
    // influence of bone 1
    weights.insert( 2, 1 ) = 0.5_ra;
    weights.insert( 3, 1 ) = 0.5_ra;
    weights.insert( 4, 1 ) = 1_ra;
    weights.insert( 5, 1 ) = 1_ra;

    // Interpolate the transformations
    Ra::Core::Quaternion q0( t0.linear() );
    Ra::Core::Quaternion q1( t1.linear() );
    Ra::Core::Quaternion q3 = q0.slerp( 0.5_ra, q1 );

    // test dual quaternions
    auto dq = Ra::Core::Animation::computeDQ( pose, weights );
    REQUIRE( q3.toRotationMatrix().isApprox( dq[2].getTransform().linear() ) );

    // apply and verify dualquaternion deformation
    auto sk = Ra::Core::Animation::applyDualQuaternions( dq, vertices );
    REQUIRE( vertices[0].isApprox( sk[0] ) );
    Ra::Core::Transform t( q3 );
    auto vt = t * vertices[2];
    REQUIRE( vt.isApprox( sk[2] ) );
    auto vt1 = t1 * vertices[4];
    REQUIRE( vt1.isApprox( sk[4] ) );

    // test naive dual quaternions
    auto dq_n = Ra::Core::Animation::computeDQ( pose, weights );
    REQUIRE( q3.toRotationMatrix().isApprox( dq_n[2].getTransform().linear() ) );
}
