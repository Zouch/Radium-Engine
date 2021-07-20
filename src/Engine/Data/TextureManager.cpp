#include <Engine/Data/Texture.hpp>
#include <Engine/Data/TextureManager.hpp>

#include <Core/Utils/Log.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace Ra {
namespace Engine {
namespace Data {

using namespace Core::Utils; // log

TextureManager::TextureManager() = default;

TextureManager::~TextureManager() {
    for ( auto& tex : m_textures ) {
        delete tex.second;
    }
    m_textures.clear();
    m_pendingTextures.clear();
    m_pendingData.clear();
}

TextureParameters&
TextureManager::addTexture( const std::string& name, uint width, uint height, void* data ) {
    TextureParameters texData;
    texData.name   = name;
    texData.width  = width;
    texData.height = height;
    texData.texels = data;

    m_pendingTextures[name] = texData;

    return m_pendingTextures[name];
}

void TextureManager::loadTextureImage( TextureParameters& texParameters ) {
    stbi_set_flip_vertically_on_load( true );
    int n;
    unsigned char* data = stbi_load( texParameters.name.c_str(),
                                     (int*)( &( texParameters.width ) ),
                                     (int*)( &( texParameters.height ) ),
                                     &n,
                                     0 );

    if ( !data ) {
        LOG( logERROR ) << "Something went wrong when loading image \"" << texParameters.name
                        << "\".";
        texParameters.width = texParameters.height = 0;
        return;
    }

    switch ( n ) {
    case 1: {
        texParameters.format         = GL_RED;
        texParameters.internalFormat = GL_R8;
    } break;

    case 2: {
        // suppose it is GL_LUMINANCE_ALPHA
        texParameters.format         = GL_RG;
        texParameters.internalFormat = GL_RG8;
    } break;

    case 3: {
        texParameters.format         = GL_RGB;
        texParameters.internalFormat = GL_RGB8;
    } break;

    case 4: {
        texParameters.format         = GL_RGBA;
        texParameters.internalFormat = GL_RGBA8;
    } break;
    default: {
        texParameters.format         = GL_RGBA;
        texParameters.internalFormat = GL_RGBA8;
    } break;
    }

    CORE_ASSERT( data, "Data is null" );
    texParameters.texels = data;
    texParameters.type   = GL_UNSIGNED_BYTE;
}

Texture* TextureManager::loadTexture( const TextureParameters& texParameters, bool linearize ) {
    TextureParameters texParams = texParameters;
    // TODO : allow to keep texels in texture parameters with automatic lifetime management.
    bool mustFreeTexels = false;
    if ( texParams.texels == nullptr ) {
        loadTextureImage( texParams );
        mustFreeTexels = true;
    }
    auto ret = new Texture( texParams );
    ret->initializeGL( linearize );

    if ( mustFreeTexels ) {
        stbi_image_free( ret->getParameters().texels );
        ret->getParameters().texels = nullptr;
    }
    return ret;
}

Texture* TextureManager::getOrLoadTexture( const TextureParameters& texParameters,
                                           bool linearize ) {
    {
        // Is texture in the manager ?
        auto it = m_textures.find( texParameters.name );
        if ( it != m_textures.end() ) { return it->second; }
    }
    {
        // Is texture pending but registered in the manager
        auto it = m_pendingTextures.find( texParameters.name );
        if ( it != m_pendingTextures.end() ) {
            auto pendingParams             = it->second;
            auto ret                       = loadTexture( pendingParams, linearize );
            m_textures[pendingParams.name] = ret;
            m_pendingTextures.erase( it );
            return ret;
        }
    }
    // Texture is not in the manager, add it
    auto ret = loadTexture( texParameters, linearize );

    m_textures[texParameters.name] = ret;
    return ret;
}

void TextureManager::deleteTexture( const std::string& filename ) {
    auto it = m_textures.find( filename );

    if ( it != m_textures.end() ) {
        delete it->second;
        m_textures.erase( it );
    }
}

void TextureManager::deleteTexture( Texture* texture ) {
    deleteTexture( texture->getName() );
}

void TextureManager::updateTextureContent( const std::string& texture, void* content ) {
    CORE_ASSERT( m_textures.find( texture ) != m_textures.end(),
                 "Trying to update non existing texture" );
    m_pendingData[texture] = content;
}

void TextureManager::updatePendingTextures() {
    if ( m_pendingData.empty() ) { return; }

    for ( auto& data : m_pendingData ) {
        LOG( logINFO ) << "TextureManager::updateTextures \"" << data.first << "\".";
        m_textures[data.first]->updateData( data.second );
    }
    m_pendingData.clear();
}

} // namespace Data
} // namespace Engine
} // namespace Ra
