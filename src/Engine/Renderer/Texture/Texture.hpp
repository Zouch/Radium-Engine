#ifndef RADIUMENGINE_TEXTURE_HPP
#define RADIUMENGINE_TEXTURE_HPP

#include <Engine/RaEngine.hpp>

#include <memory>
#include <string>

#include <Engine/Renderer/OpenGL/OpenGL.hpp>

namespace globjects {
class Texture;
}

namespace Ra {
namespace Engine {

/**
 * Describes the content and parameters of a texture.
 */
struct TextureData {
    /// Name of the texture
    std::string name{};
    /// OpenGL target
    GLenum target{GL_TEXTURE_2D};
    /// width of the texture (s dimension)
    size_t width{1};
    /// height of the texture (t dimension)
    size_t height{1};
    /// width of the texture (q dimension)
    size_t depth{1};
    /// Format of the external data
    GLenum format{GL_RGB};
    /// OpenGL internal format (WARNING, for Integer textures, must be GL_XXX_INTEGER)
    GLenum internalFormat{GL_RGB};
    /// Type of the components in external data
    GLenum type{GL_UNSIGNED_BYTE};
    /// OpenGL wrap mode in the s direction
    GLenum wrapS{GL_CLAMP_TO_EDGE};
    /// OpenGL wrap mode in the t direction
    GLenum wrapT{GL_CLAMP_TO_EDGE};
    /// OpenGL wrap mode in the q direction
    GLenum wrapR{GL_CLAMP_TO_EDGE};
    /// OpenGL minification filter ( GL_LINEAR or GL_NEAREST or GL_XXX_MIPMAP_YYY )
    GLenum minFilter{GL_LINEAR};
    /// OpenGL magnification filter ( GL_LINEAR or GL_NEAREST )
    GLenum magFilter{GL_LINEAR};
    /// External data (not stored after OpenGL texture creation)
    void* texels{nullptr};
};

/** Represent a Texture of the engine
 * See TextureManager to informations about how unique texture are defined.
 */
class RA_ENGINE_API Texture final {
  public:
    /** Texture parameters
     */
    TextureData m_textureParameters;

    /** Textures are not copyable, delete copy constructor.
     */
    Texture( const Texture& ) = delete;

    /** Textures are not copyable, delete operator =.
     */
    void operator=( const Texture& ) = delete;

    /**
     * Texture constructor. No OpenGL initialization is done there.
     *
     * @param name The filename containing the texture
     *
     */
    explicit Texture( const std::string& name = "" );

    /**
     * Texture constructor. No OpenGL initialization is done there.
     *
     * @param texParameters Name of the texture
     */
    explicit Texture( const TextureData& texParameters );

    /**
     * Texture desctructor. Both internal data and OpenGL stuff are deleted.
     */
    ~Texture();

    /** @brief Generate the OpenGL representation of the texture according to the stored TextureData
     *
     * This method use the available m_textureParameters to generate and and configure OpenGL
     * texture. before calling this method, user must fill a TextureData structure, set the
     * m_textureParameters to this TextureData structure and call Generate with the appropriate
     * linearize and mipmap parameters.
     *
     * @param linearize (default false) : convert the texture from sRGB to Linear RGB color space
     * @param mipmaped (default false) : generate a prefiltered mipmap for the texture.
     * @note This will become soon the only way to generate an Radium Engine OpenGL texture.
     */
    void InitializeGL( bool linearize = false, bool mipmaped = false );

    /**
     * @brief Init the textures needed for the cubemap from OpenGL point of view.
     *
     * Generate, bind and configure OpenGL texture.<br/>
     * Also sets wrapping to GL_REPEAT and min / mag filters to GL_LINEAR, although no mipmaps are
     * generated.<br/><br/>
     *
     * It is highly recommended to take a look at
     * <a href="https://www.opengl.org/wiki/GLAPI/glTexImage2D">glTexImage2D documentation</a>
     * since this method doc will highly refer to it.
     *
     * @param internalFormat The number of color components of the texture, and their size.
     * Refer to the link given above, at the \b internalFormat section
     * for further informations about available internal formats.
     *
     * @param width Width of the six 2D textures.
     *
     * @param height Height of the six 2D textures.
     *
     * @param format The format of the pixel data.
     * Refer to the link given above, at the \b format section
     * for further informations about the available formats.
     *
     * @param type The data type of the pixel data.
     * Refer to the link given above, at the \b type section
     * for further informations about the available types.
     *
     * @param data Data contained in the texture. Can be nullptr. <br/>
     * If \b data is not null, the texture will take the ownership of it.
     *
     * @param linearize (default false) : convert the texture from sRGB to Linear RGB color space
     *
     * @param mipmaped (default false) : generate a prefiltered mipmap for the texture.
     *
     * @todo integrate this method in the same workflow than other textures ...
     */
     void GenerateCube( uint width, uint height, GLenum format, void** data = nullptr,
                       bool linearize = false, bool mipmaped = false );

    /**
     * @brief Bind the texture to enable its use in a shader
     * @param unit Index of the texture to be bound. If -1 only calls glBindTexture.
     */
    void bind( int unit = -1 );

    /**
     * @return Name of the texture.
     */
    inline std::string getName() const { return m_textureParameters.name; }

    /**
     * Update the data contained by the texture
     * @param newData The new data, must contain the same number of elements than old data, no check
     * will be performed.
     */
    void updateData( void* newData );

    /**
     * Update the parameters contained by the texture.
     * User first modify the public attributes corresponding to the parameter he wants to change the
     * value (e.g wrap* or *Filter) and call this function to update the OpenGL texture state ...
     * @return
     */
    void updateParameters();

    /**
     * Convert a color texture from sRGB to Linear RGB spaces.
     * This will transform the internal representation of the texture to GL_FLOAT.
     * Only GL_RGB[8, 16, 16F, 32F] and GL_RGBA[8, 16, 16F, 32F] are managed.
     * Full transformation as described at https://en.wikipedia.org/wiki/SRGB
     * @param gamma the gama value to use (sRGB is 2.4)
     */
    void linearize( Scalar gamma = Scalar( 2.4 ) );

    /**
     * @return the pixel format of the texture
     */
    GLenum format() const { return m_textureParameters.format; }
    /**
     * @return the width of the texture
     */
    uint width() const { return m_textureParameters.width; }
    /**
     * @return the height of the texture
     */
    uint height() const { return m_textureParameters.height; }
    /**
     * @return the depth of the texture
     */
    uint depth() const { return m_textureParameters.depth; }
    /**
     * @return the globjects::Texture associated with the texture
     */
    globjects::Texture* texture() const { return m_texture.get(); }

    /** Resize the texture.
     * WARNING : must be used only on textures attached to framebuffer)
     * @param w width of the texture
     * @param h height of the texture
     * @param d depth of the texture
     */
    void resize(size_t w = 1, size_t h = 1, size_t d = 1);

  private:
    /**
     * Convert a color texture from sRGB to Linear RGB spaces.
     * This will transform the internal representation of the texture to GL_FLOAT.
     * Only GL_RGB[8, 16, 16F, 32F] and GL_RGBA[8, 16, 16F, 32F] are managed.
     * Full transformation as described at https://en.wikipedia.org/wiki/SRGB
     * @param texels the array of texels to linearize
     * @param numCommponent number of color channels.
     * @param bool hasAlphaChannel indicate if the last channel is an alpha channel.
     * @param gamma the gama value to use (sRGB is 2.4)
     * @note only 8 bit textures are managed by this operator.
     */
    void sRGBToLinearRGB( uint8_t* texels, int numCommponent, bool hasAlphaChannel,
                          Scalar gamma = Scalar( 2.4 ) );

    /// Link to glObject texture
    std::unique_ptr<globjects::Texture> m_texture;
    /// Is the texture mipmaped ?
    bool m_isMipMaped{false};
    /// Is the texture in LinearRGB ?
    bool m_isLinear{false};
};
} // namespace Engine
} // namespace Ra

#endif // RADIUMENGINE_TEXTURE_HPP
