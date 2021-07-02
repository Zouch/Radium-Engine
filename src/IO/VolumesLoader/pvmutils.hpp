#pragma once

namespace Ra {
namespace IO {
namespace PVMVolume {
// source file extracted from http://www.stereofx.org/download/ V^3 Volume Renderer

/**
 * \brief Loads a volume in pvm format. See The Volume Library at
 * http://schorsch.efi.fh-nuernberg.de/data/volume/
 * @param filename name of the file to load
 * @param width     width of the loaded data
 * @param height    height of the loaded data
 * @param depth     depth  of the loaded data
 * @param bytes     number of byte per density value
 * @param scalex    x scale of the voxel
 * @param scaley    y scale of the voxel
 * @param scalez    z scale of the voxel
 * @param desc      Text describing the data
 * @param courtesy  Text with copyright info
 * @param parameter Text with aquisition parameters description
 * @param comment   Text comenting the data
 * @return  a pointer to the raw density data, nullptr if the file was not loaded.
 */
unsigned char* readPVMvolume( const char* filename,
                              unsigned int* width,
                              unsigned int* height,
                              unsigned int* depth,
                              unsigned int* bytes       = nullptr,
                              float* scalex             = nullptr,
                              float* scaley             = nullptr,
                              float* scalez             = nullptr,
                              unsigned char** desc      = nullptr,
                              unsigned char** courtesy  = nullptr,
                              unsigned char** parameter = nullptr,
                              unsigned char** comment   = nullptr );

} // namespace PVMVolume

} // namespace IO
} // namespace Ra
