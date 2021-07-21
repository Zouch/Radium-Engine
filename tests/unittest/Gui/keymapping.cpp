#include <Core/Resources/Resources.hpp>
#include <Core/Utils/StdFilesystem.hpp>
#include <Gui/Utils/KeyMappingManager.hpp>

#include <catch2/catch.hpp>

#include <QtGlobal>

using namespace Ra;
using namespace Ra::Gui;
using Idx = KeyMappingManager::KeyMappingAction;
using Ctx = KeyMappingManager::Context;

class Dummy : public KeyMappingManageable<Dummy>
{
  public:
    Dummy();
    friend class KeyMappingManageable<Dummy>;

  private:
    bool checkIntegrity( const std::string& mess ) const;
    static void configureKeyMapping_impl();

    // here in public for testing purpose
  public:
#define KeyMappingDummy \
    KMA_VALUE( TEST1 )  \
    KMA_VALUE( TEST2 )

#define KMA_VALUE( XX ) static KeyMappingManager::KeyMappingAction XX;
    KeyMappingDummy
#undef KMA_VALUE
};

using KeyMapping = KeyMappingManageable<Dummy>;

#define KMA_VALUE( XX ) Gui::KeyMappingManager::KeyMappingAction Dummy::XX;
KeyMappingDummy
#undef KMA_VALUE

    void
    Dummy::configureKeyMapping_impl() {

    KeyMapping::setContext( Gui::KeyMappingManager::getInstance()->getContext( "DummyContext" ) );
    if ( Dummy::getContext().isInvalid() ) {
        LOG( Ra::Core::Utils::logINFO )
            << "DummyContext not defined (maybe the configuration file do not contains it)";
        return;
    }

#define KMA_VALUE( XX ) \
    XX = Gui::KeyMappingManager::getInstance()->getActionIndex( KeyMapping::getContext(), #XX );
    KeyMappingDummy
#undef KMA_VALUE
}

TEST_CASE( "Gui/Utils/KeyMappingManager", "[Gui][Gui/Utils][KeyMappingManager]" ) {
    QCoreApplication::setOrganizationName( "RadiumUnitTests" );
    QCoreApplication::setApplicationName( "KeyMappingManager" );
    QSettings settings;
    settings.clear();
    KeyMappingManager::createInstance();
    auto mgr = Gui::KeyMappingManager::getInstance();
    auto optionalPath { Core::Resources::getRadiumResourcesPath() };
    auto resourcesRootDir { optionalPath.value_or( "[[Default resrouces path not found]]" ) };
    ///\todo how to check here ?
    auto defaultConfigFile = resourcesRootDir + std::string( "Configs/default.xml" );

    SECTION( "key mapping file load" ) {
        REQUIRE( defaultConfigFile == mgr->getLoadedFilename() );
        mgr->loadConfiguration( "dummy" );
        REQUIRE( defaultConfigFile == mgr->getLoadedFilename() );
        std::cout << std::filesystem::current_path() << "\n";
        mgr->loadConfiguration( "data/keymapping-valid.xml" );
        REQUIRE( "data/keymapping-valid.xml" == mgr->getLoadedFilename() );
        // invalid xml should not be loaded, and switch to default
        mgr->loadConfiguration( "data/keymapping-invalid.xml" );
        REQUIRE( defaultConfigFile == mgr->getLoadedFilename() );
        // while config error are loaded, with error message
        mgr->loadConfiguration( "data/keymapping-double-actions.xml" );
        REQUIRE( "data/keymapping-double-actions.xml" == mgr->getLoadedFilename() );
        // bad tag load defaults
        mgr->loadConfiguration( "data/keymapping-bad-tag.xml" );
        REQUIRE( defaultConfigFile == mgr->getLoadedFilename() );
        // bad main tag loads with a warning
        mgr->loadConfiguration( "data/keymapping-bad-main.xml" );
        REQUIRE( "data/keymapping-bad-main.xml" == mgr->getLoadedFilename() );
    }

    SECTION( "getAction" ) {
        mgr->loadConfiguration( "data/keymapping-valid.xml" );
        // check context
        auto cameraContext { mgr->getContext( "CameraContext" ) };
        REQUIRE( cameraContext.isValid() );
        REQUIRE( mgr->getContextName( cameraContext ) == "CameraContext" );

        auto viewerContext { mgr->getContext( "ViewerContext" ) };
        REQUIRE( viewerContext.isValid() );
        REQUIRE( viewerContext != cameraContext );

        auto invalidContext { mgr->getContext( "InvalidContext" ) };
        REQUIRE( invalidContext.isInvalid() );
        // invalidContext index returns "Invalid" context name
        REQUIRE( mgr->getContextName( Ctx {} ) == "Invalid" );
        REQUIRE( mgr->getContextName( Ctx { 42 } ) == "Invalid" );

        // test on action
        auto validAction { mgr->getAction( cameraContext, Qt::LeftButton, Qt::NoModifier, -1 ) };
        REQUIRE( validAction.isValid() );
        REQUIRE( validAction == mgr->getActionIndex( cameraContext, "TRACKBALLCAMERA_ROTATE" ) );
        REQUIRE( mgr->getActionIndex( Ctx {}, "TRACKBALLCAMERA_ROTATE" ).isInvalid() );
        REQUIRE( mgr->getActionIndex( Ctx { 42 }, "TRACKBALLCAMERA_ROTATE" ).isInvalid() );
        REQUIRE( mgr->getActionName( cameraContext, validAction ) == "TRACKBALLCAMERA_ROTATE" );

        // invalid action index returns "Invalid" action name
        REQUIRE( mgr->getActionName( cameraContext, Idx {} ) == "Invalid" );

        // modifiers as key are ignored
        validAction = mgr->getAction( cameraContext, Qt::LeftButton, Qt::ShiftModifier, -1 );
        auto action2 {
            mgr->getAction( cameraContext, Qt::LeftButton, Qt::ShiftModifier, Qt::Key_Shift ) };
        REQUIRE( validAction == action2 );

        // tests some invalid actions
        auto invalidAction { mgr->getAction( cameraContext, Qt::LeftButton, Qt::AltModifier, -1 ) };
        REQUIRE( invalidAction.isInvalid() );
        REQUIRE( invalidAction != mgr->getActionIndex( cameraContext, "TRACKBALLCAMERA_ROTATE" ) );
        REQUIRE( mgr->getAction( Idx {}, Qt::LeftButton, Qt::AltModifier, -1 ).isInvalid() );
        REQUIRE( mgr->getAction( Idx {}, Qt::LeftButton, Qt::AltModifier, 1 ).isInvalid() );
        REQUIRE( mgr->getAction( Idx {}, Qt::LeftButton, Qt::NoModifier, -1 ).isInvalid() );
        REQUIRE( mgr->getAction( Idx {}, Qt::RightButton, Qt::AltModifier, -1 ).isInvalid() );

        // with key and modifiers
        validAction = mgr->getAction( viewerContext, Qt::RightButton, Qt::NoModifier, Qt::Key_V );
        REQUIRE( validAction.isValid() );
        REQUIRE( validAction == mgr->getActionIndex( viewerContext, "VIEWER_PICKING_VERTEX" ) );

        // action index
        REQUIRE( mgr->getActionIndex( viewerContext, "UnkownAction" ).isInvalid() );
        validAction = mgr->getAction( viewerContext, Qt::NoButton, Qt::ControlModifier, Qt::Key_W );
        REQUIRE( validAction.isValid() );
        REQUIRE( validAction == mgr->getActionIndex( viewerContext, "VIEWER_TOGGLE_WIREFRAME" ) );

        // action are context dependent
        invalidAction = mgr->getAction( viewerContext, Qt::LeftButton, Qt::NoModifier, -1 );
        REQUIRE( invalidAction.isInvalid() );
        REQUIRE( mgr->getActionIndex( cameraContext, "VIEWER_TOGGLE_WIREFRAME" ).isInvalid() );
    }

    SECTION( "listener" ) {
        // dummy1.xml
        // LeftButton triggers TEST1, RightButton triggers TEST2
        // (with index TEST1 : 0, TEST2: 1)
        // dummy2.xml is the other way round dummy2.xml
        // LeftButton triggers TEST2, RightButton triggers TEST1
        // (with index TEST2: 0, TEST1: 1)

        auto didx = mgr->addListener( Dummy::configureKeyMapping );
        mgr->loadConfiguration( "data/dummy1.xml" );

        // action index correspond to configuration
        auto test1Idx = mgr->getActionIndex( Dummy::getContext(), "TEST1" );
        auto test2Idx = mgr->getActionIndex( Dummy::getContext(), "TEST2" );

        REQUIRE( test1Idx ==
                 mgr->getAction( Dummy::getContext(), Qt::LeftButton, Qt::NoModifier, -1 ) );
        REQUIRE( test2Idx ==
                 mgr->getAction( Dummy::getContext(), Qt::RightButton, Qt::NoModifier, -1 ) );
        // and set in Dummy class
        REQUIRE( Dummy::TEST1 == test1Idx );
        REQUIRE( Dummy::TEST2 == test2Idx );

        // reload trigger configureKeyMapping and update binding/action index
        mgr->loadConfiguration( "data/dummy2.xml" );
        auto test1Idx2 = mgr->getActionIndex( Dummy::getContext(), "TEST1" );
        auto test2Idx2 = mgr->getActionIndex( Dummy::getContext(), "TEST2" );

        // action index have been updated in Dummy class (by observation)
        REQUIRE( Dummy::TEST1 ==
                 mgr->getAction( Dummy::getContext(), Qt::RightButton, Qt::NoModifier, -1 ) );
        REQUIRE( Dummy::TEST2 ==
                 mgr->getAction( Dummy::getContext(), Qt::LeftButton, Qt::NoModifier, -1 ) );
        // and do not corresponds to the old one
        REQUIRE( test1Idx != Dummy::TEST1 );
        REQUIRE( test2Idx != Dummy::TEST2 );

        // remove listener and relaod do not update action index
        mgr->removeListener( didx );
        mgr->loadConfiguration( "data/dummy1.xml" );
        // action index have been reverted
        REQUIRE( test1Idx ==
                 mgr->getAction( Dummy::getContext(), Qt::LeftButton, Qt::NoModifier, -1 ) );
        REQUIRE( test2Idx ==
                 mgr->getAction( Dummy::getContext(), Qt::RightButton, Qt::NoModifier, -1 ) );
        // but not updated in Dummy class
        REQUIRE( Dummy::TEST1 != test1Idx );
        REQUIRE( Dummy::TEST1 == test1Idx2 );
        REQUIRE( Dummy::TEST2 != test2Idx );
        REQUIRE( Dummy::TEST2 == test2Idx2 );
    }

    SECTION( "Custom context and actions" ) {
        auto nonExistingcontext = mgr->getContext( "Keymapping::CustomContext" );
        REQUIRE( nonExistingcontext.isInvalid() );
        auto customContext = mgr->addContext( "Keymapping::CustomContext" );
        REQUIRE( customContext.isValid() );
        auto replicateContext = mgr->addContext( "Keymapping::CustomContext" );
        REQUIRE( replicateContext == customContext );
        auto customAction = mgr->addAction(
            "Keymapping::CustomContext", "CustomAction", "Key_F1", "", "", "", false );
        REQUIRE( customAction.isValid() );
        auto customActionDuplicate = mgr->addAction(
            "Keymapping::CustomContext", "CustomAction", "Key_F1", "", "", "", false );
        REQUIRE( customActionDuplicate.isValid() );
        REQUIRE( customActionDuplicate == customAction );
        auto customActionOtherBinding = mgr->addAction(
            "Keymapping::CustomContext", "CustomAction", "Key_F2", "", "", "", false );
        REQUIRE( customActionOtherBinding.isValid() );
        REQUIRE( customActionOtherBinding == customAction );
    }
}
