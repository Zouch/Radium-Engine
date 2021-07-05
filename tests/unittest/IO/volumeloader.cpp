#include <Core/Asset/FileData.hpp>
#include <Core/Asset/VolumeData.hpp>
#include <Core/Geometry/Volume.hpp>
#include <IO/VolumesLoader/VolumeLoader.hpp>
#include <catch2/catch.hpp>

TEST_CASE( "IO/VolumesLoader", "[IO]" ) {
    using namespace Ra::IO;
    using namespace Ra::Core::Asset;
    using namespace Ra::Core::Geometry;

    VolumeLoader loader;

    SECTION( "Basic FileLoaderInterface" ) {
        auto loader_name = loader.name();
        REQUIRE( loader_name.compare( "VolumeLoader (pbrt experimental, pvm)" ) == 0 );

        auto supportedFiles = loader.getFileExtensions();

        auto support_vol = std::find(
            std::begin( supportedFiles ), std::end( supportedFiles ), std::string {"*.vol"} );
        REQUIRE( support_vol != std::end( supportedFiles ) );

        auto support_pvm = std::find(
            std::begin( supportedFiles ), std::end( supportedFiles ), std::string {"*.pvm"} );
        REQUIRE( support_pvm != std::end( supportedFiles ) );

        auto support_openvdb = std::find(
            std::begin( supportedFiles ), std::end( supportedFiles ), std::string {"*.vdb"} );
        REQUIRE( support_openvdb == std::end( supportedFiles ) );

        auto handle_vol = loader.handleFileExtension( "pvm" );
        REQUIRE( handle_vol );

        auto handle_pvm = loader.handleFileExtension( "pvm" );
        REQUIRE( handle_pvm );

        auto handle_openvdb = loader.handleFileExtension( "vdb" );
        REQUIRE( !handle_openvdb );
    }

    SECTION( "Loading PVM data file" ) {
        std::unique_ptr<FileData> loadedFile( loader.loadFile( "data/Lobster.pvm" ) );
        REQUIRE( loadedFile != nullptr );
        auto volumeFiledata = loadedFile->getVolumeData();
        REQUIRE( volumeFiledata.size() == 1 );
        auto abstractvolume = volumeFiledata[0]->volume;
        REQUIRE( abstractvolume != nullptr );
        REQUIRE( abstractvolume->getType() == AbstractVolume::VolumeStorageType::DISCRETE_DENSE );

        auto volumeData = dynamic_cast<VolumeGrid*>( abstractvolume );
        REQUIRE( volumeData != nullptr );

        auto volumeSize = volumeData->size();
        REQUIRE(
            ( ( volumeSize[0] == 301 ) && ( volumeSize[1] == 324 ) && ( volumeSize[2] == 56 ) ) );

        auto binsize = volumeData->binSize();
        REQUIRE(
            ( ( binsize[0] == 1._ra ) && ( binsize[1] == 1._ra ) && ( binsize[2] == 1.4_ra ) ) );
    }
}
