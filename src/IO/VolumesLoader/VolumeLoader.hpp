#pragma once

#include <Core/Asset/FileLoaderInterface.hpp>
#include <IO/RaIO.hpp>

namespace Ra {
namespace IO {
/**
 * \brief Loads density grid for volume data.
 * This loader support 2 file formats for density grid data
 *   - PVM file format from The Volume Library at http://schorsch.efi.fh-nuernberg.de/data/volume/
 *   - Custom file format (derived from pbrt heterogeneous media definition) with the following
 *   properties
 *      sigma_a [ RGB values of the absorption coefficient ] units are mm-1
 *      sigma_s [ RGB values of the scattering coefficient ] units are mm-1
 *      size [ w h d : 3 ints that gives the size of the grid ]
 *      density [ w*h*d white space separated floating point values defining the density ]
 */
class VolumeLoader : public Ra::Core::Asset::FileLoaderInterface
{
  public:
    VolumeLoader();

    ~VolumeLoader() override;

    std::vector<std::string> getFileExtensions() const override;
    bool handleFileExtension( const std::string& extension ) const override;
    Ra::Core::Asset::FileData* loadFile( const std::string& filename ) override;
    std::string name() const override;

  private:
    /**
     * Load custom vol file format
     * @param filename the file to load
     * @return resulting file data, nullptr if the file was not loaded
     */
    Ra::Core::Asset::FileData* loadVolFile( const std::string& filename );

    /**
     * Load pvm file format
     * @param filename the file to load
     * @return resulting file data, nullptr if the file was not loaded
     */
    Ra::Core::Asset::FileData* loadPvmFile( const std::string& filename );
};

} // namespace IO
} // namespace Ra
