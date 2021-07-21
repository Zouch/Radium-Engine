#include <Core/Resources/Resources.hpp>
#include <Core/Utils/StdFilesystem.hpp>
#include <Gui/Utils/KeyMappingManager.hpp>

#include <QtGlobal>

using namespace Ra;
using namespace Ra::Gui;
using Idx = KeyMappingManager::KeyMappingAction;
using Ctx = KeyMappingManager::Context;

int main( int argc, char** argv ) {

    /// need a base dir in argv[1]
    if ( argc != 2 ) { return -1; }
    std::string base { argv[1] };

    QSettings settings( "RadiumIntegrationTests", "KeyMappingManager" );
    settings.clear();
    KeyMappingManager::createInstance();
    auto mgr = Gui::KeyMappingManager::getInstance();
    auto optionalPath { Core::Resources::getRadiumResourcesPath() };
    auto resourcesRootDir { optionalPath.value_or( "[[Default ressource path not found]]" ) };

    mgr->loadConfiguration( "../data/inputs/keymapping-valid.xml" );
    if ( !mgr->saveConfiguration( base + "/keymapping-valid.xml" ) ) return -1;
    return 0;
}
