
#include <ExampleLibrary.hpp>
#include <PrivateHeader.hpp>

#include <Core/Resources/Resources.hpp>
#include <Core/Utils/Log.hpp>

#include <fstream>
#include <string>

namespace ExampleLibrary {

// This symbol will be used to identify the library and find its directory using
// Ra::Core::Resources::ResourcesLocator.
static int ExampleLibraryMagick = 0x0F0F0F0F;

void ExampleMessage( const std::string& message ) {
    auto resourcesPath = Ra::Core::Resources::ResourcesLocator(
        reinterpret_cast<void*>( &ExampleLibraryMagick ), "Resources/LibraryResources", "../" );
    std::string welcomeMsg = resourcesPath.getBasePath() + "/welcome.txt";
    std::ifstream inputMsg( welcomeMsg );
    if ( inputMsg.is_open() ) {
        std::stringstream buffer;
        buffer << inputMsg.rdbuf();
        LOG( Ra::Core::Utils::logINFO )
            << ExamplePrefix << " -- " << message << buffer.str() << " at " << welcomeMsg;
    }
    else {
        LOG( Ra::Core::Utils::logINFO ) << ExamplePrefix << " -- " << message << " -- "
                                        << "Unable to open resource file " << welcomeMsg;
    }
}
} // namespace ExampleLibrary
