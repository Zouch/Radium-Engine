/*****
 *
 *   Implementation of the interface to acces interpolated attributes from vertex attributes.
 *   This inteface MUST be used ONLY on fragment shaders.
 *
 *****/
//----------------- Supported vertex Attribs ---------------------
layout( location = 0 ) in vec3 in_position;
layout( location = 1 ) in vec3 in_normal;
layout( location = 2 ) in vec3 in_texcoord;
layout( location = 3 ) in vec3 in_vertexcolor;
#ifndef DONT_USE_INPUT_TANGENT
layout( location = 4 ) in vec3 in_tangent;
#endif

//------------------- VertexAttrib interface ---------------------
vec4 getWorldSpacePosition() {
    return vec4( in_position, 1.0 );
}

vec3 getWorldSpaceNormal() {
    if ( length( in_normal.xyz ) < 0.0001 ) { // Spec GLSL : vector not set -> (0, 0, 0, 1)
        vec3 X = dFdx( in_position );
        vec3 Y = dFdy( in_position );
        return normalize( cross( X, Y ) );
    }
    else {
        return normalize( in_normal );
    }
}

#ifndef DONT_USE_INPUT_TANGENT
vec3 getWorldSpaceTangent() {
    if ( length( in_tangent.xyz ) < 0.0001 ) { // Spec GLSL : vector not set -> (0, 0, 0, 1)
        return normalize( dFdx( in_position ) );
    }
    else {
        return normalize( in_tangent );
    }
}
#else
vec3 getWorldSpaceTangent() {
    return normalize( dFdx( in_position ) );
}
#endif

vec3 getWorldSpaceBiTangent() {
    return normalize( cross( in_normal, getWorldSpaceTangent() ) );
}

vec3 getPerVertexTexCoord() {
    return in_texcoord;
}

vec4 getPerVertexBaseColor() {
    return vec4( in_vertexcolor, 1.0 );
}
