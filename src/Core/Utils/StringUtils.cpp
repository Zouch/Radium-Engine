#include <Core/Utils/StringUtils.hpp>

/*
 * NOMINMAX is already defined in CoreMacros.hpp ...

#ifdef COMPILER_MSVC
#    define NOMINMAX // Avoid C2039 MSVC compiler error
//#undef vsnprintf
//#define vsnprintf(buffer, count, format, argptr) vsnprintf_s(buffer, count, count, format, argptr)
#endif
*/

#include <algorithm> // std::min std::max
#include <memory>

namespace Ra {
namespace Core {
namespace Utils {

std::string getFileExt( const std::string& str ) {
    auto pos        = str.find_last_of( '.' );
    std::string res = pos < str.length() ? str.substr( pos + 1 ) : "";
    return res;
}

std::string getDirName( const std::string& path ) {
    // We remove any trailing slashes.
    auto pos = path.find_last_not_of( '/' );

    // Don't strip the last / from "/"
    if ( pos >= path.length() ) { pos = path.find_first_of( "/" ); }
    std::string res = path.substr( 0, pos + 1 );

    // Now find the previous slash and cut the string.
    pos = res.find_last_of( '/' );

    // The directory is actually "/" because the last slash is in first position.
    // In that case we should return "/"
    if ( pos == 0 ) { res = "/"; }
    else if ( pos < res.length() ) {
        res = res.substr( 0, pos );
    }
    else {
        res = ".";
    }

    return res;
}

std::string getBaseName( const std::string& path, bool keepExtension ) {
    // We remove any trailing slashes.
    auto pos = path.find_last_not_of( '/' );
    // Don't strip the last / from "/"
    if ( pos >= path.length() ) { pos = path.find_first_of( "/" ); }
    std::string res = path.substr( 0, pos + 1 );

    // Now find the previous slash and cut the string.
    pos = res.find_last_of( '/' );
    if ( pos < res.length() ) { res = res.substr( pos + 1 ); }
    if ( !keepExtension ) {
        pos = res.find_last_of( '.' );
        if ( pos < res.length() ) { res = res.substr( 0, pos ); }
    }

    return res;
}

} // namespace Utils
} // namespace Core
} // namespace Ra
