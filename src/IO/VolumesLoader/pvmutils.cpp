#include <VolumesLoader/pvmutils.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Ra {
namespace IO {
namespace PVMVolume {

// source file extracted from http://www.stereofx.org/download/ V^3 Volume Renderer

enum VR_ERROR {
    VR_ERROR_NONFATAL = 0,
    VR_ERROR_FATAL    = 1,
    VR_ERROR_MEM      = 2,
    VR_ERROR_IO       = 3,
    VR_ERROR_CODE     = 4
};

#define ERRORMSG() vrerrormsg( __FILE__, __LINE__, VR_ERROR_FATAL )
#define MEMERROR() vrerrormsg( __FILE__, __LINE__, VR_ERROR_MEM )
#define IOERROR() vrerrormsg( __FILE__, __LINE__, VR_ERROR_IO )
#define CODEERROR() vrerrormsg( __FILE__, __LINE__, VR_ERROR_CODE )
#define WARNMSG( msg ) vrerrormsg( __FILE__, __LINE__, VR_ERROR_NONFATAL, msg )

inline void vrerrormsg( const char* file, int line, int fatal, const char* msg = nullptr ) {
    if ( fatal == VR_ERROR_NONFATAL )
        fprintf( stderr, "warning" );
    else if ( fatal == VR_ERROR_MEM )
        fprintf( stderr, "insufficient memory" );
    else if ( fatal == VR_ERROR_IO )
        fprintf( stderr, "io error" );
    else if ( fatal == VR_ERROR_CODE )
        fprintf( stderr, "unimplemented code" );
    else
        fprintf( stderr, "fatal error" );
    fprintf( stderr, " in <%s> at line %d!\n", file, line );
    if ( msg != nullptr ) fprintf( stderr, "description: %s\n", msg );

    if ( fatal != VR_ERROR_NONFATAL ) exit( 1 );
}

// Differential Data Stream defines and constants

#define BOOLINT char

#ifndef TRUE
#    define TRUE ( 1 )
#endif
#ifndef FALSE
#    define FALSE ( 0 )
#endif

#define DDS_MAXSTR ( 256 )
#define DDS_BLOCKSIZE ( 1 << 20 )
#define DDS_INTERLEAVE ( 1 << 24 )
#define DDS_RL ( 7 )
#define DDS_ISINTEL ( *( (unsigned char*)( &DDS_INTEL ) + 1 ) == 0 )

char DDS_ID[]  = "DDS v3d\n";
char DDS_ID2[] = "DDS v3e\n";
unsigned char* DDS_cache;
unsigned int DDS_cachepos, DDS_cachesize;

unsigned int DDS_buffer;
unsigned int DDS_bufsize;

unsigned short int DDS_INTEL = 1;

inline int DDS_decode( int bits ) {
    return bits >= 1 ? bits + 1 : bits;
}

// helper functions for DDS:
inline unsigned int DDS_shiftl( const unsigned int value, const unsigned int bits ) {
    return ( bits >= 32 ) ? 0 : value << bits;
}

inline unsigned int DDS_shiftr( const unsigned int value, const unsigned int bits ) {
    return ( bits >= 32 ) ? 0 : value >> bits;
}

inline void DDS_swapuint( unsigned int* x ) {
    unsigned int tmp = *x;

    *x = ( ( tmp & 0xff ) << 24 ) | ( ( tmp & 0xff00 ) << 8 ) | ( ( tmp & 0xff0000 ) >> 8 ) |
         ( ( tmp & 0xff000000 ) >> 24 );
}

void DDS_initbuffer() {
    DDS_buffer  = 0;
    DDS_bufsize = 0;
}

inline void DDS_clearbits() {
    DDS_cache     = nullptr;
    DDS_cachepos  = 0;
    DDS_cachesize = 0;
}

inline void DDS_loadbits( unsigned char* data, unsigned int size ) {
    DDS_cache     = data;
    DDS_cachesize = size;

    if ( ( DDS_cache = (unsigned char*)realloc( DDS_cache, DDS_cachesize + 4 ) ) == nullptr )
        MEMERROR();
    *( (unsigned int*)&DDS_cache[DDS_cachesize] ) = 0;

    DDS_cachesize = 4 * ( ( DDS_cachesize + 3 ) / 4 );
    if ( ( DDS_cache = (unsigned char*)realloc( DDS_cache, DDS_cachesize ) ) == nullptr )
        MEMERROR();
}

inline unsigned int DDS_readbits( unsigned int bits ) {
    unsigned int value;

    if ( bits < DDS_bufsize )
    {
        DDS_bufsize -= bits;
        value = DDS_shiftr( DDS_buffer, DDS_bufsize );
    }
    else
    {
        value = DDS_shiftl( DDS_buffer, bits - DDS_bufsize );

        if ( DDS_cachepos >= DDS_cachesize )
            DDS_buffer = 0;
        else
        {
            DDS_buffer = *( (unsigned int*)&DDS_cache[DDS_cachepos] );
            if ( DDS_ISINTEL ) DDS_swapuint( &DDS_buffer );
            DDS_cachepos += 4;
        }

        DDS_bufsize += 32 - bits;
        value |= DDS_shiftr( DDS_buffer, DDS_bufsize );
    }

    DDS_buffer &= DDS_shiftl( 1, DDS_bufsize ) - 1;

    return value;
}

// deinterleave a byte stream
void DDS_deinterleave( unsigned char* data,
                       unsigned int bytes,
                       unsigned int skip,
                       unsigned int block = 0,
                       BOOLINT restore    = FALSE ) {
    unsigned int i, j, k;

    unsigned char *data2, *ptr;

    if ( skip <= 1 ) return;

    if ( block == 0 )
    {
        if ( ( data2 = (unsigned char*)malloc( bytes ) ) == nullptr ) MEMERROR();

        if ( !restore )
            for ( ptr = data2, i = 0; i < skip; i++ )
                for ( j = i; j < bytes; j += skip )
                    *ptr++ = data[j];
        else
            for ( ptr = data, i = 0; i < skip; i++ )
                for ( j = i; j < bytes; j += skip )
                    data2[j] = *ptr++;

        memcpy( data, data2, bytes );
    }
    else
    {
        if ( ( data2 = (unsigned char*)malloc(
                   ( bytes < skip * block ) ? bytes : skip * block ) ) == nullptr )
            MEMERROR();

        if ( !restore )
        {
            for ( k = 0; k < bytes / skip / block; k++ )
            {
                for ( ptr = data2, i = 0; i < skip; i++ )
                    for ( j = i; j < skip * block; j += skip )
                        *ptr++ = data[k * skip * block + j];

                memcpy( data + k * skip * block, data2, skip * block );
            }

            for ( ptr = data2, i = 0; i < skip; i++ )
                for ( j = i; j < bytes - k * skip * block; j += skip )
                    *ptr++ = data[k * skip * block + j];

            memcpy( data + k * skip * block, data2, bytes - k * skip * block );
        }
        else
        {
            for ( k = 0; k < bytes / skip / block; k++ )
            {
                for ( ptr = data + k * skip * block, i = 0; i < skip; i++ )
                    for ( j = i; j < skip * block; j += skip )
                        data2[j] = *ptr++;

                memcpy( data + k * skip * block, data2, skip * block );
            }

            for ( ptr = data + k * skip * block, i = 0; i < skip; i++ )
                for ( j = i; j < bytes - k * skip * block; j += skip )
                    data2[j] = *ptr++;

            memcpy( data + k * skip * block, data2, bytes - k * skip * block );
        }
    }

    free( data2 );
}

// interleave a byte stream
void DDS_interleave( unsigned char* data,
                     unsigned int bytes,
                     unsigned int skip,
                     unsigned int block = 0 ) {
    DDS_deinterleave( data, bytes, skip, block, TRUE );
}

// decode a Differential Data Stream
void DDS_decode( unsigned char* chunk,
                 unsigned int size,
                 unsigned char** data,
                 unsigned int* bytes,
                 unsigned int block = 0 ) {
    unsigned int skip, strip;

    unsigned char *ptr1, *ptr2;

    unsigned int cnt, cnt1, cnt2;
    int bits, act;

    DDS_initbuffer();

    DDS_clearbits();
    DDS_loadbits( chunk, size );

    skip  = DDS_readbits( 2 ) + 1;
    strip = DDS_readbits( 16 ) + 1;

    ptr1 = ptr2 = nullptr;
    cnt = act = 0;

    while ( ( cnt1 = DDS_readbits( DDS_RL ) ) != 0 )
    {
        bits = DDS_decode( DDS_readbits( 3 ) );

        for ( cnt2 = 0; cnt2 < cnt1; cnt2++ )
        {
            if ( strip == 1 || cnt <= strip )
                act += DDS_readbits( bits ) - ( 1 << bits ) / 2;
            else
                act += *( ptr2 - strip ) - *( ptr2 - strip - 1 ) + DDS_readbits( bits ) -
                       ( 1 << bits ) / 2;

            while ( act < 0 )
                act += 256;
            while ( act > 255 )
                act -= 256;

            if ( ( cnt & ( DDS_BLOCKSIZE - 1 ) ) == 0 )
            {
                if ( ptr1 == nullptr )
                {
                    if ( ( ptr1 = (unsigned char*)malloc( DDS_BLOCKSIZE ) ) == nullptr ) MEMERROR();
                    ptr2 = ptr1;
                }
                else
                {
                    if ( ( ptr1 = (unsigned char*)realloc( ptr1, cnt + DDS_BLOCKSIZE ) ) ==
                         nullptr )
                        MEMERROR();
                    ptr2 = &ptr1[cnt];
                }
            }

            *ptr2++ = act;
            cnt++;
        }
    }

    if ( ptr1 != nullptr )
        if ( ( ptr1 = (unsigned char*)realloc( ptr1, cnt ) ) == nullptr ) MEMERROR();

    DDS_interleave( ptr1, cnt, skip, block );

    *data  = ptr1;
    *bytes = cnt;
}

// read from a RAW file
unsigned char* readRAWfiled( FILE* file, unsigned int* bytes ) {
    unsigned char* data;
    unsigned int cnt, blkcnt;

    data = nullptr;
    cnt  = 0;

    do
    {
        if ( data == nullptr )
        {
            if ( ( data = (unsigned char*)malloc( DDS_BLOCKSIZE ) ) == nullptr ) MEMERROR();
        }
        else if ( ( data = (unsigned char*)realloc( data, cnt + DDS_BLOCKSIZE ) ) == nullptr )
            MEMERROR();

        blkcnt = fread( &data[cnt], 1, DDS_BLOCKSIZE, file );
        cnt += blkcnt;
    } while ( blkcnt == DDS_BLOCKSIZE );

    if ( cnt == 0 )
    {
        free( data );
        return nullptr;
    }

    if ( ( data = (unsigned char*)realloc( data, cnt ) ) == nullptr ) MEMERROR();

    *bytes = cnt;

    return data;
}

// read a RAW file
unsigned char* readRAWfile( const char* filename, unsigned int* bytes ) {
    FILE* file;

    unsigned char* data;

    if ( ( file = fopen( filename, "rb" ) ) == nullptr ) return nullptr;

    data = readRAWfiled( file, bytes );

    fclose( file );

    return data;
}

// read a Differential Data Stream
unsigned char* readDDSfile( const char* filename, unsigned int* bytes ) {
    int version = 1;

    FILE* file;

    int cnt;

    unsigned char *chunk, *data;
    unsigned int size;

    if ( ( file = fopen( filename, "rb" ) ) == nullptr ) return nullptr;

    for ( cnt = 0; DDS_ID[cnt] != '\0'; cnt++ )
        if ( fgetc( file ) != DDS_ID[cnt] )
        {
            fclose( file );
            version = 0;
            break;
        }

    if ( version == 0 )
    {
        if ( ( file = fopen( filename, "rb" ) ) == nullptr ) return nullptr;

        for ( cnt = 0; DDS_ID2[cnt] != '\0'; cnt++ )
            if ( fgetc( file ) != DDS_ID2[cnt] )
            {
                fclose( file );
                return nullptr;
            }

        version = 2;
    }

    if ( ( chunk = readRAWfiled( file, &size ) ) == nullptr ) IOERROR();

    fclose( file );

    DDS_decode( chunk, size, &data, bytes, version == 1 ? 0 : DDS_INTERLEAVE );

    free( chunk );

    return data;
}

// read a compressed PVM volume
unsigned char* readPVMvolume( const char* filename,
                              unsigned int* width,
                              unsigned int* height,
                              unsigned int* depth,
                              unsigned int* bytes,
                              float* scalex,
                              float* scaley,
                              float* scalez,
                              unsigned char** desc,
                              unsigned char** courtesy,
                              unsigned char** parameter,
                              unsigned char** comment ) {
    unsigned char *data, *ptr;
    unsigned int datasize, numc;

    int version = 1;

    unsigned char* volume;

    float sx = 1.0f, sy = 1.0f, sz = 1.0f;

    unsigned int len1 = 0, len2 = 0, len3 = 0, len4 = 0;

    if ( ( data = readDDSfile( filename, &datasize ) ) == nullptr )
        if ( ( data = readRAWfile( filename, &datasize ) ) == nullptr ) return nullptr;

    if ( datasize < 5 ) return nullptr;

    if ( ( data = (unsigned char*)realloc( data, datasize + 1 ) ) == nullptr ) MEMERROR();
    data[datasize] = '\0';

    if ( strncmp( (char*)data, "PVM\n", 4 ) != 0 )
    {
        if ( strncmp( (char*)data, "PVM2\n", 5 ) == 0 )
            version = 2;
        else if ( strncmp( (char*)data, "PVM3\n", 5 ) == 0 )
            version = 3;
        else
            return nullptr;

        ptr = &data[5];
        if ( sscanf( (char*)ptr, "%u %u %u\n%g %g %g\n", width, height, depth, &sx, &sy, &sz ) !=
             6 )
            ERRORMSG();
        if ( *width < 1 || *height < 1 || *depth < 1 || sx <= 0.0f || sy <= 0.0f || sz <= 0.0f )
            ERRORMSG();
        ptr = (unsigned char*)strchr( (char*)ptr, '\n' ) + 1;
    }
    else
    {
        ptr = &data[4];
        while ( *ptr == '#' )
            while ( *ptr++ != '\n' )
                ;

        if ( sscanf( (char*)ptr, "%u %u %u\n", width, height, depth ) != 3 ) ERRORMSG();
        if ( *width < 1 || *height < 1 || *depth < 1 ) ERRORMSG();
    }

    if ( scalex != nullptr && scaley != nullptr && scalez != nullptr )
    {
        *scalex = sx;
        *scaley = sy;
        *scalez = sz;
    }

    ptr = (unsigned char*)strchr( (char*)ptr, '\n' ) + 1;
    if ( sscanf( (char*)ptr, "%u\n", &numc ) != 1 ) ERRORMSG();
    if ( numc < 1 ) ERRORMSG();

    if ( bytes != nullptr )
        *bytes = numc;
    else if ( numc != 1 )
        ERRORMSG();

    ptr = (unsigned char*)strchr( (char*)ptr, '\n' ) + 1;
    if ( version == 3 )
        len1 = strlen( (char*)( ptr + ( *width ) * ( *height ) * ( *depth ) * numc ) ) + 1;
    if ( version == 3 )
        len2 = strlen( (char*)( ptr + ( *width ) * ( *height ) * ( *depth ) * numc + len1 ) ) + 1;
    if ( version == 3 )
        len3 =
            strlen( (char*)( ptr + ( *width ) * ( *height ) * ( *depth ) * numc + len1 + len2 ) ) +
            1;
    if ( version == 3 )
        len4 = strlen( (char*)( ptr + ( *width ) * ( *height ) * ( *depth ) * numc + len1 + len2 +
                                len3 ) ) +
               1;
    if ( ( volume = (unsigned char*)malloc( ( *width ) * ( *height ) * ( *depth ) * numc + len1 +
                                            len2 + len3 + len4 ) ) == nullptr )
        MEMERROR();
    if ( data + datasize !=
         ptr + ( *width ) * ( *height ) * ( *depth ) * numc + len1 + len2 + len3 + len4 )
        ERRORMSG();

    memcpy( volume, ptr, ( *width ) * ( *height ) * ( *depth ) * numc + len1 + len2 + len3 + len4 );
    free( data );

    if ( desc != nullptr )
    {
        if ( len1 > 1 )
            *desc = volume + ( *width ) * ( *height ) * ( *depth ) * numc;
        else
            *desc = nullptr;
    }
    if ( courtesy != nullptr )
    {
        if ( len2 > 1 )
            *courtesy = volume + ( *width ) * ( *height ) * ( *depth ) * numc + len1;
        else
            *courtesy = nullptr;
    }

    if ( parameter != nullptr )
    {
        if ( len3 > 1 )
            *parameter = volume + ( *width ) * ( *height ) * ( *depth ) * numc + len1 + len2;
        else
            *parameter = nullptr;
    }

    if ( comment != nullptr )
    {
        if ( len4 > 1 )
            *comment = volume + ( *width ) * ( *height ) * ( *depth ) * numc + len1 + len2 + len3;
        else
            *comment = nullptr;
    }

    return volume;
}

} // namespace PVMVolume

} // namespace IO
} // namespace Ra
