#ifndef RADIUMENGINE_TINYPLYFILELOADER_HPP
#define RADIUMENGINE_TINYPLYFILELOADER_HPP

#include <Core/File/FileLoaderInterface.hpp>
#include <IO/RaIO.hpp>

namespace Ra {

namespace Asset {
class FileData;
} // namespace Asset

namespace IO {

/// The TinyPlyFileLoader loads Point Clouds from a PLY-format file.
class RA_IO_API TinyPlyFileLoader : public Asset::FileLoaderInterface {
  public:
    TinyPlyFileLoader();

    virtual ~TinyPlyFileLoader();

    std::vector<std::string> getFileExtensions() const override;

    bool handleFileExtension( const std::string& extension ) const override;

    Asset::FileData* loadFile( const std::string& filename ) override;

    std::string name() const override;
};

} // namespace IO
} // namespace Ra

#endif // RADIUMENGINE_ASSIMPFILELOADER_HPP
