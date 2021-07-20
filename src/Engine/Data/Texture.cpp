#include <Core/Utils/Log.hpp>
#include <Engine/Data/Texture.hpp>

#include <globjects/Texture.h>

#include <cmath>

namespace Ra {
namespace Engine {
namespace Data {
using namespace Core::Utils; // log

Texture::Texture( const TextureParameters& texParameters ) :
    m_textureParameters { texParameters },
    m_texture { nullptr },
    m_isMipMapped { false },
    m_isLinear { false } {}

Texture::~Texture() = default;

void Texture::initializeGL( bool linearize ) {
    if ( ( m_textureParameters.target != GL_TEXTURE_1D ) &&
         ( m_textureParameters.target != GL_TEXTURE_2D ) &&
         ( m_textureParameters.target != GL_TEXTURE_RECTANGLE ) &&
         ( m_textureParameters.target != GL_TEXTURE_3D ) &&
         ( m_textureParameters.target != GL_TEXTURE_CUBE_MAP ) )
    {
        LOG( logERROR ) << "Texture of type " << m_textureParameters.target
                        << " must be generated explicitly!";
        return;
    }
    // Transform texels if needed
    if ( linearize )
    {
        uint numComp  = 0;
        bool hasAlpha = false;
        switch ( m_textureParameters.format )
        {
            // RED and RG texture store a gray scale color. Verify if we need to convert
        case GL_RED:
            numComp = 1;
            break;
        case GL_RG:
            // corresponds to deprecated GL_LUMINANCE_ALPHA
            numComp  = 2;
            hasAlpha = true;
            break;
        case GL_RGB:
            numComp = 3;
            break;
        case GL_RGBA:
            numComp  = 4;
            hasAlpha = true;
            break;
        default:
            LOG( logERROR ) << "Textures with format " << m_textureParameters.format
                            << " can't be linearized." << m_textureParameters.name;
            return;
        }
        if ( m_textureParameters.target == GL_TEXTURE_CUBE_MAP )
        { linearizeCubeMap( numComp, hasAlpha ); }
        else
        {
            // This will only do do the RGB space conversion
            sRGBToLinearRGB(
                reinterpret_cast<uint8_t*>( m_textureParameters.texels ), numComp, hasAlpha );
        }
    }
    // Generate OpenGL texture
    if ( m_texture == nullptr )
    {
        m_texture = globjects::Texture::create( m_textureParameters.target );
        GL_CHECK_ERROR;
    }
    // Update the sampler parameters
    m_isMipMapped = !( m_textureParameters.minFilter == GL_NEAREST ||
                       m_textureParameters.minFilter == GL_LINEAR );
    updateParameters();
    // upload texture to the GPU
    updateData( m_textureParameters.texels );
    // Generate mip-map if needed.
    if ( m_isMipMapped ) { m_texture->generateMipmap(); }
}

void Texture::bind( int unit ) {
    if ( unit >= 0 ) { m_texture->bindActive( uint( unit ) ); }
    else
    { m_texture->bind(); }
}

void Texture::bindImageTexture( int unit,
                                GLint level,
                                GLboolean layered,
                                GLint layer,
                                GLenum access ) {
    m_texture->bindImageTexture(
        uint( unit ), level, layered, layer, access, m_textureParameters.internalFormat );
}

void Texture::updateData( const void* data ) {
    switch ( m_texture->target() )
    {
    case GL_TEXTURE_1D: {
        m_texture->image1D( 0,
                            m_textureParameters.internalFormat,
                            GLsizei( m_textureParameters.width ),
                            0,
                            m_textureParameters.format,
                            m_textureParameters.type,
                            data );
        GL_CHECK_ERROR
    }
    break;
    case GL_TEXTURE_2D:
    case GL_TEXTURE_RECTANGLE: {
        m_texture->image2D( 0,
                            m_textureParameters.internalFormat,
                            GLsizei( m_textureParameters.width ),
                            GLsizei( m_textureParameters.height ),
                            0,
                            m_textureParameters.format,
                            m_textureParameters.type,
                            data );
        GL_CHECK_ERROR
    }
    break;
    case GL_TEXTURE_3D: {
        m_texture->image3D( 0,
                            m_textureParameters.internalFormat,
                            GLsizei( m_textureParameters.width ),
                            GLsizei( m_textureParameters.height ),
                            GLsizei( m_textureParameters.depth ),
                            0,
                            m_textureParameters.format,
                            m_textureParameters.type,
                            data );
        GL_CHECK_ERROR
    }
    break;
    case GL_TEXTURE_CUBE_MAP: {
        // Load the 6 faces of the cube-map
        void** texels = (void**)data;
        m_texture->bind();
        // track globjects update that will hopefully support direct loading of
        // cube-maps
        gl::glTexImage2D( gl::GL_TEXTURE_CUBE_MAP_POSITIVE_X,
                          0,
                          m_textureParameters.internalFormat,
                          GLsizei( m_textureParameters.width ),
                          GLsizei( m_textureParameters.height ),
                          0,
                          m_textureParameters.format,
                          m_textureParameters.type,
                          texels[0] );
        gl::glTexImage2D( gl::GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
                          0,
                          m_textureParameters.internalFormat,
                          GLsizei( m_textureParameters.width ),
                          GLsizei( m_textureParameters.height ),
                          0,
                          m_textureParameters.format,
                          m_textureParameters.type,
                          texels[1] );

        gl::glTexImage2D( gl::GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
                          0,
                          m_textureParameters.internalFormat,

                          GLsizei( m_textureParameters.width ),
                          GLsizei( m_textureParameters.height ),
                          0,
                          m_textureParameters.format,
                          m_textureParameters.type,
                          texels[2] );
        gl::glTexImage2D( gl::GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
                          0,
                          m_textureParameters.internalFormat,
                          GLsizei( m_textureParameters.width ),
                          GLsizei( m_textureParameters.height ),
                          0,
                          m_textureParameters.format,
                          m_textureParameters.type,
                          texels[3] );

        gl::glTexImage2D( gl::GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
                          0,
                          m_textureParameters.internalFormat,
                          GLsizei( m_textureParameters.width ),
                          GLsizei( m_textureParameters.height ),
                          0,
                          m_textureParameters.format,
                          m_textureParameters.type,
                          texels[4] );
        gl::glTexImage2D( gl::GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
                          0,
                          m_textureParameters.internalFormat,
                          GLsizei( m_textureParameters.width ),
                          GLsizei( m_textureParameters.height ),
                          0,
                          m_textureParameters.format,
                          m_textureParameters.type,
                          texels[5] );

        m_texture->unbind();
        GL_CHECK_ERROR
    }
    break;
    default: {
        CORE_ASSERT( 0, "Unsupported texture type ?" );
    }
    break;
    }
    GL_CHECK_ERROR;
}

// let the compiler warn about case fallthrough
void Texture::updateParameters() {
    switch ( m_texture->target() )
    {
    case GL_TEXTURE_CUBE_MAP:
    case GL_TEXTURE_3D:
        m_texture->setParameter( GL_TEXTURE_WRAP_R, m_textureParameters.wrapP );
        GL_CHECK_ERROR;
        [[fallthrough]];
    case GL_TEXTURE_2D:
    case GL_TEXTURE_RECTANGLE:
        m_texture->setParameter( GL_TEXTURE_WRAP_T, m_textureParameters.wrapT );
        GL_CHECK_ERROR;
        [[fallthrough]];
    case GL_TEXTURE_1D:
        m_texture->setParameter( GL_TEXTURE_WRAP_S, m_textureParameters.wrapS );
        GL_CHECK_ERROR;
        break;
    default:
        break;
    }
    m_texture->setParameter( GL_TEXTURE_MIN_FILTER, m_textureParameters.minFilter );
    GL_CHECK_ERROR;
    m_texture->setParameter( GL_TEXTURE_MAG_FILTER, m_textureParameters.magFilter );
    GL_CHECK_ERROR;
}

void Texture::linearize() {
    if ( m_texture != nullptr )
    {
        LOG( logERROR ) << "Only non OpenGL initialized texture can be linearized.";
        return;
    }
    // Only RGB and RGBA texture contains color information
    // (others are not really colors and must be managed explicitly by the user)
    uint numComp  = 0;
    bool hasAlpha = false;
    switch ( m_textureParameters.format )
    {
        // RED texture store a gray scale color. Verify if we need to convert
    case GL_RED:
        numComp = 1;
        break;
    case GL_RGB:
        numComp = 3;
        break;
    case GL_RGBA:
        numComp  = 4;
        hasAlpha = true;
        break;
    default:
        LOG( logERROR ) << "Textures with format " << m_textureParameters.format
                        << " can't be linearized." << m_textureParameters.name;
        return;
    }
    sRGBToLinearRGB( reinterpret_cast<uint8_t*>( m_textureParameters.texels ), numComp, hasAlpha );
}

void Texture::sRGBToLinearRGB( uint8_t* texels, uint numComponent, bool hasAlphaChannel ) {
    if ( !m_isLinear )
    {
        m_isLinear = true;
        // auto linearize = [gamma](float in)-> float {
        auto linearize = []( uint8_t in ) -> uint8_t {
            // Constants are described at https://en.wikipedia.org/wiki/SRGB
            float c = float( in ) / 255;
            if ( c < 0.04045 ) { c = c / 12.92f; }
            else
            { c = std::pow( ( ( c + 0.055f ) / ( 1.055f ) ), 2.4f ); }
            return uint8_t( c * 255 );
        };
        uint numValues = hasAlphaChannel ? numComponent - 1 : numComponent;
#pragma omp parallel for
        for ( int i = 0; i < int( m_textureParameters.width * m_textureParameters.height *
                                  m_textureParameters.depth );
              ++i )
        {
            // Convert each R or RGB value while keeping alpha unchanged
            for ( uint p = i * numComponent; p < i * numComponent + numValues; ++p )
            { texels[p] = linearize( texels[p] ); }
        }
    }
}

void Texture::resize( size_t w, size_t h, size_t d, void* pix ) {
    m_textureParameters.width  = w;
    m_textureParameters.height = h;
    m_textureParameters.depth  = d;
    m_textureParameters.texels = pix;
    if ( m_texture == nullptr ) { initializeGL( false ); }
    else
    { updateData( m_textureParameters.texels ); }
    if ( m_isMipMapped ) { m_texture->generateMipmap(); }
}

void Texture::linearizeCubeMap( uint numComponent, bool hasAlphaChannel ) {
    if ( m_textureParameters.type == gl::GLenum::GL_UNSIGNED_BYTE )
    {
        /// Only unsigned byte texture could be linearized. Considering other formats where already
        /// linear
        for ( int i = 0; i < 6; ++i )
        {
            sRGBToLinearRGB(
                reinterpret_cast<uint8_t*>( ( (void**)m_textureParameters.texels )[i] ),
                numComponent,
                hasAlphaChannel );
        }
    }
}

} // namespace Data
} // namespace Engine
} // namespace Ra
