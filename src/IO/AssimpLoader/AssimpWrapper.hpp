#pragma once

#include <assimp/color4.h>
#include <assimp/matrix4x4.h>
#include <assimp/quaternion.h>
#include <assimp/types.h>
#include <assimp/vector3.h>

#include <Core/Types.hpp>
#include <Core/Utils/Color.hpp>

namespace Ra {
namespace IO {

inline Core::Vector3 assimpToCore( const aiVector3D& v ) {
    return Core::Vector3( v.x, v.y, v.z );
}

inline aiVector3D coreToAssimp( const Core::Vector3& v ) {
    return aiVector3D( v.x(), v.y(), v.z() );
}

inline Core::Quaternion assimpToCore( const aiQuaternion& q ) {
    return Core::Quaternion( q.w, q.x, q.y, q.z );
}

inline Core::Transform assimpToCore( const aiMatrix4x4& M ) {
    Core::Matrix4 m( Core::Matrix4::Identity() );
    for ( uint i = 0; i < 4; ++i ) {
        for ( uint j = 0; j < 4; ++j ) {
            m( i, j ) = M[i][j];
        }
    }
    return Core::Transform( m );
}

inline Core::Transform
assimpToCore( const aiVector3D& T, const aiQuaternion& R, const aiVector3D& S ) {
    Core::Vector3 t    = assimpToCore( T );
    Core::Quaternion r = assimpToCore( R );
    Core::Vector3 s    = assimpToCore( S );
    Core::Transform M;
    M.fromPositionOrientationScale( t, r, s );
    return M;
}

inline Core::Utils::Color assimpToCore( const aiColor3D& c ) {
    return Core::Utils::Color( c.r, c.g, c.b );
}

inline Core::Utils::Color assimpToCore( const aiColor4D& c ) {
    return Core::Utils::Color( c.r, c.g, c.b, c.a );
}

inline std::string assimpToCore( const aiString& string ) {
    std::string result( string.C_Str() );
    std::transform( result.begin(), result.end(), result.begin(), []( char in ) {
        if ( in == '\\' ) return '/';
        return in;
    } );
    return result.empty() ? "default" : result;
}

inline Core::VectorNi assimpToCore( const uint* index, const uint size ) {
    Core::VectorNi v( size );
    for ( uint i = 0; i < size; ++i ) {
        v[i] = index[i];
    }

    return v;
}

} // namespace IO
} // namespace Ra
