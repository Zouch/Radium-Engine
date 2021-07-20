#pragma once

#include <Engine/RaEngine.hpp>

#include <memory>
#include <string>

#include <Engine/OpenGL.hpp>

#include <Core/Utils/Color.hpp>

namespace globjects {
class Texture;
}

namespace Ra {
namespace Engine {
namespace Data {
/**
 * Describes the content and parameters of a texture.
 * This structures encapsulates all the states used for creating an OpenGL texture.
 *  These parameters describe the image data of the texture :
 *    - target, width, height, depth, format, internalFormat, type and texels for describing image
 * data
 *    - wrapS, wrapT, wrapP, minFilter and magFilter for describing the sampler of the texture.
 *
 *  When one wants to create a texture, the first thing to do is to create and fill a Texture
 * parameter structure that will describe the Texture.
 *
 *  The Texture creation could be done either using the TextureManager or directly on the client
 * class/function.
 *
 *  When a texture is created, no OpenGL initialisation is realized. The user must first call
 * initializeGL before being able to use this texture in an OpenGL operation.
 *
 *  MipMap representation of the texture is automatically generated as soon as the minFilter
 * parameter is something else than GL_LINEAR or GL_NEAREST
 *
 * @note No coherence checking will be done on the content of this structure. User must ensure
 * coherent data and parameters before creating the OpenGL texture with Texture::initializeGL
 */
struct TextureParameters {
    /// Name of the texture
    std::string name {};
    /// OpenGL target
    GLenum target { GL_TEXTURE_2D };
    /// width of the texture (s dimension)
    size_t width { 1 };
    /// height of the texture (t dimension)
    size_t height { 1 };
    /// width of the texture (p dimension)
    size_t depth { 1 };
    /// Format of the external data
    GLenum format { GL_RGB };
    /// OpenGL internal format (WARNING, for Integer textures, must be GL_XXX_INTEGER)
    GLenum internalFormat { GL_RGB };
    /// Type of the components in external data
    GLenum type { GL_UNSIGNED_BYTE };
    /// OpenGL wrap mode in the s direction
    GLenum wrapS { GL_CLAMP_TO_EDGE };
    /// OpenGL wrap mode in the t direction
    GLenum wrapT { GL_CLAMP_TO_EDGE };
    /// OpenGL wrap mode in the p direction
    GLenum wrapP { GL_CLAMP_TO_EDGE };
    /// OpenGL minification filter ( GL_LINEAR or GL_NEAREST or GL_XXX_MIPMAP_YYY )
    GLenum minFilter { GL_LINEAR };
    /// OpenGL magnification filter ( GL_LINEAR or GL_NEAREST )
    GLenum magFilter { GL_LINEAR };
    /// External data (ownership is left to caller, not stored after OpenGL texture creation).
    /// Note that, for cube-map texture, this is considered as a "void*[6]" array containing the 6
    /// faces of the cube corresponding to the targets. <br/>
    /// texels[0] <-- GL_TEXTURE_CUBE_MAP_POSITIVE_X <br/>
    /// texels[1] <-- GL_TEXTURE_CUBE_MAP_NEGATIVE_X <br/>
    /// texels[2] <-- GL_TEXTURE_CUBE_MAP_POSITIVE_Y <br/>
    /// texels[3] <-- GL_TEXTURE_CUBE_MAP_NEGATIVE_Y <br/>
    /// texels[4] <-- GL_TEXTURE_CUBE_MAP_POSITIVE_Z <br/>
    /// texels[5] <-- GL_TEXTURE_CUBE_MAP_NEGATIVE_Z <br/>
    /// @todo memory allocated for this pointer might be lost at texture deletion as ownership is
    /// unclear.
    void* texels { nullptr };
};

/** Represent a Texture of the engine
 * See TextureManager for information about how unique texture are defined.
 */
class RA_ENGINE_API Texture final
{
  public:
    /** Textures are not copyable, delete copy constructor.
     */
    Texture( const Texture& ) = delete;

    /** Textures are not copyable, delete operator =.
     */
    void operator=( const Texture& ) = delete;

    /**
     * Texture constructor. No OpenGL initialization is done there.
     *
     * @param texParameters Name of the texture
     */
    explicit Texture( const TextureParameters& texParameters );

    /**
     * Texture destructor. Both internal data and OpenGL stuff are deleted.
     */
    ~Texture();

    /** @brief Generate the OpenGL representation of the texture according to the stored TextureData
     *
     * This method use the available m_textureParameters to generate and configure OpenGL
     * texture.
     *
     * Before uploading texels to the GPU, this method will apply RGB space conversion if needed.
     *
     * @param linearize (default false) : convert the texture from sRGB to Linear RGB color space
     * before OpenGL initialisation
     * @note This will become soon the only way to generate an Radium Engine OpenGL texture.
     */
    void initializeGL( bool linearize = false );

    /**
     * @brief Bind the texture to enable its use in a shader
     * @param unit Index of the texture to be bound. If -1 only calls glBindTexture.
     */
    void bind( int unit = -1 );

    /**
     * Bind the texture to an image unit for the purpose of reading and writing it from shaders.
     * @note, only available since openGL 4.2, not available on MacOs
     * uses m_parameters.internalFormat as format.
     * see
     * https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindImageTexture.xhtml
     * for documentation
     */
    void bindImageTexture( int unit, GLint level, GLboolean layered, GLint layer, GLenum access );

    /**
     * @return Name of the texture.
     */
    inline std::string getName() const { return m_textureParameters.name; }

    /**
     * Update the data contained by the texture
     * @param newData The new data, must contain the same number of elements than old data, no
     * check will be performed.
     */
    void updateData( const void* newData );

    /**
     * Update the parameters contained by the texture.
     * User first modify the public attributes corresponding to the parameter he wants to change
     * the value (e.g wrap* or *Filter) and call this function to update the OpenGL texture
     * state ...
     * @return
     */
    void updateParameters();

    /**
     * Convert a color texture from sRGB to Linear RGB spaces.
     * This will transform the internal representation of the texture to GL_SCALAR (GL_FLOAT).
     * Only GL_RGB[8, 16, 16F, 32F] and GL_RGBA[8, 16, 16F, 32F] are managed.
     * Full transformation as described at https://en.wikipedia.org/wiki/SRGB
     */
    void linearize();

    /**
     * @return the pixel format of the texture
     */
    GLenum format() const { return m_textureParameters.format; }
    /**
     * @return the width of the texture
     */
    size_t width() const { return m_textureParameters.width; }
    /**
     * @return the height of the texture
     */
    size_t height() const { return m_textureParameters.height; }
    /**
     * @return the depth of the texture
     */
    size_t depth() const { return m_textureParameters.depth; }

    void* texels() { return m_textureParameters.texels; }
    /**
     * @return the globjects::Texture associated with the texture
     */
    globjects::Texture* texture() const { return m_texture.get(); }

    /** Resize the texture.
     * This allocate GPU memory to store the new resized texture and, if texels are not nullptr,
     * upload the new content.
     * @note : If texels are not nullptr, user must ensure the texels array is correctly
     * dimensioned.
     * @param w width of the texture
     * @param h height of the texture
     * @param d depth of the texture
     * @param pix the new texels array corresponding the the new texture dimension
     */
    void resize( size_t w = 1, size_t h = 1, size_t d = 1, void* pix = nullptr );

    /// get read access to texture parameters
    const TextureParameters& getParameters() const { return m_textureParameters; }

    /** get read/write access to texture parameters, need to update
     * representation afterward, @see setParameters()
     */
    TextureParameters& getParameters() { return m_textureParameters; }

    /** set TextureParameters.
     * If texels is changed, need to call initializeGL() to update GPU representation
     * if only wrap or filter parameters are change, updateParameters() is
     * sufficient to update the GPU representation.
     */
    void setParameters( const TextureParameters& textureParameters ) {
        m_textureParameters = textureParameters;
    }

  private:
    /**
     * Convert a color texture from sRGB to Linear RGB spaces.
     * The content of the array of texels.
     * designated by the texel pointer is modified by side effect.
     * Full transformation as described at https://en.wikipedia.org/wiki/SRGB
     * @param texels the array of texels to linearize
     * @param numComponent number of color channels.
     * @param bool hasAlphaChannel indicate if the last channel is an alpha channel.
     * @param gamma the gama value to use (sRGB is 2.4)
     * @note only 8 bit (GL_UNSIGNED_BYTE data format) textures are managed by this operator.
     */
    void sRGBToLinearRGB( uint8_t* texels, uint numComponent, bool hasAlphaChannel );

    /// linearize a cube map by calling sRGBToLinearRGB fore each face
    void linearizeCubeMap( uint numComponent, bool hasAlphaChannel );

    /** Texture parameters
     */
    TextureParameters m_textureParameters;

    /// Link to glObject texture
    std::unique_ptr<globjects::Texture> m_texture;
    /// Is the texture mip-mapped ?
    bool m_isMipMapped { false };
    /// Is the texture in LinearRGB ?
    bool m_isLinear { false };
};
} // namespace Data
} // namespace Engine
} // namespace Ra
